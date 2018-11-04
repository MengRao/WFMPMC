#include <bits/stdc++.h>
#include "../WFMPMC.h"
#include "timestamp.h"
#include "cpupin.h"

using namespace std;

const int max_num = 100000000;

WFMPMC<int, 8> q;
int lounger = 0;

void bench() {
    int write_num = 0;
    int read_num = 0;

    while(read_num < max_num) {
        for(int i = 0; i < 8; i++) {
            if(lounger == 1) {
                q.emplace(write_num);
            }
            else if(lounger == 2) {
                while(!q.tryPush(write_num))
                    ;
            }
            else if(lounger == 3) {
                while(!q.tryVisitPush([&](int& data) { data = write_num; }))
                    ;
            }
            else { // lounger == 0
                auto idx = q.getWriteIdx();
                int* data;
                while((data = q.getWritable(idx)) == nullptr)
                    ;
                *data = write_num;
                q.commitWrite(idx);
            }
            write_num++;
        }
        for(int i = 0; i < 8; i++) {
            int cur;
            if(lounger == 1) {
                cur = q.pop();
            }
            else if(lounger == 2) {
                while(!q.tryPop(cur))
                    ;
            }
            else if(lounger == 3) {
                while(!q.tryVisitPop([&](int&& data) { cur = data; }))
                    ;
            }
            else {
                int64_t idx = q.getReadIdx();
                int* data;
                while((data = q.getReadable(idx)) == nullptr)
                    ;
                cur = *data;
                q.commitRead(idx);
            }
            if(cur != read_num) {
                cout << "bad, cur: " << cur << " read_num: " << read_num << endl;
                exit(1);
            }
            read_num++;
        }
    }
}

int main(int argc, char** argv) {
    if(argc > 1) lounger = stoi(argv[1]);
    if(!cpupin(1)) return 1;

    cout << "lounger: " << lounger << endl;

    auto before = rdtsc();
    bench();
    auto latency = rdtsc() - before;
    cout << "latency: " << latency << endl;
    cout << "latency per write/read: " << (latency / max_num) << " cycles" << endl;
}

