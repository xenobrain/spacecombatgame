#ifndef ENGINE_CORE_CORE_TYPES_H
#define ENGINE_CORE_CORE_TYPES_H

#include <cstddef>
#include <cstdint>

#define count_of(x) (sizeof(x) / sizeof(x[0]))
#define move(x) static_cast<decltype(x)&&>(x)

#if defined(_WIN32) && defined(_MSC_VER)
#ifdef DEBUG
        #define DEBUG_BREAK __debugbreak()
    #else
        #define DEBUG_BREAK do {} while (0)
    #endif
#elif defined(__APPLE__)
#ifdef DEBUG
#define DEBUG_BREAK __builtin_debugtrap()
#else
#define DEBUG_BREAK do {} while (0)
#endif
#elif defined(__linux__)
#include <signal.h>
    #ifdef DEBUG
        #define DEBUG_BREAK raise(SIGTRAP)
    #else
        #define DEBUG_BREAK do {} while (0)
    #endif
#endif


extern "C" {
    auto extern __cdecl memcpy(void *dest, const void *src, size_t size) -> void*;
    auto extern __cdecl memset(void *ptr, int value, size_t size) -> void*;
    auto extern __cdecl malloc(size_t size) -> void*;
    auto extern __cdecl free(void *ptr) -> void;
}

constexpr auto wyhash(uint64_t key)-> uint64_t {
    key += 0x60bee2bee120fc15;
    key ^= key >> 33;
    key *= 0x2545f4914f6cdd1d;
    key ^= key >> 29;
    key *= 0x85ebca6b6f2bafc5;
    key ^= key >> 32;
    return key;
}

auto constexpr wyhash(const char* str, uint64_t seed = 0) -> uint64_t {
    auto constexpr wyp0 = 0xa0761d6478bd642full;
    auto constexpr wyp1 = 0xe7037ed1a0b428dbull;
    auto constexpr wyp2 = 0x8ebc6af09c88c6e3ull;

    while (*str) {
        seed ^= static_cast<uint64_t>(*str++);
        seed *= wyp1;
        seed += wyp0;
    }

    seed ^= seed >> 47;
    seed *= wyp2;
    seed ^= seed >> 47;

    return seed;
}

namespace xc {
    template<typename T> class default_allocator {
    public:
        auto allocate(size_t n) -> T* { return static_cast<T*>(malloc(n * sizeof(T))); }
        auto deallocate(T* ptr) -> void { free(ptr); }

        template<typename... Args> auto construct(T* ptr, Args&&... args) -> void { *ptr = T(args...); }
        auto destroy(T* ptr) -> void { ptr->~T(); }
    };
}

#endif