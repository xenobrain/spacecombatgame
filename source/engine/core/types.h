#ifndef ENGINE_CORE_CORE_TYPES_H
#define ENGINE_CORE_CORE_TYPES_H

#include <cstddef>
#include <cstdint>

#define count_of(x) (sizeof(x) / sizeof(x[0]))
#define move(x) static_cast<decltype(x)&&>(x)

#if defined(_WIN32) && defined(_MSC_VER)
#define CDECL __cdecl
#ifdef DEBUG
        #define DEBUG_BREAK __debugbreak()
    #else
        #define DEBUG_BREAK do {} while (0)
    #endif
#elif defined(__APPLE__)
#define CDECL
#ifdef DEBUG
#define DEBUG_BREAK __builtin_debugtrap()
#else
#define DEBUG_BREAK do {} while (0)
#endif
#elif defined(PLATFORM_LINUX)
#define CDECL

#include <signal.h>
    #ifdef DEBUG
        #define DEBUG_BREAK raise(SIGTRAP)
    #else
        #define DEBUG_BREAK do {} while (0)
    #endif
#else
#define CDECL
#endif


extern "C" {
    auto extern CDECL memcpy(void *dest, const void *src, size_t size) -> void*;
    auto extern CDECL memset(void *ptr, int value, size_t size) -> void*;
    auto extern CDECL malloc(size_t size) -> void*;
    auto extern CDECL free(void *ptr) -> void;
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
    while (*str) {
        seed ^= static_cast<uint64_t>(*str++);
        seed *= 0xe7037ed1a0b428dbull;
        seed +=  0xa0761d6478bd642full;
    }

    seed ^= seed >> 47;
    seed *= 0x8ebc6af09c88c6e3ull;
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

    // Delegate
    template <typename T> class delegate;

    template <typename R, typename... Args> class delegate<R(Args...)> {
    public:
        template <R(*function)(Args...)> auto bind(void) -> void {
            _instance = nullptr;
            _proxy = &function_proxy<function>;
        }

        template <class C, R(C::* function)(Args...)> auto bind(C* instance) -> void {
            _instance = instance;
            _proxy = &method_proxy<C, function>;
        }

        template <class C, R(C::* function)(Args...) const> auto bind(const C* instance) -> void {
            _instance = const_cast<C*>(instance);
            _proxy = &const_method_proxy<C, function>;
        }

        auto invoke(Args... args) -> R { return _proxy(_instance, static_cast<Args&&>(args)...); }

    private:
        typedef R(*proxy_function)(void*, Args...);

        template <R(*function)(Args...)> auto static function_proxy(void*, Args... args) -> R {
            return function(static_cast<Args&&>(args)...);
        }
        template <class C, R(C::* function)(Args...)> auto static method_proxy(void* instance, Args... args) -> R {
            return (static_cast<C*>(instance)->*function)(static_cast<Args&&>(args)...);
        }
        template <class C, R(C::* function)(Args...) const> auto static const_method_proxy(void* instance, Args... args) -> R {
            return (static_cast<const C*>(instance)->*function)(static_cast<Args&&>(args)...);
        }

        void* _instance = nullptr;
        proxy_function _proxy = nullptr;
    };


    // Math types
    template<typename T, int N> struct vector;
    template<typename T> struct vector<T,1> { T x; };
    template<typename T> struct vector<T,2> { T x, y; };
    template<typename T> struct vector<T,3> { T x, y, z; };
    template<typename T> struct vector<T,4> { T x, y, z, w; };

    template<typename T, int M, int N> struct matrix;
    template<typename T, int M> struct matrix<T,M,1> { vector<T,M> x; };
    template<typename T, int M> struct matrix<T,M,2> { vector<T,M> x, y; };
    template<typename T, int M> struct matrix<T,M,3> { vector<T,M> x, y, z; };
    template<typename T, int M> struct matrix<T,M,4> { vector<T,M> x, y, z, w; };

    using vector2 = vector<float,2>;
    using vector3 = vector<float,3>;
    using vector4 = vector<float,4>;
    using matrix2 = matrix<float,2,2>;
    using matrix3 = matrix<float,3,3>;
    using matrix4 = matrix<float,4,4>;
}

#endif // ENGINE_CORE_CORE_TYPES_H