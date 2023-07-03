// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <engine/core/types.h>
#include <engine/platform/platform_system.h>

auto static constexpr WINDOW_TITLE = "Prototype";
auto static constexpr WINDOW_WIDTH = 1280;
auto static constexpr WINDOW_HEIGHT = 720;
auto running = true;

#if defined(PLATFORM_MACOS)

#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <objc/objc-runtime.h>
#include <objc/NSObjCRuntime.h>
#include <CoreGraphics/CoreGraphics.h>


// Dummy stack protector
extern "C" void __stack_chk_fail() {}
extern "C" void __stack_chk_guard() {}

template<typename T> auto get_class(const char* className) -> T { return reinterpret_cast<T>(objc_getClass(className)); }
template<typename R, typename... Args> auto send(id obj, const char* selector, Args... args) { return reinterpret_cast<R(*)(id, SEL, Args...)>(objc_msgSend)(obj, sel_getUid(selector), args...); }

extern id NSApp;
extern id NSDefaultRunLoopMode;

id static window;
id metalLayer;
objc_class* windowDelegate;

auto windowWillClose(id, SEL, id) -> void { running = false; }


auto create_metal_layer() -> void {
    metalLayer = send<id>(get_class<id>("CAMetalLayer"), "new");
    send<void>(metalLayer, "setDevice:", send<id>(get_class<id>("MTLCreateSystemDefaultDevice"), "new"));
    send<void>(metalLayer, "setPixelFormat:", 70); // MTLPixelFormatBGRA8Unorm
    send<void>(metalLayer, "setFrame:", CGRectMake(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));

    auto contentView = send<id>(window, "contentView");
    send<void>(contentView, "setWantsLayer:", YES);
    send<void>(contentView, "setLayer:", metalLayer);
}

namespace xc::platform {
    auto initialize() -> bool {
        NSApp = send<id>(get_class<id>("NSApplication"), "sharedApplication");
        send<void>(NSApp, "setActivationPolicy:", 0);
        send<void>(NSApp, "activateIgnoringOtherApps:", YES);

        windowDelegate = objc_allocateClassPair(objc_getClass("NSResponder"), "WindowDelegate", 0);
        class_addMethod(windowDelegate, sel_registerName("windowWillClose:"), reinterpret_cast<IMP>(windowWillClose), "v@:@");
        objc_registerClassPair(windowDelegate);

        window = send<id>(get_class<id>("NSWindow"), "alloc");
        send<void>(window, "initWithContentRect:styleMask:backing:defer:", CGRect{{0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}}, 15, 2, 0);
        send<void>(window, "setTitle:", send<id>(get_class<id>("NSString"), "stringWithUTF8String:", WINDOW_TITLE));
        send<void>(window, "center");
        send<void>(window, "setDelegate:", send<id>(get_class<id>("WindowDelegate"), "new"));
        send<void>(window, "makeKeyAndOrderFront:", nil);

        create_metal_layer();

        print("Platform initialization successful\n");
        return true;
    }

    auto uninitialize() -> void {
        objc_disposeClassPair(windowDelegate);
        send<void>(metalLayer, "release");
        send<void>(window, "release");
    }

    auto tick() -> void {
        send<void>(send<id>(window, "contentView"), "setNeedsDisplay:", YES);
        auto event = send<id>(NSApp, "nextEventMatchingMask:untilDate:inMode:dequeue:", ULONG_MAX, nil, NSDefaultRunLoopMode, YES);

        switch (send<NSUInteger>(event, "type")) {
            case 10:
                auto key = send<NSUInteger>(event, "keyCode");
                print("key down: %i", key); break;
        }

        send<void>(NSApp, "sendEvent:", event);
    }

    auto exit(int const code) -> void {
        __asm__ volatile (
                "mov $0x2000001, %%eax\n"   // System call number for exit
                "mov %[code], %%edi\n"      // Move the 'code' parameter into %edi
                "syscall\n"                 // Invoke the system call
                :
                : [code] "r"(code)          // Input constraint to specify 'code' as an input operand
        : "%eax", "%edi"            // Clobbered registers
        );
    }

