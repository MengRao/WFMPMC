#include <bits/stdc++.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "shm_common.h"
#include "cpupin.h"
using namespace std;

bool lounger = false;
volatile bool running = true;
Q* q;

void signalHandler(int s) {
    running = false;
}

void writer() {
    int num = 0;
    int tid = (pid_t)::syscall(SYS_gettid);
    while(running) {
        if(q->full()) {
            this_thread::yield();
            continue;
        }
        if(lounger) {
            q->emplace(Entry{now(), tid, num});
        }
        else {
            auto idx = q->getWriteIdx();
            Entry* data;
            while((data = q->getWritable(idx)) == nullptr)
                ;
            data->tid = tid;
            data->val = num;
            data->ts = now();
            q->commitWrite(idx);
        }
        cout << "tid: " << tid << " num: " << num << endl;
        num++;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

int main(int argc, char** argv) {
    if(argc > 1 && *argv[1] == '1') lounger = true;
    if(argc > 2) {
        int cpuid = stoi(argv[2]);
        if(!cpupin(cpuid)) return 1;
    }
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    q = getQ();
    if(q == nullptr) return 1;
    writer();
    return 0;
}

