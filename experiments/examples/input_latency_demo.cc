// One editable line, for input-to-glass latency measurements against other editors.
//
// Typed characters are inserted at the caret, Backspace deletes one code point, a click moves the
// caret to the nearest glyph boundary and Escape quits. Each change repaints only the line's band,
// so what an external observer sees is the floor of the stack: event delivery, one paint, one
// present. Pair it with a tool that posts the key or click at the HID tap and takes the display
// time of the first changed frame from ScreenCaptureKit.
//
// INPUT_LATENCY_LOG=1 prints, for every change, the time from the event's HID timestamp to
// keyDown, from keyDown to insert_text (the NSTextInputContext round trip, measured at 60 us),
// from there to the paint, and from the paint to the frame reaching the glass.

#include "px/px.h"
#include "ui/retained_text.h"
#include "ui/window.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr double kTextX = 40.0;
constexpr double kLineTop = 80.0;
constexpr double kBandHeight = 48.0;
constexpr std::string_view kInitialText = "The quick brown fox";

class InputLatencyControl final : public control, public px_input_client {
public:
    InputLatencyControl(window* w, px_font_t* font) : window_(w), font_(font) {
        metrics_ = px_font_get_metrics(font);
        log_ = std::getenv("INPUT_LATENCY_LOG") != nullptr;
        text_ = kInitialText;
        relayout();
        caret_ = boundaries_.size() - 1;
    }

    bool handle_event(const px_event_t* event) override {
        switch (event->type) {
        case PX_EVENT_KEY:
            if (!event->pressed) {
                break;
            }
            if (event->key == PX_KEY_ESCAPE) {
                window_->close();
                return true;
            }
            if (event->key == PX_KEY_BACKSPACE) {
                begin_change("backspace", event->timestamp);
                pending_.t_insert = px_now();
                delete_backward();
                return true;
            }
            // Not consumed: the platform hands the event to the input context, which calls
            // insert_text below.
            begin_change("key", event->timestamp);
            break;
        case PX_EVENT_MOUSE_BUTTON:
            if (event->pressed && event->button == PX_MOUSE_LEFT) {
                begin_change("click", event->timestamp);
                pending_.t_insert = px_now();
                caret_ = index_for_x(event->pos.x);
                mark_line_dirty();
                return true;
            }
            break;
        default:
            break;
        }
        return false;
    }

    void draw(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        if (pending_.kind && pending_.frame_id == 0) {
            pending_.frame_id = px_current_frame_id(window_->px_window());
            pending_.t_paint = px_now();
        }
        rc->begin_rect_batch();
        rc->draw_rect(bounds, fcolor{1.0f, 1.0f, 1.0f, 1.0f});
        const double caret_x = kTextX + prefix_advance_[caret_];
        rc->draw_rect(rect{caret_x, kLineTop + 6.0, 2.0, kBandHeight - 12.0},
                      fcolor{0.1f, 0.4f, 0.9f, 1.0f});
        rc->end_rect_batch();

        rc->begin_text_batch();
        const double baseline =
            kLineTop + (kBandHeight - metrics_.line_height) * 0.5 + metrics_.ascent;
        draw_retained_text(rc, font_, vec2{kTextX, baseline}, fcolor{0.1f, 0.1f, 0.1f, 1.0f},
                           &layout_);
        rc->end_text_batch();
    }

    // Called on the main thread when a frame reaches the glass.
    void on_presented(uint64_t frame_id, double presented) {
        if (!pending_.kind || frame_id != pending_.frame_id) {
            return;
        }
        if (log_) {
            std::printf("%-10s hid->keyDown %6.2f  keyDown->insert %6.2f  insert->paint %6.2f  "
                        "paint->glass %6.2f  total %6.2f ms\n",
                        pending_.kind, (pending_.t_key - pending_.t_event) * 1000.0,
                        (pending_.t_insert - pending_.t_key) * 1000.0,
                        (pending_.t_paint - pending_.t_insert) * 1000.0,
                        (presented - pending_.t_paint) * 1000.0,
                        (presented - pending_.t_event) * 1000.0);
            std::fflush(stdout);
        }
        pending_ = {};
    }

    // px_input_client
    void insert_text(const char* utf8, px_range_t replacement) override {
        pending_.t_insert = px_now();
        marked_.clear();
        insert_at_caret(utf8);
    }

