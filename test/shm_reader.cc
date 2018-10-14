#include <bits/stdc++.h>
#include "shm_common.h"
using namespace std;

bool lounger = false;
volatile bool running = true;
Q* q;

void signalHandler(int s) {
    running = false;
}

void reader() {
    while(running) {
        if(lounger) {
            Entry cur = q->pop();
            auto ts = now();
            cout << "tid: " << cur.tid << " val: " << cur.val << " latency: " << (ts - cur.ts) << endl;
        }
        else {
            int64_t idx = q->getReadIdx();
            Entry* data;
            while((data = q->allocRead(idx)) == nullptr) this_thread::yield();
            auto ts = now();
            cout << "tid: " << data->tid << " val: " << data->val << " latency: " << (ts - data->ts) << endl;
            q->commitRead(idx);
        }
    }
}

int main(int argc, char** argv) {
    if(argc > 1 && *argv[1] == '1') lounger = true;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    q = getQ();
    if(q == nullptr) return 1;
    reader();
    return 0;
}


