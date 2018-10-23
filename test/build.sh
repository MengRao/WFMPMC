set -x
g++ -std=c++11 -O3 -o bench bench.cc ../WFMPMC.cc
g++ -std=c++11 -O3 -o tor_test tor_test.cc ../WFMPMC.cc
g++ -std=c++11 -O3 -o multhread_test multhread_test.cc ../WFMPMC.cc -pthread
g++ -std=c++11 -O3 -o shm_writer shm_writer.cc ../WFMPMC.cc -lrt
g++ -std=c++11 -O3 -o shm_reader shm_reader.cc ../WFMPMC.cc -lrt
