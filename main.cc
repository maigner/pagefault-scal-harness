#include <cstdint>
#include <iostream>
#include <pthread.h>
#include <unistd.h>

using namespace std;

const uint64_t kCpuMHz = 2000;

pthread_barrier_t barrier __attribute__((aligned(128)));

inline uint64_t Rdtsc() {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

void *do_stuff(void* arg) {
  uint64_t id = (uint64_t)arg;
  int r;

  r = pthread_barrier_wait(&barrier);
}

int main(int argc, char **argv) {

  int r;
  uint64_t numThreads = 80;
  if (argc > 1) {
    numThreads = atoi(argv[1]);
  }

  r = pthread_barrier_init(&barrier, NULL, numThreads);

  pthread_t *worker = static_cast<pthread_t*>(
      calloc(numThreads, sizeof(pthread_t))
      );

  for (uint64_t i = 0; i < numThreads; ++i) {
      r = pthread_create(&worker[i], NULL, do_stuff, (void*)i);
  }

  return EXIT_SUCCESS;
}

