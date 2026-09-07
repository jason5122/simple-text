#include <d3d11.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using DwmGetDxSharedSurfaceFn = BOOL(WINAPI*)(HWND, HANDLE*, LUID*, ULONG*, ULONG*, ULONGLONG*);

constexpr DWORD kFrameTimeoutMs = 3000;
constexpr DWORD kFrameQuietMs = 100;
constexpr DWORD kFramePollMs = 10;
constexpr UINT kCaptureWidth = 1600;
constexpr UINT kCaptureHeight = 600;

template <typename T>
void release(T*& object) {
    if (object) {
        object->Release();
        object = nullptr;
    }
}

bool same_luid(const LUID& lhs, const LUID& rhs) {
    return lhs.LowPart == rhs.LowPart && lhs.HighPart == rhs.HighPart;
}

void print_hresult(const wchar_t* operation, HRESULT result) {
    std::fwprintf(stderr, L"%ls failed: HRESULT 0x%08lx\n", operation,
                  static_cast<unsigned long>(result));
}

void print_last_error(const wchar_t* operation) {
    std::fwprintf(stderr, L"%ls failed: Win32 error %lu\n", operation,
                  static_cast<unsigned long>(GetLastError()));
}

struct WindowInfo {
    HWND window = nullptr;
    std::wstring image_path;
};

bool get_window_image_path(HWND window, std::wstring* image_path) {
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (!process) return false;

    wchar_t buffer[32768];
    DWORD buffer_size = ARRAYSIZE(buffer);
    const BOOL got_path = QueryFullProcessImageNameW(process, 0, buffer, &buffer_size);
    CloseHandle(process);
    if (!got_path) return false;

    image_path->assign(buffer, buffer_size);
    return true;
}

bool is_sublime_window(HWND window, WindowInfo* info) {
    if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER)) return false;

    std::wstring image_path;
    if (!get_window_image_path(window, &image_path)) return false;
    const std::wstring::size_type separator = image_path.find_last_of(L"\\/");
    const wchar_t* filename =
        separator == std::wstring::npos ? image_path.c_str() : image_path.c_str() + separator + 1;
    if (_wcsicmp(filename, L"sublime_text.exe") != 0) return false;

    info->window = window;
    info->image_path = std::move(image_path);
    return true;
}

BOOL CALLBACK find_sublime_window_callback(HWND window, LPARAM parameter) {
    auto* info = reinterpret_cast<WindowInfo*>(parameter);
    return is_sublime_window(window, info) ? FALSE : TRUE;
}

WindowInfo find_sublime_window() {
    WindowInfo info;
    const HWND foreground = GetForegroundWindow();
    if (foreground && is_sublime_window(foreground, &info)) return info;
    EnumWindows(find_sublime_window_callback, reinterpret_cast<LPARAM>(&info));
    return info;
}

std::wstring join_path(std::wstring_view directory, std::wstring_view filename) {
    std::wstring path(directory);
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/') path.push_back(L'\\');
    path.append(filename);
    return path;
}

bool ensure_directory(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        if (attributes & FILE_ATTRIBUTE_DIRECTORY) return true;
        std::fwprintf(stderr, L"%ls exists but is not a directory\n", path.c_str());
        return false;
    }

    if (CreateDirectoryW(path.c_str(), nullptr)) return true;
    print_last_error(L"CreateDirectoryW");
    return false;
}

