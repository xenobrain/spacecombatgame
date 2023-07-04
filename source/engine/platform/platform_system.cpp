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

#if defined(PLATFORM_LINUX)

#include <X11/Xlib.h>

extern "C" void __stack_chk_fail() {}
extern "C" void __stack_chk_guard() {}

namespace xc::platform {
    auto initialize() -> bool {
        Display *display = XOpenDisplay(nullptr);
        if (!display) {
            //std::cerr << "Error opening display" << std::endl;
            return 1;
        }

        // Get the default screen
        int screen = DefaultScreen(display);

        // Create the window
        Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 640, 480, 0,
                                            BlackPixel(display, screen), WhitePixel(display, screen));

        // Set window properties
        XSelectInput(display, window, ExposureMask | KeyPressMask);
        XMapWindow(display, window);

        return true;
    }

    auto uninitialize() -> void {}

    auto tick() -> void {}

    auto exit(int code) -> void {}

    auto load_library(char const *name) -> void * {}

    auto unload_library(void *library) -> void {}

    auto load_function(void *library, char const *name) -> void * {}
}

extern "C" {
#include <sys/mman.h>
#include <linux/mman.h>

#define MIN_BLOCK_SIZE 4096 // Minimum block size for allocation

typedef struct block {
    size_t size;
    struct block *next;
} Block;

Block *head = NULL; // Head of the linked list of allocated blocks

void *malloc(size_t size) {
    // Ensure size is a multiple of the system page size
    size_t aligned_size = (size + MIN_BLOCK_SIZE - 1) & ~(MIN_BLOCK_SIZE - 1);

    // Allocate memory using mmap system call
    void *memory;
    asm volatile (
            "mov $9, %%rax\n"             // mmap system call number
            "xor %%rdi, %%rdi\n"          // addr (NULL)
            "mov %1, %%rsi\n"             // length
            "mov %2, %%rdx\n"             // prot (PROT_READ | PROT_WRITE)
            "mov $0x22, %%r10\n"          // flags (MAP_PRIVATE | MAP_ANONYMOUS)
            "xor %%r8, %%r8\n"            // fd (zeroed)
            "xor %%r9, %%r9\n"            // offset (zeroed)
            "syscall\n"                   // invoke the system call
            "mov %%rax, %0\n"             // save the result
            : "=r"(memory)
            : "r"(aligned_size), "r"((unsigned long) (PROT_READ | PROT_WRITE))
            : "%rax", "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9"
            );

    if (memory == MAP_FAILED)
        return NULL;

    // Create a new block and add it to the list
    Block *new_block = (Block *) memory;
    new_block->size = aligned_size;
    new_block->next = head;
    head = new_block;

    // Return the memory region after the block struct
    return (void *) (new_block + 1);
}

void free(void *ptr) {
    if (ptr == NULL)
        return;

    // Find the block in the list
    Block *current_block = head;
    Block *previous_block = NULL;
    while (current_block != NULL && (void *) (current_block + 1) != ptr) {
        previous_block = current_block;
        current_block = current_block->next;
    }

    // If the block was found, remove it from the list and deallocate memory using munmap
    if (current_block != NULL) {
        if (previous_block != NULL)
            previous_block->next = current_block->next;
        else
            head = current_block->next;

        // Deallocate memory using munmap system call
        asm volatile (
                "movq $11, %%rax\n"       // munmap system call number
                "movq %0, %%rdi\n"        // address
                "movq %1, %%rsi\n"        // size
                "xor %%rdx, %%rdx\n"      // flags (zeroed)
                "syscall\n"               // invoke the system call
                :
                : "r"(current_block), "r"(current_block->size)
                : "%rax", "%rdi", "%rsi", "%rdx"
                );
    }
}

auto memset(void *s, int c, size_t n) -> void* {
    auto *ptr = reinterpret_cast<unsigned char*>(s);
    auto value = static_cast<unsigned char>(c);

    for (size_t i = 0; i < n; i++) {
        *ptr = value;
        ptr++;
    }

    return s;
}

auto memcpy(void *dest, const void *src, size_t n) -> void* {
    auto *dst_ptr = reinterpret_cast<unsigned char*>(dest);
    auto *src_ptr = reinterpret_cast<const unsigned char*>(src);

    for (size_t i = 0; i < n; i++) {
        *dst_ptr = *src_ptr;
        dst_ptr++;
        src_ptr++;
    }

    return dest;
}

}

auto print(const char *format, ...) -> void {}

#endif

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>

HINSTANCE hinstance;
HDC hdc;

static HWND window;


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

#endif // PLATFORM_WINDOWS
