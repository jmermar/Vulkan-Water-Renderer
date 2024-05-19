#pragma once
#include <array>
#include <cstdint>
#include <cstring>

template <typename T, size_t SIZE>
class Pool {
   private:
    std::array<T, SIZE> data;
    bool free[SIZE];

    uint32_t getIndex(T* elem) {
        return ((uint64_t)elem - (uint64_t)&data[0]) / sizeof(T);
    }

   public:
    Pool() { memset(free, 1, SIZE); }
    T* allocate() {
        for (size_t i = 0; i < SIZE; i++) {
            if (free[i]) {
                free[i] = false;
                return &data[i];
            }
        }
        return 0;
    }

    void destroy(T* elem) {
        auto index = getIndex(elem);
        if (index < SIZE) {
            data[index] = T();
            free[index] = true;
        }
    }
};