bool write_bgra_png(IWICImagingFactory* factory,
                    const wchar_t* path,
                    UINT width,
                    UINT height,
                    std::uint8_t* pixels,
                    DWORD row_pitch) {
    const std::uint64_t buffer_size =
        static_cast<std::uint64_t>(height - 1) * row_pitch + width * 4;
    if (buffer_size > std::numeric_limits<DWORD>::max()) {
        std::fwprintf(stderr, L"Image is too large for the WIC PNG encoder\n");
        return false;
    }

    IWICStream* stream = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IPropertyBag2* properties = nullptr;
    const wchar_t* failed_operation = nullptr;

    HRESULT result = factory->CreateStream(&stream);
    if (FAILED(result)) {
        failed_operation = L"IWICImagingFactory::CreateStream";
    } else if (FAILED(result = stream->InitializeFromFilename(path, GENERIC_WRITE))) {
        failed_operation = L"IWICStream::InitializeFromFilename";
    } else if (FAILED(result =
                          factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder))) {
        failed_operation = L"IWICImagingFactory::CreateEncoder(PNG)";
    } else if (FAILED(result = encoder->Initialize(stream, WICBitmapEncoderNoCache))) {
        failed_operation = L"IWICBitmapEncoder::Initialize";
    } else if (FAILED(result = encoder->CreateNewFrame(&frame, &properties))) {
        failed_operation = L"IWICBitmapEncoder::CreateNewFrame";
    } else if (FAILED(result = frame->Initialize(properties))) {
        failed_operation = L"IWICBitmapFrameEncode::Initialize";
    } else if (FAILED(result = frame->SetSize(width, height))) {
        failed_operation = L"IWICBitmapFrameEncode::SetSize";
    } else {
        WICPixelFormatGUID pixel_format = GUID_WICPixelFormat32bppBGRA;
        result = frame->SetPixelFormat(&pixel_format);
        if (FAILED(result) || !IsEqualGUID(pixel_format, GUID_WICPixelFormat32bppBGRA)) {
            if (SUCCEEDED(result)) result = WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
            failed_operation = L"IWICBitmapFrameEncode::SetPixelFormat";
        } else if (FAILED(result = frame->WritePixels(height, row_pitch,
                                                      static_cast<DWORD>(buffer_size), pixels))) {
            failed_operation = L"IWICBitmapFrameEncode::WritePixels";
        } else if (FAILED(result = frame->Commit())) {
            failed_operation = L"IWICBitmapFrameEncode::Commit";
        } else if (FAILED(result = encoder->Commit())) {
            failed_operation = L"IWICBitmapEncoder::Commit";
        }
    }

    if (failed_operation) print_hresult(failed_operation, result);
    release(properties);
    release(frame);
    release(encoder);
    release(stream);
    return !failed_operation;
}

bool get_client_box(HWND window, const D3D11_TEXTURE2D_DESC& source_desc, D3D11_BOX* client_box) {
    RECT window_rect{};
    RECT client_rect{};
    if (!GetWindowRect(window, &window_rect)) {
        print_last_error(L"GetWindowRect");
        return false;
    }
    if (!GetClientRect(window, &client_rect)) {
        print_last_error(L"GetClientRect");
        return false;
    }

    POINT top_left{client_rect.left, client_rect.top};
    POINT bottom_right{client_rect.right, client_rect.bottom};
    if (!ClientToScreen(window, &top_left) || !ClientToScreen(window, &bottom_right)) {
        print_last_error(L"ClientToScreen");
        return false;
    }

    const LONG window_width = window_rect.right - window_rect.left;
    const LONG window_height = window_rect.bottom - window_rect.top;
    if (window_width <= 0 || window_height <= 0 ||
        source_desc.Width > static_cast<UINT>(std::numeric_limits<int>::max()) ||
        source_desc.Height > static_cast<UINT>(std::numeric_limits<int>::max())) {
        std::fwprintf(stderr, L"Invalid window or surface dimensions\n");
        return false;
    }

    const auto map_coordinate = [](LONG offset, LONG window_extent, UINT surface_extent) -> LONG {
        if (window_extent == static_cast<LONG>(surface_extent)) return offset;
        return static_cast<LONG>(MulDiv(offset, static_cast<int>(surface_extent), window_extent));
    };
    const LONG left =
        map_coordinate(top_left.x - window_rect.left, window_width, source_desc.Width);
    const LONG top =
        map_coordinate(top_left.y - window_rect.top, window_height, source_desc.Height);
    const LONG right =
        map_coordinate(bottom_right.x - window_rect.left, window_width, source_desc.Width);
    const LONG bottom =
        map_coordinate(bottom_right.y - window_rect.top, window_height, source_desc.Height);
    if (left < 0 || top < 0 || right <= left || bottom <= top ||
        right > static_cast<LONG>(source_desc.Width) ||
        bottom > static_cast<LONG>(source_desc.Height)) {
        std::fwprintf(stderr,
                      L"Client rectangle (%ld,%ld)-(%ld,%ld) is outside the %ux%u surface\n", left,
                      top, right, bottom, source_desc.Width, source_desc.Height);
        return false;
    }

    client_box->left = static_cast<UINT>(left);
    client_box->top = static_cast<UINT>(top);
    client_box->front = 0;
    client_box->right = static_cast<UINT>(right);
    client_box->bottom = static_cast<UINT>(bottom);
    client_box->back = 1;
    return true;
}

