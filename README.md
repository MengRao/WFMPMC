# WFMPMC
WFMPMC is a **wait-free**(almost: [reason](https://github.com/MengRao/WFMPMC#a-pitfall-for-shared-memory-usage))  and **zero-copy** multiple producer multiple consumer queue template written in C++11.

It's also suitable for residing in shared memory in Linux for IPC

## Usage
Both writer and reader need to call 3 member functions in sequence to complete one operation. Take writer for example: 
1. `getWriteIdx()` to allocate an  index to write at. 
2. `getWritable(idx)` to get a writable pointer and user should construct the object referred to by the pointer. If the object at the index is not ready for writing(when the queue is `full()`) it'll return nullptr, and user can retry.
3. `commitWrite(idx)` to commit the operation after writing is done.
  
Of course, all the 3 operations are wait-free.
```c++
WFMPMC<int, 8> q;

// write integer 123 into queue
auto idx = q.getWriteIdx();
int* data;
while((data = q.getWritable(idx)) == nullptr)
  ;
*data = 123;
q.commitWrite(idx);

...

// read an integer from queue
auto idx = q.getReadIdx();
int* data;
while((data = q.getReadable(idx)) == nullptr)
  ;
std::cout<< *data << std::endl;
q.commitRead(idx);
```
Note that the user defined type `T` doesn't require default constructor, but the pointer returned by `getWritable(idx)`(if successful) points to an **unconstructed** object, surprise? Thus user should construct it himself if necessary. See [tor_test.cc](https://github.com/MengRao/WFMPMC/blob/master/test/tor_test.cc).

If you find the API too tedious to use and don't care about the wait-free and zero-copy features, there're **Lounger** versions for **you**: `emplace()` and `pop()`.
```c++
WFMPMC<int, 8> q;

// write integer 123 into queue
q.emplace(123)

...

// read an integer from queue
std::cout << q.pop() << std::endl;
```
There're also `tryPush()` and `tryPop()` which are not zero-copy but wait-free:
```c++
WFMPMC<int, 8> q;

// write integer 123 into queue
while(!q.tryPush(123))
  ;

...

// read an integer from queue
int data;
while(!q.tryPop(data))
  ;
std::cout << data << std::endl;
```
The implementation of Try API is a little bit tricky that it uses **object scope thread local** variables to save the index when a try is failed, and reuse it in the next try. It uses an open addressing hash table for searching thread ids, and the size of the hash table can be set by the third template parameter `THR_SIZE`.

Lastly, zero-copy try API was added which uses Visitor model: `tryVisitPush(lambda)`/`tryVisitPop(lambda)`:
```c++
WFMPMC<int, 8> q;

// write integer 123 into queue
while(!q.tryVisitPush([](int& data){ data = 123; }))
  ;

...

// read an integer from queue
while(!q.tryVisitPop([](int&& data){ std::cout << data << std::endl; }))
  ;
```

## Performance
Benchmark is done on an Ubuntu 14.04 host with Intel(R) Xeon(R) CPU E5-2687W 0 @ 3.10GHz.

Single thread write/read pair operation for int32_t takes **53 cycles**. See [bench.cc](https://github.com/MengRao/WFMPMC/blob/master/test/bench.cc).

SHM IPC latency for transfering an 16 byte object is around **200 cycles** when cpu cores are pinned properly, and the performance is not affected by the number of writers or readers, as long as there's at least one reader waiting for reading. See [shm_writer.cc](https://github.com/MengRao/WFMPMC/blob/master/test/shm_writer.cc)/[shm_reader.cc](https://github.com/MengRao/WFMPMC/blob/master/test/shm_reader.cc).

## A Pitfall for Shared Memory Usage
WFMPMC requires that the index acquired from getXXXIdx() must be committed by commitXXX(idx) later(Try API is similar that it must succeed eventually), otherwise the queue will be corrupted. If unfortunately, an program who has got an idx and is busy-waiting on it needs to shut down, it can't do it right away but still waiting to commit, and this could last infinitely. 

Even worse, if a writer holding an index crashes before committing, then the corresponding reader will never succeed in reading one element even if other writer has completed a write operation. This is the reason why WFMPMC is not strict wait-free.

One way to mitigate this risk is to check if the operation is likely to wait before calling getXXXIdx(), that is, check `empty()` for reading and `full()` for writing. See [shm_writer.cc](https://github.com/MengRao/WFMPMC/blob/master/test/shm_writer.cc)/[shm_reader.cc](https://github.com/MengRao/WFMPMC/blob/master/test/shm_reader.cc) for details.

## References
- *Erik Rigtorp*. [MPMCQueue](https://github.com/rigtorp/MPMCQueue).
