#include "experiments/platform/conformance/capture.h"
#include "experiments/platform/px/px_gl.h"
#include "experiments/platform/px/win/px_win_private.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <limits>
#include <string>
#include <vector>
#include <wincodec.h>
#include <windows.h>

namespace {

constexpr double kPollSeconds = 0.002;
constexpr double kGraceSeconds = 0.2;
constexpr double kTimeoutSeconds = 3.0;

template <typename T>
void release(T*& object) {
    if (object) {
        object->Release();
        object = nullptr;
    }
}

void print_hresult(const wchar_t* operation, HRESULT result) {
    std::fwprintf(stderr, L"%ls failed: HRESULT 0x%08lx\n", operation,
                  static_cast<unsigned long>(result));
}

std::wstring to_utf16(const char* utf8) {
    if (!utf8 || !*utf8) return {};
    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (length <= 0) return {};
    std::wstring result(static_cast<size_t>(length - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result.data(), length);
    return result;
}

struct WindowsFrame {
    UINT width = 0;
    UINT height = 0;
    std::vector<std::uint8_t> pixels;
};

class PngWriter {
public:
    ~PngWriter() {
        release(factory_);
        if (uninitialize_com_) CoUninitialize();
    }

    bool write(WindowsFrame* frame, const wchar_t* path) {
        if (!initialize()) return false;
        const std::uint64_t row_pitch = static_cast<std::uint64_t>(frame->width) * 4;
        const std::uint64_t buffer_size = row_pitch * frame->height;
        if (buffer_size > std::numeric_limits<DWORD>::max()) {
            std::fwprintf(stderr, L"Image is too large for the WIC PNG encoder\n");
            return false;
        }

        IWICStream* stream = nullptr;
        IWICBitmapEncoder* encoder = nullptr;
        IWICBitmapFrameEncode* image = nullptr;
        IPropertyBag2* properties = nullptr;
        const wchar_t* failed_operation = nullptr;
        HRESULT result = factory_->CreateStream(&stream);
        if (FAILED(result)) {
            failed_operation = L"IWICImagingFactory::CreateStream";
        } else if (FAILED(result = stream->InitializeFromFilename(path, GENERIC_WRITE))) {
            failed_operation = L"IWICStream::InitializeFromFilename";
        } else if (FAILED(result = factory_->CreateEncoder(GUID_ContainerFormatPng, nullptr,
                                                           &encoder))) {
            failed_operation = L"IWICImagingFactory::CreateEncoder(PNG)";
        } else if (FAILED(result = encoder->Initialize(stream, WICBitmapEncoderNoCache))) {
            failed_operation = L"IWICBitmapEncoder::Initialize";
        } else if (FAILED(result = encoder->CreateNewFrame(&image, &properties))) {
            failed_operation = L"IWICBitmapEncoder::CreateNewFrame";
        } else if (FAILED(result = image->Initialize(properties))) {
            failed_operation = L"IWICBitmapFrameEncode::Initialize";
        } else if (FAILED(result = image->SetSize(frame->width, frame->height))) {
            failed_operation = L"IWICBitmapFrameEncode::SetSize";
        } else {
            WICPixelFormatGUID pixel_format = GUID_WICPixelFormat32bppBGRA;
            result = image->SetPixelFormat(&pixel_format);
            if (FAILED(result) || !IsEqualGUID(pixel_format, GUID_WICPixelFormat32bppBGRA)) {
                if (SUCCEEDED(result)) result = WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
                failed_operation = L"IWICBitmapFrameEncode::SetPixelFormat";
            } else if (FAILED(result = image->WritePixels(
                                  frame->height, static_cast<UINT>(row_pitch),
                                  static_cast<DWORD>(buffer_size), frame->pixels.data()))) {
                failed_operation = L"IWICBitmapFrameEncode::WritePixels";
            } else if (FAILED(result = image->Commit())) {
                failed_operation = L"IWICBitmapFrameEncode::Commit";
            } else if (FAILED(result = encoder->Commit())) {
                failed_operation = L"IWICBitmapEncoder::Commit";
            }
        }

        if (failed_operation) print_hresult(failed_operation, result);
        release(properties);
        release(image);
        release(encoder);
        release(stream);
        return !failed_operation;
    }

private:
    bool initialize() {
        if (factory_) return true;
        const HRESULT com_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        uninitialize_com_ = SUCCEEDED(com_result);
        if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
            print_hresult(L"CoInitializeEx", com_result);
            return false;
        }
        const HRESULT result =
            CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                             __uuidof(IWICImagingFactory), reinterpret_cast<void**>(&factory_));
        if (FAILED(result)) {
            print_hresult(L"CoCreateInstance(CLSID_WICImagingFactory)", result);
            return false;
        }
        return true;
    }

    bool uninitialize_com_ = false;
    IWICImagingFactory* factory_ = nullptr;
};

PngWriter& png_writer() {
    static PngWriter writer;
    return writer;
}

bool frames_equal(WindowsFrame* lhs, WindowsFrame* rhs) {
    return lhs && rhs && lhs->width == rhs->width && lhs->height == rhs->height &&
           lhs->pixels == rhs->pixels;
}

struct WindowSearch {
    DWORD process_id = 0;
    const wchar_t* process_name = nullptr;
    HWND result = nullptr;
};

