#include <bits/stdc++.h>
#include "../WFMPMC.h"
using namespace std;

const int nthr = 4;
const int num_per_thr = 1000000;

WFMPMC<int, 4, 16> q;
int64_t write_sum = 0;
int64_t read_sum = 0;
int lounger = 0;

void writer() {
    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<int> dis(0, 100000);

    int64_t sum = 0;
    int cnt = num_per_thr;
    while(cnt--) {
        int newval = dis(generator);
        sum += newval;
        if(lounger == 1) {
            q.emplace(newval);
        }
        else if(lounger == 2) {
            while(!q.tryPush(newval))
                ;
        }
        else if(lounger == 3) {
            while(!q.tryVisitPush([&](int& data) { data = newval; }))
                ;
        }
        else { // lounger == 0
            auto idx = q.getWriteIdx();
            int* data;
            while((data = q.getWritable(idx)) == nullptr)
                ;
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
        if(lounger == 1) {
            sum += q.pop();
        }
        else if(lounger == 2) {
            int data;
            while(!q.tryPop(data))
                ;
            sum += data;
        }
        else if(lounger == 3) {
            while(!q.tryVisitPop([&](int&& data) { sum += data; }))
                ;
        }
        else {
            int64_t idx = q.getReadIdx();
            int* data;
            while((data = q.getReadable(idx)) == nullptr)
                ;
            sum += *data;
            q.commitRead(idx);
        }
    }
    __sync_fetch_and_add(&read_sum, sum);
}

int main(int argc, char** argv) {
    if(argc > 1) lounger = stoi(argv[1]);

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
