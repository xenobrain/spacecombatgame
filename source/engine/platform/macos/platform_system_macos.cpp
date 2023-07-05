#include <engine/platform/platform_system.h>

#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <objc/objc-runtime.h>
#include <objc/NSObjCRuntime.h>
#include <CoreGraphics/CoreGraphics.h>

auto static constexpr WINDOW_TITLE = "Prototype";
auto static constexpr WINDOW_WIDTH = 1280;
auto static constexpr WINDOW_HEIGHT = 720;


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
