#ifndef ENGINE_CORE_STRING_H
#define ENGINE_CORE_STRING_H

#include <engine/core/types.h>

namespace xc {
    template<typename T = char, typename Allocator = xc::default_allocator<T>> class string_t {
    public:
        string_t() = default;

        explicit string_t(const T* str) {
            _size = strlen(str);
            _data = allocate(_size);
            copy(str, _size);
        }

        template <std::size_t N> explicit string_t(const T (&str)[N]) {
            _size = N - 1;
            _data = allocate(_size);
            copy(str, _size);
        }

        string_t(const string_t &other) {
            _data = allocate(other.size());
            copy(other.c_str(), other.size());
            _size = other.size();
        }

        ~string_t() {
            clear();
            deallocate(_data);
        }


        explicit operator uint64_t() const { return static_cast<uint64_t>(wyhash(c_str(), size())); }

        auto operator!=(const string_t& other) const -> bool { return *this != other; }
        auto operator==(const string_t& other) const -> bool {
            return (_size == other._size) && (memcmp(_data, other._data, _size * sizeof(T)) == 0);
        }
        auto operator=(const string_t &other) -> string_t& {
            if (this != &other) {
                clear();
                deallocate(_data);
                _data = allocate(other.size());
                copy(other.c_str(), other.size());
                _size = other.size();
            }
            return *this;
        }

        [[nodiscard]] auto size() const -> size_t { return _size; }
        [[nodiscard]] auto c_str() const -> T const * { return _data; }

    private:
        T *_data = {};
        size_t _size = {};
        Allocator _allocator = {};

        auto allocate(size_t n) -> T * { return _allocator.allocate(n); }
        auto deallocate(T *ptr) -> void { _allocator.deallocate(ptr); }

        auto copy(const T *source, std::size_t length) -> void {
            memcpy(_data, source, length * sizeof(T));
            _data[length] = '\0';
        }

        auto clear() -> void {
            memset(_data, 0, _size * sizeof(T));
            _size = 0;
        }

        auto strlen(const char *str) -> size_t {
            size_t length = 0;
            while (str[length] != '\0') ++length;
            return length;
        }
    };

    using string = string_t<char>;
}

#endif
