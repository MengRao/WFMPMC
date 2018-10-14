#pragma once

template<class T, uint32_t SIZE>
class WFMPMC
{
public:
    static_assert(SIZE && !(SIZE & (SIZE - 1)), "SIZE must be a power of 2");

    // make constructor thread safe for SHM usage
    WFMPMC() {
        if(!__sync_bool_compare_and_swap(&init_state, 0, 1)) {
            while(init_state != 2)
                ;
            return;
        }
        for(uint32_t i = 0; i < SIZE; i++) {
            blks[i].stat = i;
        }
        asm volatile("" : : : "memory"); // memory fence
        init_state = 2;
    }

    int64_t getWriteIdx() {
        return __sync_fetch_and_add(&write_idx, 1);
    }

    T* allocWrite(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        if(blk.stat != idx) return nullptr;
        asm volatile("" : "=m"(blk.stat) : "m"(blk.data) :); // memory fence
        return &blk.data;
    }

    void commitWrite(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        asm volatile("" : : "m"(blk) :); // memory fence
        blk.stat = ~idx;
    }

    template<typename... Args>
    void emplace(Args&&... args) {
        int64_t idx = getWriteIdx();
        T* data;
        while((data = allocWrite(idx)) == nullptr)
            ;
        new(data) T(std::forward<Args>(args)...);
        commitWrite(idx);
    }

    int64_t getReadIdx() {
        return __sync_fetch_and_add(&read_idx, 1);
    }

    T* allocRead(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        if(blk.stat != ~idx) return nullptr;
        asm volatile("" : "=m"(blk) : :); // memory fence
        return &blk.data;
    }

    void commitRead(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        asm volatile("" : "=m"(blk.data) : "m"(blk.stat) :); // memory fence
        blk.stat = idx + SIZE;
    }

    T pop() {
        int64_t idx = getReadIdx();
        T* data;
        while((data = allocRead(idx)) == nullptr)
            ;
        T ret = std::move(*data);
        data->~T();
        commitRead(idx);
        return ret;
    }

private:
    int volatile init_state;
    alignas(64) int64_t write_idx;
    alignas(64) int64_t read_idx;
    struct
    {
        alignas(64) int64_t volatile stat;
        T data;
    } blks[SIZE];
};


