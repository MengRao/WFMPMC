# WFMPMC
WFMPMC is a real **wait-free** and **zero-copy** multiple producer multiple consumer queue template written in C++11.

It's also suitable for residing in shared memory in Linux for IPC

## Usage
Both writer and reader need to call 3 member functions in sequence to complete one operation. Take writer for example: 
  * getWriteIdx() to allocate an  index to write at. 
  * getWritable(idx) to get a writable pointer and user should assgin the object refered to by the pointer. If the object at the index is not ready for writing it'll return nullptr, and user should retry.
  * commitWrite(idx) to commit the operation after writing is done.
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
If you find the interface too tedious to use and don't care about the wait-free and zero-copy features, there're **Lounger** versions for **you**!
```c++
WFMPMC<int, 8> q;

// write integer 123 into queue
q.emplace(123)

...

// read an integer from queue
std::cout << q.pop() << std::endl;
```
Happy now?

## Performance
Benchmark is done on an Ubuntu 14.04 host with Intel(R) Xeon(R) CPU E5-2687W 0 @ 3.10GHz.

Single thread write/read pair operation for int32_t takes **53 cycles**. See [bench.cc](https://github.com/MengRao/WFMPMC/blob/master/test/bench.cc).

SHM IPC latency for transfering an 16 byte object is around **200 cycles** when cpu cores are pinned properly, and the performance is not affected by the number of writers or readers, as long as there's at least one reader waiting for reading. See [shm_writer.cc](https://github.com/MengRao/WFMPMC/blob/master/test/shm_writer.cc)/[shm_reader.cc](https://github.com/MengRao/WFMPMC/blob/master/test/shm_reader.cc).

## A Pitfall for Shared Memory Usage
WFMPMC requires that the index acquired from getXXXIdx() must be committed by commitXXX(idx) later, otherwise the queue will be corrupted. If unfortunately, an program who has got an idx and is busy-waiting on it needs to shut down, it can't do it right away but still waiting to commit, and it could last infinitely. 

One way to avoid this issue is to check if the operation is likely to wait before calling getXXXIdx(), that is, check **empty()** for reading and **full()** for writing. See [shm_writer.cc](https://github.com/MengRao/WFMPMC/blob/master/test/shm_writer.cc)/[shm_reader.cc](https://github.com/MengRao/WFMPMC/blob/master/test/shm_reader.cc) for details.
