#include <bits/stdc++.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "shm_common.h"
#include "cpupin.h"
using namespace std;

int lounger = 0;
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
        if(lounger == 1) {
            q->emplace(Entry{now(), tid, num});
        }
        else if(lounger == 2) {
            while(!q->tryPush(Entry{now(), tid, num}))
                ;
        }
        else if(lounger == 3) {
            while(!q->tryVisitPush([&](Entry& data) {
                data.tid = tid;
                data.val = num;
                data.ts = now();
            }))
                ;
        }
        else { // lounger == 0
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
    if(argc > 1) lounger = stoi(argv[1]);
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

