#include <bits/stdc++.h>
#include "../WFMPMC.h"
using namespace std;

const int nthr = 6;
const int num_per_thr = 10000;

WFMPMC<int, 4> q;
int64_t write_sum = 0;
int64_t read_sum = 0;
bool lounger = false;

void writer() {
    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<int> dis(0, 100000);

    int64_t sum = 0;
    int cnt = num_per_thr;
    while(cnt--) {
        int newval = dis(generator);
        sum += newval;
        if(lounger) {
            q.emplace(newval);
        }
        else {
            auto idx = q.getWriteIdx();
            int* data;
            while((data = q.allocWrite(idx)) == nullptr) this_thread::yield();
            *data = newval;
            q.commitWrite(idx);
        }
    }
    __sync_fetch_and_add(&write_sum, sum);
}

void reader() {
    int64_t sum = 0;
    int cnt = num_per_thr;
    while(cnt--) {
        if(lounger) {
            sum += q.pop();
        }
        else {
            int64_t idx = q.getReadIdx();
            int* data;
            while((data = q.allocRead(idx)) == nullptr) this_thread::yield();
            sum += *data;
            q.commitRead(idx);
        }
    }
    __sync_fetch_and_add(&read_sum, sum);
}

int main(int argc, char** argv) {
    if(argc > 1 && *argv[1] == '1') lounger = true;

    cout << "lounger: " << lounger << endl;

    vector<thread> thr;
    for(int i = 0; i < nthr; i++) {
        thr.emplace_back(writer);
    }
    for(int i = 0; i < nthr; i++) {
        thr.emplace_back(reader);
    }
    for(auto& cur : thr) {
        cur.join();
    }
    cout << "write_sum: " << write_sum << " read_sum: " << read_sum << endl;
}