    void set_marked_text(const char* utf8, px_range_t selected, px_range_t replacement) override {
        marked_ = utf8;
    }
    void unmark_text() override { marked_.clear(); }
    bool has_marked_text() const override { return !marked_.empty(); }
    px_range_t marked_range() const override {
        if (marked_.empty()) {
            return px_range_t::none();
        }
        return px_range_t{static_cast<int64_t>(boundaries_[caret_]),
                          static_cast<int64_t>(marked_.size())};
    }
    px_range_t selected_range() const override {
        return px_range_t{static_cast<int64_t>(boundaries_[caret_]), 0};
    }
    rect first_rect_for_range(px_range_t range, px_range_t* actual) override {
        if (actual) {
            *actual = px_range_t{static_cast<int64_t>(boundaries_[caret_]), 0};
        }
        return rect{kTextX + prefix_advance_[caret_], kLineTop, 1.0, kBandHeight};
    }
    int64_t character_index_for_point(vec2 pos) override {
        return static_cast<int64_t>(boundaries_[index_for_x(pos.x)]);
    }
    void do_command(const char* selector_name) override {
        if (!std::strcmp(selector_name, "deleteBackward:")) {
            delete_backward();
        } else if (!std::strcmp(selector_name, "moveLeft:") && caret_ > 0) {
            --caret_;
            mark_line_dirty();
        } else if (!std::strcmp(selector_name, "moveRight:") && caret_ + 1 < boundaries_.size()) {
            ++caret_;
            mark_line_dirty();
        }
    }

private:
    struct pending_change {
        const char* kind = nullptr;
        double t_event = 0.0;   // the event's HID timestamp
        double t_key = 0.0;     // when the app saw the event
        double t_insert = 0.0;  // when the text or caret actually changed
        double t_paint = 0.0;   // the first paint afterwards
        uint64_t frame_id = 0;
    };

    void begin_change(const char* kind, double event_time) {
        pending_ = {};
        pending_.kind = kind;
        pending_.t_event = event_time;
        pending_.t_key = px_now();
    }

    void insert_at_caret(const char* utf8) {
        text_.insert(boundaries_[caret_], utf8);
        const size_t caret_byte = boundaries_[caret_] + std::strlen(utf8);
        relayout();
        caret_ = index_for_byte(caret_byte);
        mark_line_dirty();
    }

    // Code point boundaries of text_, as byte offsets, from 0 to size().
    void relayout() {
        boundaries_.clear();
        for (size_t i = 0; i < text_.size(); ++i) {
            if ((static_cast<unsigned char>(text_[i]) & 0xC0) != 0x80) {
                boundaries_.push_back(i);
            }
        }
        boundaries_.push_back(text_.size());
        // One native layout for the whole line; each glyph's cluster is the byte offset of its
        // source character, which places every boundary without shaping prefixes.
        layout_ = prepare_retained_text(font_, text_);
        std::vector<std::pair<uint32_t, double>> starts;
        for (const retained_text_batch& batch : layout_.batches) {
            for (const fx_glyph& glyph : batch.layout.glyphs) {
                starts.emplace_back(glyph.cluster, batch.x_offset + glyph.x_offset);
            }
        }
        std::sort(starts.begin(), starts.end());
        prefix_advance_.assign(boundaries_.size(), layout_.advance);
        size_t j = 0;
        for (size_t i = 0; i < boundaries_.size(); ++i) {
            while (j < starts.size() && starts[j].first < boundaries_[i]) {
                ++j;
            }
            if (j < starts.size()) {
                prefix_advance_[i] = starts[j].second;
            }
        }
        if (caret_ >= boundaries_.size()) {
            caret_ = boundaries_.size() - 1;
        }
    }

    void delete_backward() {
        if (caret_ == 0) {
            return;
        }
        const size_t from = boundaries_[caret_ - 1];
        const size_t to = boundaries_[caret_];
        text_.erase(from, to - from);
        relayout();
        caret_ = index_for_byte(from);
        mark_line_dirty();
    }

    size_t index_for_byte(size_t byte) const {
        const auto it = std::lower_bound(boundaries_.begin(), boundaries_.end(), byte);
        return it == boundaries_.end() ? boundaries_.size() - 1
                                       : static_cast<size_t>(it - boundaries_.begin());
    }

    size_t index_for_x(double x) const {
        size_t best = 0;
        double best_distance = 1e9;
        for (size_t i = 0; i < prefix_advance_.size(); ++i) {
            const double distance = std::abs(kTextX + prefix_advance_[i] - x);
            if (distance < best_distance) {
                best_distance = distance;
                best = i;
            }
        }
        return best;
    }

    void mark_line_dirty() { window_->mark_rect_dirty(rect{0.0, kLineTop, 8192.0, kBandHeight}); }

    window* window_ = nullptr;
    px_font_t* font_ = nullptr;
    px_font_metrics metrics_;
    bool log_ = false;
    pending_change pending_;
    std::string text_;
    std::string marked_;
    std::vector<size_t> boundaries_;
    std::vector<double> prefix_advance_;
    retained_text layout_;
    size_t caret_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("input_latency_demo", "com.example.input_latency_demo", argc, argv, 0);

    window_impl win(1300.0, 600.0, "input latency demo", fcolor{1.0f, 1.0f, 1.0f, 1.0f});
    window_basic_aspect basic(&win);
    win.add_window_aspect(&basic);

    px_font_t* font = px_create_font("Menlo", 16.0f);
    InputLatencyControl root(&win, font);
    win.set_root_control(&root);
    win.set_input_client(&root);
    px_set_frame_presented_callback(win.px_window(), [&root](uint64_t frame_id, double presented) {
        root.on_presented(frame_id, presented);
    });

    win.show();
    px_run_event_loop();
    return 0;
}