std::wstring process_name(HWND window) {
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (!process) return {};
    wchar_t path[32768];
    DWORD length = ARRAYSIZE(path);
    const BOOL ok = QueryFullProcessImageNameW(process, 0, path, &length);
    CloseHandle(process);
    if (!ok) return {};
    const wchar_t* filename = path;
    for (DWORD i = 0; i < length; ++i) {
        if (path[i] == L'\\' || path[i] == L'/') filename = path + i + 1;
    }
    std::wstring name(filename);
    const size_t extension = name.rfind(L'.');
    if (extension != std::wstring::npos) name.resize(extension);
    return name;
}

BOOL CALLBACK find_window_callback(HWND window, LPARAM parameter) {
    auto* search = reinterpret_cast<WindowSearch*>(parameter);
    if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER)) return TRUE;
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    if ((search->process_id && process_id == search->process_id) ||
        (search->process_name &&
         _wcsicmp(process_name(window).c_str(), search->process_name) == 0)) {
        search->result = window;
        return FALSE;
    }
    return TRUE;
}

}  // namespace

namespace capture {

Frame capture_frame(WindowId window_id, Crop crop) {
    const HWND window = reinterpret_cast<HWND>(window_id);
    RECT client{};
    if (!window || !GetClientRect(window, &client) || !wglGetCurrentContext()) return nullptr;

    const int client_width = client.right - client.left;
    const int client_height = client.bottom - client.top;
    const int left = std::clamp(crop.x, 0, client_width);
    const int top = std::clamp(crop.y, 0, client_height);
    const int requested_width = crop.w > 0 ? crop.w : client_width - left;
    const int requested_height = crop.h > 0 ? crop.h : client_height - top;
    const int width = std::clamp(requested_width, 0, client_width - left);
    const int height = std::clamp(requested_height, 0, client_height - top);
    if (width == 0 || height == 0) return nullptr;

    auto* frame = new WindowsFrame{
        .width = static_cast<UINT>(width),
        .height = static_cast<UINT>(height),
        .pixels = std::vector<std::uint8_t>(static_cast<size_t>(width) *
                                            static_cast<size_t>(height) * 4),
    };
    std::vector<std::uint8_t> bottom_up(frame->pixels.size());
    GLint pack_alignment = 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &pack_alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glFinish();
    glReadBuffer(GL_FRONT);
    glReadPixels(left, client_height - top - height, width, height, GL_BGRA, GL_UNSIGNED_BYTE,
                 bottom_up.data());
    glPixelStorei(GL_PACK_ALIGNMENT, pack_alignment);
    if (glGetError() != GL_NO_ERROR) {
        delete frame;
        return nullptr;
    }

    const size_t row_bytes = static_cast<size_t>(width) * 4;
    for (int y = 0; y < height; ++y) {
        std::copy_n(bottom_up.data() + static_cast<size_t>(height - y - 1) * row_bytes, row_bytes,
                    frame->pixels.data() + static_cast<size_t>(y) * row_bytes);
    }
    return frame;
}

void release_frame(Frame frame) { delete static_cast<WindowsFrame*>(frame); }

bool frame_to_png(Frame frame, const char* out_path) {
    auto* windows_frame = static_cast<WindowsFrame*>(frame);
    const std::wstring wide_path = to_utf16(out_path);
    return windows_frame && !wide_path.empty() &&
           png_writer().write(windows_frame, wide_path.c_str());
}

void pump(double seconds) {
    for (px_window_t* window : px_win_all_windows()) {
        px_win_flush_dirty_rects(window);
    }

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
    while (true) {
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) return;
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        const auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::steady_clock::duration::zero()) return;
        const auto remaining_ms = std::chrono::duration<double, std::milli>(remaining).count();
        const DWORD timeout = static_cast<DWORD>(std::max(1.0, std::ceil(remaining_ms)));
        MsgWaitForMultipleObjectsEx(0, nullptr, timeout, QS_ALLINPUT,
                                    MWMO_INPUTAVAILABLE | MWMO_ALERTABLE);
    }
}

Frame wait_settled(WindowId window_id, Crop crop, Frame baseline) {
    const auto started_at = std::chrono::steady_clock::now();
    Frame last = nullptr;
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count() <
           kTimeoutSeconds) {
        pump(kPollSeconds);
        Frame current = capture_frame(window_id, crop);
        if (!current) continue;
        const bool changed = !baseline || !frames_equal(static_cast<WindowsFrame*>(current),
                                                        static_cast<WindowsFrame*>(baseline));
        const bool stable = last && frames_equal(static_cast<WindowsFrame*>(current),
                                                 static_cast<WindowsFrame*>(last));
        const bool graced =
            stable &&
            std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count() >=
                kGraceSeconds;
        if ((changed && stable) || graced) {
            release_frame(last);
            return current;
        }
        release_frame(last);
        last = current;
    }
    return last;
}

WindowId find_window(const char* owner) {
    const std::wstring process = to_utf16(owner);
    WindowSearch search{.process_name = process.c_str()};
    EnumWindows(&find_window_callback, reinterpret_cast<LPARAM>(&search));
    return reinterpret_cast<WindowId>(search.result);
}

WindowId find_window_for_pid(int pid) {
    WindowSearch search{.process_id = static_cast<DWORD>(pid)};
    EnumWindows(&find_window_callback, reinterpret_cast<LPARAM>(&search));
    return reinterpret_cast<WindowId>(search.result);
}

}  // namespace capture