bool save_bgra_png(const wchar_t* path,
                   const D3D11_TEXTURE2D_DESC& source_desc,
                   const D3D11_BOX& source_box,
                   ID3D11Device* device,
                   ID3D11DeviceContext* context,
                   ID3D11Texture2D* source,
                   IWICImagingFactory* wic_factory) {
    if (source_desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM &&
        source_desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB &&
        source_desc.Format != DXGI_FORMAT_B8G8R8X8_UNORM &&
        source_desc.Format != DXGI_FORMAT_B8G8R8X8_UNORM_SRGB) {
        std::fwprintf(stderr, L"Output only supports 32-bit BGRA/BGRX; DXGI format is %u\n",
                      static_cast<unsigned>(source_desc.Format));
        return false;
    }

    const UINT output_width = source_box.right - source_box.left;
    const UINT output_height = source_box.bottom - source_box.top;

    D3D11_TEXTURE2D_DESC staging_desc = source_desc;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.BindFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.MiscFlags = 0;

    ID3D11Texture2D* staging = nullptr;
    HRESULT result = device->CreateTexture2D(&staging_desc, nullptr, &staging);
    if (FAILED(result)) {
        print_hresult(L"ID3D11Device::CreateTexture2D(staging)", result);
        return false;
    }

    context->CopyResource(staging, source);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    result = context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(result)) {
        print_hresult(L"ID3D11DeviceContext::Map", result);
        release(staging);
        return false;
    }
    auto* pixels = static_cast<std::uint8_t*>(mapped.pData) +
                   static_cast<std::size_t>(source_box.top) * mapped.RowPitch +
                   static_cast<std::size_t>(source_box.left) * 4;
    const bool ok =
        write_bgra_png(wic_factory, path, output_width, output_height, pixels, mapped.RowPitch);
    context->Unmap(staging, 0);
    release(staging);
    return ok;
}

IDXGIAdapter1* find_adapter(const LUID& wanted_luid) {
    IDXGIFactory1* factory = nullptr;
    HRESULT result =
        CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));
    if (FAILED(result)) {
        print_hresult(L"CreateDXGIFactory1", result);
        return nullptr;
    }

    IDXGIAdapter1* match = nullptr;
    for (UINT index = 0;; ++index) {
        IDXGIAdapter1* adapter = nullptr;
        result = factory->EnumAdapters1(index, &adapter);
        if (result == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(result)) {
            print_hresult(L"IDXGIFactory1::EnumAdapters1", result);
            break;
        }

        DXGI_ADAPTER_DESC1 desc{};
        result = adapter->GetDesc1(&desc);
        if (SUCCEEDED(result) && same_luid(desc.AdapterLuid, wanted_luid)) {
            match = adapter;
            break;
        }
        release(adapter);
    }

    release(factory);
    return match;
}

struct SurfaceInfo {
    HANDLE shared_handle = nullptr;
    LUID adapter_luid{};
    ULONGLONG update_id = 0;
};

class DwmCapture {
public:
    ~DwmCapture() {
        release(wic_factory_);
        release(context_);
        release(device_);
        if (user32_) FreeLibrary(user32_);
    }

