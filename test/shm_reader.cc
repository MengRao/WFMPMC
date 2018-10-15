#include <bits/stdc++.h>
#include "shm_common.h"
#include "cpupin.h"
using namespace std;

bool lounger = false;
volatile bool running = true;
Q* q;

void signalHandler(int s) {
    running = false;
}

void reader() {
    while(running) {
        // The upside of checking empty() is to avoid waiting for reading
        // while the downside is the additional latency
        /*
        if(q->empty()) {
            this_thread::yield();
            continue;
        }
        */
        if(lounger) {
            Entry cur = q->pop();
            auto ts = now();
            cout << "tid: " << cur.tid << " val: " << cur.val << " latency: " << (ts - cur.ts) << " cycles" << endl;
        }
        else {
            int64_t idx = q->getReadIdx();
            Entry* data;
            // we can't check running flag here because we must commit the idx we got before exiting
            // so program would hang if no writer exits, uncomment the empty check above to avoid this issue
            while((data = q->getReadable(idx)) == nullptr)
                ;
            auto ts = now();
            cout << "tid: " << data->tid << " val: " << data->val << " latency: " << (ts - data->ts) << " cycles"
                 << endl;
            q->commitRead(idx);
        }
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
    reader();
    return 0;
}


