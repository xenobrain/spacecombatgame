#ifndef ENGINE_CORE_DYNAMIC_ARRAY_H
#define ENGINE_CORE_DYNAMIC_ARRAY_H

#include <engine/core/types.h>

namespace xc {
    template<typename T, typename Allocator = default_allocator<T>> class array {
    public:
        template<typename... Args> explicit array(Args&&... args) { push_back(move(args)...); }
        ~array() { clear(); }

        template<typename... Args> auto push_back(Args&&... args) -> void {
            if ((_size + sizeof...(Args)) > _capacity) reserve(_capacity == 0 ? 1 : (_capacity + sizeof...(Args)) * 2);
            ((allocator.construct(_data + _size++, move(args))), ...);
        }

        auto pop_back() -> void {
            if (_size > 0) {
                --_size;
                allocator.destroy(_data + _size);
            }
        }

        auto clear() -> void {
            for (size_t i = 0; i < _size; ++i) allocator.destroy(_data + i);
            allocator.deallocate(_data);
            _data = nullptr;
            _size = 0;
            _capacity = 0;
        }

        auto reserve(size_t newCapacity) -> void {
            if (newCapacity > _capacity) {
                T* newData = allocator.allocate(newCapacity);
                for (size_t i = 0; i < _size; ++i) {
                    allocator.construct(newData + i, move(_data[i]));
                    allocator.destroy(_data + i);
                }
                allocator.deallocate(_data);
                _data = newData;
                _capacity = newCapacity;
            }
        }

        auto resize(size_t newSize) -> void {
            if (newSize < _size) {
                for (size_t i = newSize; i < _size; ++i)
                    allocator.destroy(_data + i);
            } else if (newSize > _size) {
                if (newSize > _capacity) reserve(newSize);
                for (size_t i = _size; i < newSize; ++i)
                    allocator.construct(_data + i);
            }
            _size = newSize;
        }

        auto operator[](size_t index) const -> T const& { return _data[index]; }
        auto operator[](size_t index) -> T& { return _data[index]; }
        [[nodiscard]] auto capacity() const -> size_t { return _capacity; }
        [[nodiscard]] auto size() const -> size_t { return _size; }
        auto data() const -> T const* { return _data; }
        auto data() -> T* { return _data; }

        auto begin() -> T* { return _data; }
        auto end() -> T* { return _data + _size; }
        auto begin() const -> T const* { return _data; }
        auto end() const -> T const* { return _data + _size; }

    private:
        T* _data = nullptr;
        size_t _size = 0u;
        size_t _capacity = 0u;
        Allocator allocator;
    };
}

#endif