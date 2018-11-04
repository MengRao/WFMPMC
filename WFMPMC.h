/*
MIT License

Copyright (c) 2018 Meng Rao <raomeng1@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <atomic>
#include <unistd.h>
#include <sys/syscall.h>

// THR_SIZE must not be less than the max number of threads using tryPush/tryPop, otherwise they could fail forever
// It's preferred to set THR_SIZE twice the max number, because THR_SIZE is the size of an open addressing hash table
// 16 is a good default value for THR_SIZE, as 16 tids fit exactly in a cache line: 16 * 4 = 64
template<class T, uint32_t SIZE, uint32_t THR_SIZE = 16>
class WFMPMC
{
public:
    static_assert(SIZE && !(SIZE & (SIZE - 1)), "SIZE must be a power of 2");
    static_assert(THR_SIZE && !(THR_SIZE & (THR_SIZE - 1)), "THR_SIZE must be a power of 2");

    // must not init init_state in constructor
    WFMPMC()
        : write_idx(0)
        , read_idx(0) {
        for(uint32_t i = 0; i < SIZE; i++) {
            blks[i].stat.store(i, std::memory_order_relaxed);
        }
        for(uint32_t i = 0; i < THR_SIZE; i++) {
            tids[i].store(0, std::memory_order_relaxed);
            thr_idxes[i].write_idx = thr_idxes[i].read_idx = -1;
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

    ~WFMPMC() {
        for(uint32_t i = 0; i < SIZE; i++) {
            if(blks[i].stat.load(std::memory_order_relaxed) < 0) {
                reinterpret_cast<T&>(blks[i].data).~T();
            }
        }
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
        return write_idx.fetch_add(1, std::memory_order_relaxed);
    }

    // if successful, the returned pointer points to an *unconstructed* object that user should construct himself
    T* getWritable(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        if(blk.stat.load(std::memory_order_acquire) != idx) return nullptr;
        return reinterpret_cast<T*>(&blk.data);
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

    // zero-copy and wait-free
    // Visitor's signature: void f(T& val), where val is an *unconstructed* object
    template<typename Visitor>
    bool tryVisitPush(Visitor v) {
        ThrIdx* thridx = getThrIdx();
        if(!thridx) return false;
        int64_t& idx = thridx->write_idx;
        if(idx < 0) idx = getWriteIdx();
        T* data = getWritable(idx);
        if(!data) return false;
        v(*data);
        commitWrite(idx);
        idx = -1;
        return true;
    }

    // We should've provided a tryEmplace that takes variadic parameters instead of a single Type as in tryPush
    // but lambda doesn't support variadic perfect forwarding in capture until C++20, which is sad...
    template<typename Type>
    bool tryPush(Type&& val) {
        return tryVisitPush(
            [val = std::forward<decltype(val)>(val)](T& data) { new(&data) T(std::forward<decltype(val)>(val)); });
    }

    int64_t getReadIdx() {
        return read_idx.fetch_add(1, std::memory_order_relaxed);
    }

    T* getReadable(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        if(blk.stat.load(std::memory_order_acquire) != ~idx) return nullptr;
        return reinterpret_cast<T*>(&blk.data);
    }

    void commitRead(int64_t idx) {
        auto& blk = blks[idx % SIZE];
        reinterpret_cast<T&>(blk.data).~T();
        blk.stat.store(idx + SIZE, std::memory_order_release);
    }

    // Lounger(All in One) version of read, which is neither wait-free nor zero-copy
    T pop() {
        int64_t idx = getReadIdx();
        T* data;
        while((data = getReadable(idx)) == nullptr)
            ;
        T ret = std::move(*data);
        commitRead(idx);
        return ret;
    }

    // zero-copy and wait-free
    // Visitor's signature: void f(T&& val)
    template<typename Visitor>
    bool tryVisitPop(Visitor v) {
        ThrIdx* thridx = getThrIdx();
        if(!thridx) return false;
        int64_t& idx = thridx->read_idx;
        if(idx < 0) {
            idx = getReadIdx();
        }
        T* data = getReadable(idx);
        if(!data) return false;
        v(std::move(*data));
        commitRead(idx);
        idx = -1;
        return true;
    }

    // not zero-copy, but wait-free
    bool tryPop(T& v) {
        return tryVisitPop([&](T&& data) { v = std::move(data); });
    }

private:
    struct ThrIdx;
    ThrIdx* getThrIdx() {
        if(WFMPMC_tid == 0) {
            WFMPMC_tid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
        uint32_t cur = WFMPMC_tid % THR_SIZE;
        int cnt = THR_SIZE;
        while(cnt--) {
            uint32_t tid = tids[cur].load(std::memory_order_relaxed);
            if(tid == WFMPMC_tid) return &thr_idxes[cur];
            uint32_t null_tid = 0;
            if(tid == null_tid && tids[cur].compare_exchange_strong(null_tid, WFMPMC_tid, std::memory_order_relaxed)) {
                return &thr_idxes[cur];
            }
            cur = (cur + 1) % THR_SIZE;
        }
        // Bad: this thread will return nullptr forever
        return nullptr;
    }

private:
    std::atomic<int> init_state;
    alignas(64) std::atomic<int64_t> write_idx;
    alignas(64) std::atomic<int64_t> read_idx;
    struct
    {
        alignas(64) std::atomic<int64_t> stat;
        typename std::aligned_storage<sizeof(T), alignof(T)>::type data;
    } blks[SIZE];

    alignas(64) std::atomic<uint32_t> tids[THR_SIZE];
    struct ThrIdx
    {
        alignas(64) int64_t write_idx;
        int64_t read_idx;
    } thr_idxes[THR_SIZE];

    static thread_local uint32_t WFMPMC_tid;
};

// It's OK to define template static variable in header
template<class T, uint32_t SIZE, uint32_t THR_SIZE>
thread_local uint32_t WFMPMC<T, SIZE, THR_SIZE>::WFMPMC_tid;

