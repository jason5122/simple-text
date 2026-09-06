#include "experiments/platform/conformance/linux/capture_frame.h"

#include <gio/gio.h>
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace linux_capture {
namespace {

constexpr char kDestination[] = "org.gnome.Mutter.ScreenCast";
constexpr char kManagerPath[] = "/org/gnome/Mutter/ScreenCast";
constexpr char kManagerInterface[] = "org.gnome.Mutter.ScreenCast";
constexpr char kSessionInterface[] = "org.gnome.Mutter.ScreenCast.Session";
constexpr char kStreamInterface[] = "org.gnome.Mutter.ScreenCast.Stream";
constexpr char kDisplayConfigDestination[] = "org.gnome.Mutter.DisplayConfig";
constexpr char kDisplayConfigPath[] = "/org/gnome/Mutter/DisplayConfig";
constexpr char kDisplayConfigInterface[] = "org.gnome.Mutter.DisplayConfig";

std::string glib_error(const char* operation, GError* error) {
    std::string message = operation;
    message += ": ";
    message += error ? error->message : "unknown error";
    if (error) g_error_free(error);
    return message;
}

GVariant* call(GDBusConnection* connection,
               const char* path,
               const char* interface,
               const char* method,
               GVariant* parameters,
               GError** error) {
    return g_dbus_connection_call_sync(connection, kDestination, path, interface, method,
                                       parameters, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
                                       error);
}

class MutterSource final : public Source {
public:
    ~MutterSource() override {
        if (session_started_ && connection_) {
            GError* error = nullptr;
            GVariant* reply = call(connection_, session_path_.c_str(), kSessionInterface, "Stop",
                                   nullptr, &error);
            if (reply) g_variant_unref(reply);
            if (error) g_error_free(error);
        }
        if (thread_loop_) pw_thread_loop_stop(thread_loop_);
        if (stream_) pw_stream_destroy(stream_);
        if (core_) pw_core_disconnect(core_);
        if (context_) pw_context_destroy(context_);
        if (thread_loop_) pw_thread_loop_destroy(thread_loop_);
        if (signal_subscription_ && connection_) {
            g_dbus_connection_signal_unsubscribe(connection_, signal_subscription_);
        }
        if (connection_) g_object_unref(connection_);
    }

    bool initialize(std::string* error) {
        GError* glib = nullptr;
        connection_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &glib);
        if (!connection_) {
            *error = glib_error("connect to session bus", glib);
            return false;
        }
        device_scale_ = primary_monitor_scale();

        GVariantBuilder session_properties;
        g_variant_builder_init(&session_properties, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&session_properties, "{sv}", "disable-animations",
                              g_variant_new_boolean(TRUE));
        GVariant* reply = call(connection_, kManagerPath, kManagerInterface, "CreateSession",
                               g_variant_new("(a{sv})", &session_properties), &glib);
        if (!reply) {
            *error = glib_error("Mutter CreateSession", glib);
            return false;
        }
        const char* session_path = nullptr;
        g_variant_get(reply, "(&o)", &session_path);
        session_path_ = session_path;
        g_variant_unref(reply);

        // Omitting window-id is a documented private-API behavior: Mutter records the focused
        // window. The runner requires Sublime to be focused before starting this server, after
        // which the stream remains attached to that MetaWindow regardless of later focus changes.
        GVariantBuilder stream_properties;
        g_variant_builder_init(&stream_properties, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&stream_properties, "{sv}", "cursor-mode",
                              g_variant_new_uint32(0));
        reply = call(connection_, session_path_.c_str(), kSessionInterface, "RecordWindow",
                     g_variant_new("(a{sv})", &stream_properties), &glib);
        if (!reply) {
            *error = glib_error("Mutter RecordWindow", glib);
            return false;
        }
        const char* stream_path = nullptr;
        g_variant_get(reply, "(&o)", &stream_path);
        stream_path_ = stream_path;
        g_variant_unref(reply);

        signal_subscription_ = g_dbus_connection_signal_subscribe(
            connection_, kDestination, kStreamInterface, "PipeWireStreamAdded",
            stream_path_.c_str(), nullptr, G_DBUS_SIGNAL_FLAGS_NONE, &on_dbus_signal, this,
            nullptr);
        reply = call(connection_, session_path_.c_str(), kSessionInterface, "Start", nullptr,
                     &glib);
        if (!reply) {
            *error = glib_error("Mutter ScreenCast Start", glib);
            return false;
        }
        session_started_ = true;
        g_variant_unref(reply);

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (pipewire_node_ == 0 && std::chrono::steady_clock::now() < deadline) {
            while (g_main_context_iteration(nullptr, FALSE)) {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (pipewire_node_ == 0) {
            *error = "timed out waiting for Mutter's PipeWire stream";
            return false;
        }
        return start_pipewire(error);
    }

    bool next_frame(Frame* frame, Crop crop, std::chrono::milliseconds timeout) override {
        std::unique_lock lock(mutex_);
        const uint64_t previous = consumed_sequence_;
        condition_.wait_for(lock, timeout,
                            [&] { return sequence_ != previous || !pipewire_error_.empty(); });
        if (sequence_ == previous || latest_.rgba.empty()) return false;
        consumed_sequence_ = sequence_;

        const int left = std::clamp(crop.x, 0, latest_.width);
        const int top = std::clamp(crop.y, 0, latest_.height);
        const int width = std::clamp(crop.width > 0 ? crop.width : latest_.width - left, 0,
                                     latest_.width - left);
        const int height = std::clamp(crop.height > 0 ? crop.height : latest_.height - top, 0,
                                      latest_.height - top);
        if (width == 0 || height == 0) return false;

        frame->width = width;
        frame->height = height;
        frame->rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        for (int y = 0; y < height; ++y) {
            std::memcpy(frame->rgba.data() + static_cast<size_t>(y) * width * 4,
                        latest_.rgba.data() +
                            (static_cast<size_t>(top + y) * latest_.width + left) * 4,
                        static_cast<size_t>(width) * 4);
        }
        return true;
    }

    double device_scale() const override { return device_scale_; }

private:
    double primary_monitor_scale() {
        GError* error = nullptr;
        GVariant* reply = g_dbus_connection_call_sync(
            connection_, kDisplayConfigDestination, kDisplayConfigPath,
            kDisplayConfigInterface, "GetCurrentState", nullptr, nullptr,
            G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
        if (!reply) {
            std::fprintf(stderr, "Mutter GetCurrentState: %s\n",
                         error ? error->message : "unknown error");
            if (error) g_error_free(error);
            return 1.0;
        }

        // Result child 2 is a(iiduba(ssss)a{sv}): logical monitor x/y, scale, transform,
        // primary flag, physical monitors, and properties. Prefer the primary monitor; the
        // conformance VM normally has exactly one.
        GVariant* logical_monitors = g_variant_get_child_value(reply, 2);
        GVariantIter iterator;
        g_variant_iter_init(&iterator, logical_monitors);
        double first_scale = 1.0;
        bool have_scale = false;
        GVariant* logical_monitor = nullptr;
        while ((logical_monitor = g_variant_iter_next_value(&iterator))) {
            GVariant* scale_value = g_variant_get_child_value(logical_monitor, 2);
            GVariant* primary_value = g_variant_get_child_value(logical_monitor, 4);
            const double scale = g_variant_get_double(scale_value);
            const bool primary = g_variant_get_boolean(primary_value);
            g_variant_unref(scale_value);
            g_variant_unref(primary_value);
            g_variant_unref(logical_monitor);
            if (!have_scale) {
                first_scale = scale;
                have_scale = true;
            }
            if (primary) {
                g_variant_unref(logical_monitors);
                g_variant_unref(reply);
                return scale;
            }
        }
        g_variant_unref(logical_monitors);
        g_variant_unref(reply);
        return first_scale;
    }

    static void on_dbus_signal(GDBusConnection*,
                               const gchar*,
                               const gchar*,
                               const gchar*,
                               const gchar*,
                               GVariant* parameters,
                               gpointer user_data) {
        auto* self = static_cast<MutterSource*>(user_data);
        g_variant_get(parameters, "(u)", &self->pipewire_node_);
    }

    bool start_pipewire(std::string* error) {
        pw_init(nullptr, nullptr);
        thread_loop_ = pw_thread_loop_new("conformance-window-capture", nullptr);
        if (!thread_loop_) {
            *error = "pw_thread_loop_new failed";
            return false;
        }
        context_ = pw_context_new(pw_thread_loop_get_loop(thread_loop_), nullptr, 0);
        if (!context_) {
            *error = "pw_context_new failed";
            return false;
        }
        core_ = pw_context_connect(context_, nullptr, 0);
        if (!core_) {
            *error = "pw_context_connect failed";
            return false;
        }

        pw_properties* properties =
            pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture",
                              PW_KEY_MEDIA_ROLE, "Screen", nullptr);
        stream_ = pw_stream_new(core_, "conformance window capture", properties);
        if (!stream_) {
            *error = "pw_stream_new failed";
            return false;
        }

        static const pw_stream_events events = {
            .version = PW_VERSION_STREAM_EVENTS,
            .state_changed = &on_stream_state_changed,
            .param_changed = &on_stream_param_changed,
            .process = &on_stream_process,
        };
        pw_stream_add_listener(stream_, &stream_listener_, &events, this);

        std::uint8_t pod_buffer[1024];
        spa_pod_builder builder = SPA_POD_BUILDER_INIT(pod_buffer, sizeof(pod_buffer));
        const spa_pod* parameters[] = {static_cast<const spa_pod*>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
            SPA_POD_CHOICE_ENUM_Id(5, SPA_VIDEO_FORMAT_BGRA, SPA_VIDEO_FORMAT_BGRA,
                                   SPA_VIDEO_FORMAT_RGBA, SPA_VIDEO_FORMAT_BGRx,
                                   SPA_VIDEO_FORMAT_RGBx)))};
        const int result = pw_stream_connect(
            stream_, PW_DIRECTION_INPUT, pipewire_node_,
            static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
            parameters, 1);
        if (result < 0) {
            *error = "pw_stream_connect failed: " + std::to_string(result);
            return false;
        }
        if (pw_thread_loop_start(thread_loop_) < 0) {
            *error = "pw_thread_loop_start failed";
            return false;
        }
        return true;
    }

    static void on_stream_state_changed(void* user_data,
                                        pw_stream_state,
                                        pw_stream_state state,
                                        const char* error) {
        if (state != PW_STREAM_STATE_ERROR) return;
        auto* self = static_cast<MutterSource*>(user_data);
        std::lock_guard lock(self->mutex_);
        self->pipewire_error_ = error ? error : "PipeWire stream error";
        std::fprintf(stderr, "PipeWire stream error: %s\n", self->pipewire_error_.c_str());
        self->condition_.notify_all();
    }

    static void on_stream_param_changed(void* user_data, uint32_t id, const spa_pod* parameter) {
        if (!parameter || id != SPA_PARAM_Format) return;
        auto* self = static_cast<MutterSource*>(user_data);
        spa_video_info_raw format{};
        if (spa_format_video_raw_parse(parameter, &format) < 0) return;
        std::lock_guard lock(self->mutex_);
        self->format_ = format;
    }

    static void on_stream_process(void* user_data) {
        auto* self = static_cast<MutterSource*>(user_data);
        pw_buffer* selected = nullptr;
        while (pw_buffer* candidate = pw_stream_dequeue_buffer(self->stream_)) {
            if (selected) pw_stream_queue_buffer(self->stream_, selected);
            selected = candidate;
        }
        if (!selected) return;

        spa_buffer* buffer = selected->buffer;
        spa_data* data = buffer && buffer->n_datas > 0 ? &buffer->datas[0] : nullptr;
        spa_video_info_raw format;
        {
            std::lock_guard lock(self->mutex_);
            format = self->format_;
        }
        if (!data || !data->data || !data->chunk || format.size.width == 0 ||
            format.size.height == 0) {
            pw_stream_queue_buffer(self->stream_, selected);
            return;
        }

        const int width = static_cast<int>(format.size.width);
        const int height = static_cast<int>(format.size.height);
        int stride = data->chunk->stride;
        if (stride == 0) stride = width * 4;
        const auto* bytes = static_cast<const std::uint8_t*>(data->data) + data->chunk->offset;
        if (stride < 0) bytes += static_cast<size_t>(height - 1) * -stride;

        Frame frame;
        frame.width = width;
        frame.height = height;
        frame.rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        for (int y = 0; y < height; ++y) {
            const auto* source = bytes + static_cast<ptrdiff_t>(y) * stride;
            auto* destination = frame.rgba.data() + static_cast<size_t>(y) * width * 4;
            for (int x = 0; x < width; ++x) {
                switch (format.format) {
                case SPA_VIDEO_FORMAT_BGRA:
                    destination[0] = source[2];
                    destination[1] = source[1];
                    destination[2] = source[0];
                    destination[3] = source[3];
                    break;
                case SPA_VIDEO_FORMAT_RGBA:
                    destination[0] = source[0];
                    destination[1] = source[1];
                    destination[2] = source[2];
                    destination[3] = source[3];
                    break;
                case SPA_VIDEO_FORMAT_BGRx:
                    destination[0] = source[2];
                    destination[1] = source[1];
                    destination[2] = source[0];
                    destination[3] = 255;
                    break;
                case SPA_VIDEO_FORMAT_RGBx:
                    destination[0] = source[0];
                    destination[1] = source[1];
                    destination[2] = source[2];
                    destination[3] = 255;
                    break;
                default:
                    pw_stream_queue_buffer(self->stream_, selected);
                    return;
                }
                source += 4;
                destination += 4;
            }
        }
        {
            std::lock_guard lock(self->mutex_);
            self->latest_ = std::move(frame);
            ++self->sequence_;
        }
        self->condition_.notify_all();
        pw_stream_queue_buffer(self->stream_, selected);
    }

    GDBusConnection* connection_ = nullptr;
    guint signal_subscription_ = 0;
    std::string session_path_;
    std::string stream_path_;
    bool session_started_ = false;
    uint32_t pipewire_node_ = 0;

    pw_thread_loop* thread_loop_ = nullptr;
    pw_context* context_ = nullptr;
    pw_core* core_ = nullptr;
    pw_stream* stream_ = nullptr;
    spa_hook stream_listener_{};

    std::mutex mutex_;
    std::condition_variable condition_;
    spa_video_info_raw format_{};
    Frame latest_;
    uint64_t sequence_ = 0;
    uint64_t consumed_sequence_ = 0;
    std::string pipewire_error_;
    double device_scale_ = 1.0;
};

}  // namespace

std::unique_ptr<Source> make_mutter_source(std::string* error) {
    auto source = std::make_unique<MutterSource>();
    if (!source->initialize(error)) return nullptr;
    return source;
}

}  // namespace linux_capture
