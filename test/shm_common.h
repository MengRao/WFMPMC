#pragma once
#include <bits/stdc++.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include "../WFMPMC.h"
#include "timestamp.h"

template<class T>
T* shmmap(const char * filename) {
    int fd = shm_open(filename, O_CREAT | O_RDWR, 0666);
    if(fd == -1) {
        std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
        return nullptr;
    }
    if(ftruncate(fd, sizeof(T))) {
        std::cerr << "ftruncate failed: " << strerror(errno) << std::endl;
        close(fd);
        return nullptr;
    }
    T* ret = (T*)mmap(0, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if(ret == MAP_FAILED) {
        std::cerr << "mmap failed: " << strerror(errno) << std::endl;
        return nullptr;
    }
    ret->shmInit();
    return ret;
}

struct Entry
{
    uint64_t ts;
    int tid;
    int val;
};

using Q = WFMPMC<Entry, 8>;

Q* getQ() {
    return shmmap<Q>("/WFMPMC.shm");
}

inline uint64_t now() {
    return rdtsc();
}

