#ifndef ENGINE_CORE_HASH_MAP_H
#define ENGINE_CORE_HASH_MAP_H

#include <engine/core/types.h>

/*
template <typename Key, typename Value> class hash {
public:
    hash() : _capacity{16}, _size{0}, threshold(_capacity * 0.75f), data{static_cast<entry*>(malloc(_capacity * sizeof(entry)))} {}
    hash free(data); }

    auto insert(const Key& key, const Value& value) -> void {
        if (_size >= threshold) resize();

        size_t index = find_index(key);
        data[index] = { key, value, true };
        ++_size;
    }

    auto find(const Key& key, Value& value) const -> bool {
        auto index = find_index(key);
        return (data[index].occupied && data[index].key == key) && (value = data[index].value, true);
    }

    auto remove(const Key& key) -> bool {
        size_t index = find_index(key);
        return (data[index].occupied && data[index].key == key) && (data[index].occupied = false, --_size, true);
    }

    auto clear() -> void {
        for (size_t i = 0; i < _capacity; ++i) data[i].occupied = false;
        _size = 0;
    }

private:
    struct entry {
        Key key;
        Value value;
        bool occupied;
    };

    size_t _capacity, _size;
    float threshold;
    entry* data;

    auto find_index(const Key& key) const -> size_t {
        auto index = wyhash(static_cast<uint64_t>(key)) % _capacity;
        for (auto probe = 1; data[index].occupied && data[index].key != key; ++probe) index = (index + probe * probe) % _capacity;
        return index;
    }


    auto resize() -> void {
        auto newCapacity = _capacity * 2;
        auto* newData = static_cast<entry*>(malloc(newCapacity * sizeof(entry)));

        for (size_t i = 0; i < _capacity; ++i) {
            if (data[i].occupied) {
                size_t newIndex = find_index(data[i].key);

                for (size_t probe = 1; newData[newIndex].occupied; ++probe)
                    newIndex = (newIndex + probe * probe) % newCapacity;

                newData[newIndex] = data[i];
            }
        }

        free(data);
        data = newData;
        _capacity = newCapacity;
        threshold = _capacity * 0.75f;
    }
};
*/


template <typename Key, typename Value>
class hash {
public:
    hash() : capacity(16), size(0), threshold(capacity * 0.75f), data(static_cast<entry*>(malloc(capacity * sizeof(entry)))), occupancy(static_cast<uint8_t*>(malloc((capacity + 7) / 8))) {
        clear();
    }

    ~hash() {
        free(data);
        free(occupancy);
    }

    auto insert(const Key& key, const Value& value) -> void {
        if (size >= threshold)
            resize();

        size_t index = find_index(key);
        data[index] = { key, value };
        set_occupancy(index);
        ++size;
    }

    auto find(const Key& key, Value& value) const -> bool {
        size_t index = find_index(key);
        if (is_occupied(index) && data[index].key == key) {
            value = data[index].value;
            return true;
        }
        return false;
    }

    auto remove(const Key& key) -> bool {
        size_t index = find_index(key);
        if (is_occupied(index) && data[index].key == key) {
            clear_occupancy(index);
            --size;
            return true;
        }
        return false;
    }

    auto clear() -> void {
        size = 0;
        memset(occupancy, 0, (capacity + 7) / 8);
    }

private:
    struct entry {
        Key key;
        Value value;
    };

    size_t capacity;
    size_t size;
    float threshold;
    entry* data;
    uint8_t* occupancy;

    auto find_index(const Key& key) const -> size_t {
        auto hash = wyhash(static_cast<uint64_t>(key));
        size_t index = hash % capacity;
        size_t probe = 1;

        while (is_occupied(index) && data[index].key != key)
            index = (index + probe * probe) % capacity;

        return index;
    }