    auto load_library(char const* name) -> void* {
        auto l = dlopen(name, RTLD_NOW | RTLD_LOCAL);
        if (!l) print("Failed to load: %s\n", name); return l;
    }
    auto unload_library(void* library) -> void { dlclose(library); }

    auto load_function(void* library, char const* name) -> void* {
        auto f = dlsym(library, name);
        if (!f) print("Failed to load: %s\n", name);
        return f;
    };
}

auto malloc(size_t size) -> void * {
    mach_vm_address_t address = 0;
    return (mach_vm_allocate(mach_task_self(), &address, size, VM_FLAGS_ANYWHERE) == KERN_SUCCESS)
           ? reinterpret_cast<void *>(address) : nullptr;
}

auto free(void *ptr) -> void { mach_vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(ptr), 0); }

auto memcpy(void *dest, const void *src, size_t size) -> void* {
    mach_vm_copy(mach_task_self(), reinterpret_cast<vm_address_t>(src), size, reinterpret_cast<vm_address_t>(dest));
    return dest;
}


auto memset(void *ptr, int value, size_t size) -> void * {
    return mach_vm_protect(mach_task_self(), reinterpret_cast<mach_vm_address_t>(ptr), size, 0,
                           VM_PROT_READ | VM_PROT_WRITE) == KERN_SUCCESS &&
           mach_vm_write(mach_task_self(), reinterpret_cast<mach_vm_address_t>(ptr), static_cast<vm_offset_t>(value),
                         static_cast<mach_msg_type_number_t>(size)) == KERN_SUCCESS &&
           mach_vm_protect(mach_task_self(), reinterpret_cast<mach_vm_address_t>(ptr), size, 0,
                           VM_PROT_READ | VM_PROT_COPY) == KERN_SUCCESS ? ptr : nullptr;
}

// TODO: vsnprintf is working because the standard library is getting linked
auto print(const char* message, ...) -> void {
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
}

#endif // PLATFORM_MACOS


#if defined(PLATFORM_WINDOWS)
#include <Windows.h>

static HWND window;
static HDC hdc;

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
        auto const wc = WNDCLASS{{}, events, {}, {}, GetModuleHandle({}), {}, {}, {}, {}, "win"};
        RegisterClass(&wc);

        window = CreateWindow("win", "", WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, {}, {}, {}, {});
        if (!window) return false;

        hdc = GetDC(window);

        ShowWindow(window, SW_NORMAL);
        return true;
    }

    auto uninitialize() -> void {}

    auto tick() -> bool {
        auto msg = MSG{};
        while (PeekMessage(&msg, {}, {}, {}, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return true;
    }

    auto window_handle() -> void* { return hdc; }

    auto exit(int const code) -> void { ExitProcess(code); }

    auto print(char const* message, ...) -> void {
        char out_message[32000];
        memset(out_message, 0, 32000);
        va_list arg_ptr;
        va_start(arg_ptr, message);
        va_end(arg_ptr);

        DWORD bytes_written = 0u;
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), message, strlen(message), &bytes_written, nullptr);
    }
}

extern "C" {
    auto malloc(size_t size) -> void* {
        malloc_size += size;
        return HeapAlloc(GetProcessHeap(), 0, size);
    }

    auto free(void* ptr) noexcept -> void {
        malloc_size -= HeapSize(GetProcessHeap(), 0, ptr);
        HeapFree(GetProcessHeap(), 0, ptr);
    }

    auto free(void* ptr, size_t size) noexcept -> void {
        malloc_size -= size;
        HeapFree(GetProcessHeap(), 0, ptr);
    }

    #pragma function(memset)
    auto memset(void *dest, int c, size_t count) -> void*
    {
        char *bytes = (char *)dest;
        while (count--)
        {
            *bytes++ = (char)c;
        }
        return dest;
    }

    #pragma function(memcpy)
    auto memcpy(void *dest, const void *src, size_t count) -> void*
    {
        char *dest8 = (char *)dest;
        const char *src8 = (const char *)src;
        while (count--)
        {
            *dest8++ = *src8++;
        }
        return dest;
    }
}

#endif // PLATFORM_WINDOWS