    bool initialize() {
        user32_ = LoadLibraryW(L"user32.dll");
        if (!user32_) {
            print_last_error(L"LoadLibraryW(user32.dll)");
            return false;
        }
        get_surface_ = reinterpret_cast<DwmGetDxSharedSurfaceFn>(
            GetProcAddress(user32_, "DwmGetDxSharedSurface"));
        if (!get_surface_) {
            std::fwprintf(stderr, L"This Windows build does not export DwmGetDxSharedSurface\n");
            return false;
        }
        const HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                                CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory),
                                                reinterpret_cast<void**>(&wic_factory_));
        if (FAILED(result)) {
            print_hresult(L"CoCreateInstance(CLSID_WICImagingFactory)", result);
            return false;
        }
        return true;
    }

    bool query(HWND window, SurfaceInfo* surface, bool report_error) const {
        ULONG window_format = 0;
        ULONG present_flags = 0;
        SetLastError(ERROR_SUCCESS);
        const BOOL got_surface =
            get_surface_(window, &surface->shared_handle, &surface->adapter_luid, &window_format,
                         &present_flags, &surface->update_id);
        if (got_surface && surface->shared_handle) return true;
        if (report_error) print_last_error(L"DwmGetDxSharedSurface");
        return false;
    }

    bool save(HWND window, const wchar_t* output_path) {
        SurfaceInfo surface;
        if (!query(window, &surface, true)) return false;
        if (!ensure_device(surface.adapter_luid)) return false;

        ID3D11Texture2D* texture = nullptr;
        HRESULT result = device_->OpenSharedResource(
            surface.shared_handle, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture));
        if (FAILED(result)) {
            print_hresult(L"ID3D11Device::OpenSharedResource", result);
            return false;
        }

        D3D11_TEXTURE2D_DESC desc{};
        texture->GetDesc(&desc);
        D3D11_BOX client_box{};
        if (!get_client_box(window, desc, &client_box)) {
            release(texture);
            return false;
        }
        client_box.right = std::min(client_box.right, client_box.left + kCaptureWidth);
        client_box.bottom = std::min(client_box.bottom, client_box.top + kCaptureHeight);

        const bool saved =
            save_bgra_png(output_path, desc, client_box, device_, context_, texture, wic_factory_);
        release(texture);
        return saved;
    }

private:
    bool ensure_device(const LUID& adapter_luid) {
        if (device_ && same_luid(device_luid_, adapter_luid)) return true;

        release(context_);
        release(device_);
        IDXGIAdapter1* adapter = find_adapter(adapter_luid);
        if (!adapter) return false;

        const HRESULT result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                                                 D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                                                 D3D11_SDK_VERSION, &device_, nullptr, &context_);
        release(adapter);
        if (FAILED(result)) {
            print_hresult(L"D3D11CreateDevice", result);
            return false;
        }
        device_luid_ = adapter_luid;
        return true;
    }

    HMODULE user32_ = nullptr;
    DwmGetDxSharedSurfaceFn get_surface_ = nullptr;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    LUID device_luid_{};
    IWICImagingFactory* wic_factory_ = nullptr;
};

std::wstring quote_command_line_argument(std::wstring_view argument) {
    const bool needs_quotes =
        argument.empty() || argument.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
    if (!needs_quotes) return std::wstring(argument);

    std::wstring quoted(1, L'"');
    std::size_t backslashes = 0;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
        } else if (character == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(L'"');
            backslashes = 0;
        } else {
            quoted.append(backslashes, L'\\');
            backslashes = 0;
            quoted.push_back(character);
        }
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