    auto resize() -> void {
        size_t newCapacity = capacity * 2;
        entry* newData = static_cast<entry*>(malloc(newCapacity * sizeof(entry)));
        uint8_t* newOccupancy = static_cast<uint8_t*>(malloc((newCapacity + 7) / 8));
        memset(newOccupancy, 0, (newCapacity + 7) / 8);

        for (size_t i = 0; i < capacity; ++i) {
            if (is_occupied(i)) {
                size_t newIndex = find_index(data[i].key);
                size_t probe = 1;

                while (is_occupied(newIndex))
                    newIndex = (newIndex + probe * probe) % newCapacity;

                newData[newIndex] = data[i];
                set_occupancy(newIndex);
            }
        }

        free(data);
        free(occupancy);
        data = newData;
        occupancy = newOccupancy;
        capacity = newCapacity;
        threshold = capacity * 0.75f;
    }

    auto is_occupied(size_t index) const -> bool {
        return (occupancy[index / 8] >> (index % 8)) & 1;
    }

    auto set_occupancy(size_t index) -> void {
        occupancy[index / 8] |= (1 << (index % 8));
    }

    auto clear_occupancy(size_t index) -> void {
        occupancy[index / 8] &= ~(1 << (index % 8));
    }
};


/*  Separate chaning impl
template <typename Key, typename Value>
class hash {
public:
    hash() : _capacity(16), _size(0), threshold(_capacity * 0.75f), data(static_cast<entry**>(malloc(_capacity * sizeof(entry*)))) {
        clear();
    }

    hash
        for (size_t i = 0; i < _capacity; ++i) {
            entry* current = data[i];
            while (current != nullptr) {
                entry* next = current->next;
                delete current;
                current = next;
            }
        }
        free(data);
    }

    auto insert(const Key& key, const Value& value) -> void {
        if (_size >= threshold)
            resize();

        size_t index = hash_function(key) % _capacity;
        entry* newEntry = new entry{ key, value };

        if (data[index] == nullptr) {
            data[index] = newEntry;
        } else {
            entry* current = data[index];
            while (current->next != nullptr) {
                if (current->key == key) {
                    current->value = value;  // Update existing value
                    delete newEntry;  // Discard new entry
                    return;
                }
                current = current->next;
            }
            current->next = newEntry;
        }

        ++_size;
    }

    auto find(const Key& key, Value& value) const -> bool {
        size_t index = hash_function(key) % _capacity;
        entry* current = data[index];

        while (current != nullptr) {
            if (current->key == key) {
                value = current->value;
                return true;
            }
            current = current->next;
        }

        return false;
    }

    auto remove(const Key& key) -> bool {
        size_t index = hash_function(key) % _capacity;
        entry* current = data[index];
        entry* previous = nullptr;

        while (current != nullptr) {
            if (current->key == key) {
                if (previous == nullptr) {
                    data[index] = current->next;
                } else {
                    previous->next = current->next;
                }
                delete current;
                --_size;
                return true;
            }
            previous = current;
            current = current->next;
        }

        return false;
    }

    auto clear() -> void {
        for (size_t i = 0; i < _capacity; ++i) {
            entry* current = data[i];
            while (current != nullptr) {
                entry* next = current->next;
                delete current;
                current = next;
            }
            data[i] = nullptr;
        }
        _size = 0;
    }

private:
    struct entry {
        Key key;
        Value value;
        entry* next;
    };

    size_t _capacity;
    size_t _size;
    float threshold;
    entry** data;

    auto resize() -> void {
        size_t newCapacity = _capacity * 2;
        entry** newData = static_cast<entry**>(malloc(newCapacity * sizeof(entry*)));
        memset(newData, 0, newCapacity * sizeof(entry*));

        for (size_t i = 0; i < _capacity; ++i) {
            entry* current = data[i];
            while (current != nullptr) {
                entry* next = current->next;
                size_t newIndex = hash_function(current->key) % newCapacity;

                if (newData[newIndex] == nullptr) {
                    newData[newIndex] = current;
                    current->next = nullptr;
                } else {
                    current->next = newData[newIndex];
                    newData[newIndex] = current;
                }

                current = next;
            }
        }

        free(data);
        data = newData;
        _capacity = newCapacity;
        threshold = _capacity * 0.75f;
    }

    auto hash_function(const Key& key) const -> size_t {
        // Custom hash function implementation for the Key type
        // Modify this function to suit your specific needs
        // Example: return std::hash<Key>{}(key);
        // Ensure the hash function provides a good distribution of values
        // and is compatible with the equality comparison operator (==)
    }
};

*/
#endif
