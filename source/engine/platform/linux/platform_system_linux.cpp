#include <engine/platform/platform_system.h>


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