bool launch_sublime_command(const std::wstring& subl_path, const std::wstring& command) {
    std::wstring command_line = quote_command_line_argument(subl_path);
    for (const std::wstring_view argument :
         {std::wstring_view(L"--background"), std::wstring_view(L"--command"),
          std::wstring_view(command)}) {
        command_line.push_back(L' ');
        command_line += quote_command_line_argument(argument);
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(subl_path.c_str(), command_line.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        print_last_error(L"CreateProcessW(subl.exe)");
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

bool wait_for_settled_frame(DwmCapture* capture, HWND window, ULONGLONG initial_update_id) {
    const ULONGLONG started_at = GetTickCount64();
    ULONGLONG last_change_at = started_at;
    ULONGLONG last_update_id = initial_update_id;
    bool saw_change = false;

    while (GetTickCount64() - started_at < kFrameTimeoutMs) {
        SurfaceInfo surface;
        if (!capture->query(window, &surface, false)) {
            Sleep(kFramePollMs);
            continue;
        }

        const ULONGLONG now = GetTickCount64();
        if (surface.update_id != last_update_id) {
            saw_change = true;
            last_update_id = surface.update_id;
            last_change_at = now;
        } else if (saw_change && now - last_change_at >= kFrameQuietMs) {
            return true;
        }
        Sleep(kFramePollMs);
    }

    std::fwprintf(stderr,
                  L"Timed out waiting for the Sublime Text surface to change and settle\n");
    return false;
}

std::wstring json_escape(std::wstring_view value) {
    std::wstring escaped;
    for (const wchar_t character : value) {
        switch (character) {
        case L'\\':
            escaped += L"\\\\";
            break;
        case L'"':
            escaped += L"\\\"";
            break;
        case L'\n':
            escaped += L"\\n";
            break;
        case L'\r':
            escaped += L"\\r";
            break;
        case L'\t':
            escaped += L"\\t";
            break;
        default:
            escaped.push_back(character);
            break;
        }
    }
    return escaped;
}

bool read_utf8_file(const std::wstring& path, std::wstring* text) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        std::fwprintf(stderr, L"Could not open %ls\n", path.c_str());
        print_last_error(L"CreateFileW");
        return false;
    }

    LARGE_INTEGER file_size{};
    if (!GetFileSizeEx(file, &file_size) || file_size.QuadPart < 0 ||
        file_size.QuadPart > std::numeric_limits<DWORD>::max()) {
        print_last_error(L"GetFileSizeEx");
        CloseHandle(file);
        return false;
    }

    std::string utf8(static_cast<std::size_t>(file_size.QuadPart), '\0');
    DWORD bytes_read = 0;
    const bool read = utf8.empty() || (ReadFile(file, utf8.data(), static_cast<DWORD>(utf8.size()),
                                                &bytes_read, nullptr) &&
                                       bytes_read == utf8.size());
    CloseHandle(file);
    if (!read) {
        print_last_error(L"ReadFile");
        return false;
    }

    std::size_t offset = 0;
    if (utf8.size() >= 3 && static_cast<unsigned char>(utf8[0]) == 0xef &&
        static_cast<unsigned char>(utf8[1]) == 0xbb &&
        static_cast<unsigned char>(utf8[2]) == 0xbf) {
        offset = 3;
    }
    const std::size_t byte_count = utf8.size() - offset;
    if (byte_count > static_cast<std::size_t>(std::numeric_limits<int>::max())) return false;
    if (byte_count == 0) {
        text->clear();
        return true;
    }

    const int wide_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data() + offset,
                                              static_cast<int>(byte_count), nullptr, 0);
    if (wide_size <= 0) {
        print_last_error(L"MultiByteToWideChar");
        return false;
    }
    text->resize(static_cast<std::size_t>(wide_size));
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data() + offset,
                            static_cast<int>(byte_count), text->data(), wide_size) != wide_size) {
        print_last_error(L"MultiByteToWideChar");
        return false;
    }
    return true;
}

bool read_config(const std::wstring& path, std::vector<std::wstring>* values) {
    std::wstring text;
    if (!read_utf8_file(path, &text)) return false;

    std::size_t position = 0;
    while (position <= text.size()) {
        const std::size_t newline = text.find(L'\n', position);
        const std::size_t end = newline == std::wstring::npos ? text.size() : newline;
        std::wstring value = text.substr(position, end - position);
        const std::size_t first = value.find_first_not_of(L" \t\r");
        if (first != std::wstring::npos) {
            const std::size_t last = value.find_last_not_of(L" \t\r");
            value = value.substr(first, last - first + 1);
            if (!value.empty() && value.front() != L'#') values->push_back(std::move(value));
        }
        if (newline == std::wstring::npos) break;
        position = newline + 1;
    }
    if (!values->empty()) return true;
    std::fwprintf(stderr, L"No values found in %ls\n", path.c_str());
    return false;
}

struct TextInput {
    std::wstring stem;
    std::wstring path;
};

bool find_text_inputs(const std::wstring& directory, std::vector<TextInput>* inputs) {
    const std::wstring pattern = join_path(directory, L"*.txt");
    WIN32_FIND_DATAW data{};
    HANDLE search = FindFirstFileW(pattern.c_str(), &data);
    if (search == INVALID_HANDLE_VALUE) {
        std::fwprintf(stderr, L"No text files found under %ls\n", directory.c_str());
        return false;
    }

    do {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring filename(data.cFileName);
            const std::wstring::size_type separator = filename.find_last_of(L'.');
            const std::wstring stem =
                separator == std::wstring::npos ? filename : filename.substr(0, separator);
            inputs->push_back({stem, join_path(directory, filename)});
        }
    } while (FindNextFileW(search, &data));
    const DWORD find_error = GetLastError();
    FindClose(search);
    if (find_error != ERROR_NO_MORE_FILES) {
        SetLastError(find_error);
        print_last_error(L"FindNextFileW");
        return false;
    }

    std::sort(inputs->begin(), inputs->end(),
              [](const TextInput& lhs, const TextInput& rhs) { return lhs.stem < rhs.stem; });
    return !inputs->empty();
}

