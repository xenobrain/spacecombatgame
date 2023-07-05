#include <engine/platform/platform_system.h>

#include <Windows.h>

HINSTANCE hinstance;
HDC hdc;

static HWND window;

bool running = true;

auto static width = 1280;
auto static height = 720;

extern "C" auto _fltused = 0x9875;

auto static CALLBACK events(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param) -> LRESULT {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_KEYDOWN:
            break;
        default:
            break;
    }

    return DefWindowProc(h_wnd, msg, w_param, l_param);
}

namespace xc::platform {
    auto initialize() -> bool {
        hinstance = GetModuleHandle({});
        auto const wc = WNDCLASS{{}, events, {}, {}, hinstance, {}, {}, {}, {}, "win"};
        RegisterClass(&wc);

        window = CreateWindow("win", "", WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, {}, {}, {}, {});
        if (!window) return false;

        hdc = GetDC(window);

        ShowWindow(window, SW_NORMAL);

        print("Platform initialization successful\n");

        return true;
    }

    auto uninitialize() -> void {}

    auto tick() -> void {
        auto msg = MSG{};
        while (PeekMessage(&msg, {}, {}, {}, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    auto exit(int const code) -> void { ExitProcess(code); }

    auto load_library(char const* name) -> void* {
        auto library = LoadLibrary(name);
        if (!library) print("Failed to load library: %s\n", name);
        return library;
    }

    auto unload_library(void* library) -> void { FreeLibrary(reinterpret_cast<HMODULE>(library)); }

    auto load_function(void* library, char const* name) -> void* {
        auto function = GetProcAddress(reinterpret_cast<HMODULE>(library), name);
        if (!function) print("Failed to load function: %s\n", name);
        return function;
    }
}


auto print(const char* message, ...) -> void {
    char buffer[256];
    size_t buffer_size = sizeof(buffer);

    va_list arg_ptr;
            va_start(arg_ptr, message);

    int formatted_length = wvsprintf(buffer, message, arg_ptr);

    if (static_cast<size_t>(formatted_length) >= buffer_size) {
        buffer_size *= 2;
        char* out_message = static_cast<char*>(malloc(buffer_size));
        if (out_message) {
            formatted_length = wvsprintfA(out_message, message, arg_ptr);
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), out_message, formatted_length, nullptr, nullptr);
            free(out_message);
        }
    } else {
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, formatted_length, nullptr, nullptr);
    }

            va_end(arg_ptr);
}

extern "C" {
auto __cdecl malloc(size_t size) -> void* { return HeapAlloc(GetProcessHeap(), 0, size); }
auto __cdecl free(void* ptr) -> void { HeapFree(GetProcessHeap(), 0, ptr); }

#pragma function(memset)
auto __cdecl memset(void *dest, int c, size_t count) -> void* {
    char *bytes = (char *)dest;
    while (count--)
    {
        *bytes++ = (char)c;
    }
    return dest;
}

#pragma function(memcpy)
auto __cdecl memcpy(void *dest, const void *src, size_t count) -> void* {
    char *dest8 = (char *)dest;
    const char *src8 = (const char *)src;
    while (count--)
    {
        *dest8++ = *src8++;
    }
    return dest;
}
}