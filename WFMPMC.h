#pragma once
#include <atomic>

template<class T, uint32_t SIZE>
class WFMPMC
{
public:
    static_assert(SIZE && !(SIZE & (SIZE - 1)), "SIZE must be a power of 2");

    // must not init init_state in constructor
    WFMPMC()
        : write_idx(0)
        , read_idx(0) {
        for(uint32_t i = 0; i < SIZE; i++) {
            blks[i].stat.store(i, std::memory_order_relaxed);
        }
    }

    // shmInit() should only be called for objects allocated in SHM and are zero-initialized
    void shmInit() {
        int uninited_state = 0;
        if(!init_state.compare_exchange_strong(uninited_state, 1)) {
            while(init_state.load(std::memory_order_acquire) != 2)
                ;
            return;
        }
        new(this) WFMPMC();
        init_state.store(2, std::memory_order_release);
    }

    int64_t size() {
        return write_idx.load(std::memory_order_relaxed) - read_idx.load(std::memory_order_relaxed);
    }

    // A hint to check if read is likely to wait
    bool empty() {
        return size() <= 0;
    }

    // A hint to check if write is likely to wait
    bool full() {
        return size() >= SIZE;
    }

    int64_t getWriteIdx() {
        // later operation is dependent on the returned idx, so memory_order_consume is the best
        return write_idx.fetch_add(1, std::memory_order_consume);
    }

    T* getWritable(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        if(blk.stat.load(std::memory_order_acquire) != idx) return nullptr;
        return &blk.data;
    }

    void commitWrite(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        blk.stat.store(~idx, std::memory_order_release);
    }

    // Lounger(All in One) version of write, which is neither wait-free nor zero-copy
    template<typename... Args>
    void emplace(Args&&... args) {
        int64_t idx = getWriteIdx();
        T* data;
        while((data = getWritable(idx)) == nullptr)
            ;
        new(data) T(std::forward<Args>(args)...);
        commitWrite(idx);
    }

    int64_t getReadIdx() {
        // later operation is dependent on the returned idx, so memory_order_consume is the best
        return read_idx.fetch_add(1, std::memory_order_consume);
    }

    T* getReadable(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        if(blk.stat.load(std::memory_order_acquire) != ~idx) return nullptr;
        return &blk.data;
    }

    void commitRead(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        blk.stat.store(idx + SIZE, std::memory_order_release);
    }

    // Lounger(All in One) version of read, which is neither wait-free nor zero-copy
    T pop() {
        int64_t idx = getReadIdx();
        T* data;
        while((data = getReadable(idx)) == nullptr)
            ;
        T ret = std::move(*data);
        data->~T();
        commitRead(idx);
        return ret;
    }

private:
    std::atomic<int> init_state;
    alignas(64) std::atomic<int64_t> write_idx;
    alignas(64) std::atomic<int64_t> read_idx;
    struct
    {
        alignas(64) std::atomic<int64_t> stat;
        T data;
    } blks[SIZE];
};

