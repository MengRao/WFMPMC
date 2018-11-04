set -x
g++ -std=c++14 -O3 -o bench bench.cc
g++ -std=c++14 -O3 -o tor_test tor_test.cc
g++ -std=c++14 -O3 -o multhread_test multhread_test.cc -pthread
g++ -std=c++14 -O3 -o shm_writer shm_writer.cc -lrt
g++ -std=c++14 -O3 -o shm_reader shm_reader.cc -lrt
