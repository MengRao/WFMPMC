set -x
g++ -std=c++11 -O3 -o multhread_test multhread_test.cc -pthread
g++ -std=c++11 -O3 -o shm_writer shm_writer.cc -lrt
g++ -std=c++11 -O3 -o shm_reader shm_reader.cc -lrt