bool find_subl_path(const WindowInfo& window, std::wstring* subl_path) {
    const std::wstring::size_type separator = window.image_path.find_last_of(L"\\/");
    if (separator == std::wstring::npos) {
        std::fwprintf(stderr, L"Could not derive subl.exe from %ls\n", window.image_path.c_str());
        return false;
    }
    *subl_path = window.image_path.substr(0, separator + 1) + L"subl.exe";
    if (GetFileAttributesW(subl_path->c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::fwprintf(stderr, L"subl.exe was not found at %ls\n", subl_path->c_str());
        return false;
    }
    return true;
}

bool run_render_case(DwmCapture* capture,
                     const WindowInfo& window,
                     const std::wstring& subl_path,
                     const std::wstring& input_path,
                     const std::wstring& face,
                     const std::wstring& size,
                     const std::wstring& output_path,
                     std::size_t current,
                     std::size_t total) {
    SurfaceInfo before;
    if (!capture->query(window.window, &before, true)) return false;

    std::wstring command = L"rasterizer_render {\"text_path\":\"";
    command += json_escape(input_path);
    command += L"\",\"face\":\"";
    command += json_escape(face);
    command += L"\",\"size\":";
    command += size;
    command += L"}";

    if (!launch_sublime_command(subl_path, command)) return false;

    if (!wait_for_settled_frame(capture, window.window, before.update_id)) {
        return false;
    }

    const bool ok = capture->save(window.window, output_path.c_str());
    std::wprintf(L"[%zu/%zu] %ls%ls\n", current, total, output_path.c_str(),
                 ok ? L"" : L"  (FAILED)");
    return ok;
}

int run_tests(DwmCapture* capture,
              const WindowInfo& window,
              const std::wstring& test_directory,
              const std::wstring& output_directory) {
    std::vector<std::wstring> faces;
    std::vector<std::wstring> sizes;
    std::vector<TextInput> texts;
    if (!read_config(join_path(test_directory, L"faces-win.txt"), &faces) ||
        !read_config(join_path(test_directory, L"sizes.txt"), &sizes) ||
        !find_text_inputs(join_path(test_directory, L"texts"), &texts)) {
        return 1;
    }
    if (!ensure_directory(output_directory)) return 1;

    std::wstring subl_path;
    if (!find_subl_path(window, &subl_path)) return 1;
    const std::size_t total = texts.size() * faces.size() * sizes.size();
    std::wprintf(L"capturing %zu shots -> %ls\n", total, output_directory.c_str());

    std::size_t current = 0;
    for (const TextInput& text : texts) {
        for (const std::wstring& face : faces) {
            for (const std::wstring& size : sizes) {
                ++current;
                const std::wstring label = text.stem + L"-" + face + L"-" + size;
                const std::wstring output_path = join_path(output_directory, label + L".png");
                if (!run_render_case(capture, window, subl_path, text.path, face, size,
                                     output_path, current, total)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    std::setbuf(stdout, nullptr);
    if (argc != 3) {
        std::fwprintf(stderr, L"usage: capture_windows <tests_dir> <out_dir>\n");
        return 2;
    }
    if (!SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        print_last_error(L"SetThreadDpiAwarenessContext");
        return 1;
    }

    const WindowInfo window = find_sublime_window();
    if (!window.window) {
        std::fwprintf(stderr, L"No visible top-level Sublime Text window was found\n");
        return 1;
    }

    const HRESULT com_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
        print_hresult(L"CoInitializeEx", com_result);
        return 1;
    }

    int result = 1;
    {
        DwmCapture capture;
        if (capture.initialize()) {
            result = run_tests(&capture, window, argv[1], argv[2]);
        }
    }
    if (SUCCEEDED(com_result)) CoUninitialize();
    return result;
}
