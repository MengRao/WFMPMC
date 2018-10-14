#include <chrono>
#include <vector>
#include <algorithm>

inline uint64_t getns() {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}
inline uint64_t overhead_getns() {
    std::size_t iterations(10);
    uint64_t sum = 0;
    for(std::size_t i = 0; i < iterations; ++i) {
        uint64_t a = getns();
        uint64_t b = getns();
        sum += b - a;
    }
    return sum / iterations;
}

inline uint64_t rdtsc() {
    return __builtin_ia32_rdtsc();
}

inline uint64_t overhead_rdtsc() {
    std::size_t iterations(10);
    uint64_t sum = 0;
    for(std::size_t i = 0; i < iterations; ++i) {
        uint64_t a = rdtsc();
        uint64_t b = rdtsc();
        sum += b - a;
    }
    return sum / iterations;
}

inline uint64_t rdtscp() {
    unsigned int dummy;
    return __builtin_ia32_rdtscp(&dummy);
}
