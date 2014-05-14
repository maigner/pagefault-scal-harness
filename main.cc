#include <cstdint>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>

using namespace std;

struct Buffer {
  volatile char* memory;
  size_t length;
};

const uint64_t kCpuMHz = 2000;
const uint64_t kMemorySize = 128 * (1UL<<30); 

uint64_t numThreads = 64;
uint64_t useHugepages = 0;
pthread_barrier_t barrier __attribute__((aligned(128)));

inline uint64_t Rdtsc() {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

void MapAddressSpace(Buffer& buffer, int flags) {
  const int prot = PROT_READ | PROT_WRITE;
  buffer.memory = reinterpret_cast<char*>(
      mmap(NULL, buffer.length, prot, flags, 0, 0));
  if (buffer.memory == (void*)-1) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }
}

void UnmapAddressSpace(Buffer& buffer) {
  munmap((void*)buffer.memory, buffer.length);
}

void *do_stuff(void* arg) {
  uint64_t id = (uint64_t)arg;
  int r;
  uint64_t start, stop;
  Buffer buffer;
  buffer.length = kMemorySize / numThreads;

  if (id == 0) {
    //printf("Buffersize: %lu for each of %lu threads\n", buffer.length, numThreads);
  }
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  size_t pageSize = 1UL<<12;
  if (useHugepages) { 
    flags |= MAP_HUGETLB;
    pageSize = 1UL<<21;
  }

  r = pthread_barrier_wait(&barrier);

  if (id == 0) {
    start = Rdtsc();
  }

  r = pthread_barrier_wait(&barrier);

  MapAddressSpace(buffer, flags);
  
  if (id == 0) {
    stop = Rdtsc();
    printf("%lu\tMAPPING\t%lu\t", numThreads, (stop-start)/kCpuMHz);
  }

  r = pthread_barrier_wait(&barrier);
  
  if (id == 0) {
    start = Rdtsc();
  }
  
  r = pthread_barrier_wait(&barrier);

  for (uint64_t i = 1; i < buffer.length-pageSize; i += pageSize) {
    //printf("%lu %p\n", i, &buffer.memory[i]);
    buffer.memory[i-1] = buffer.memory[i];
  }

  r = pthread_barrier_wait(&barrier);

  if (id == 0) {
    stop = Rdtsc();
    printf("FAULTING\t%lu\n", (stop-start)/kCpuMHz);
  }

}

int main(int argc, char **argv) {

  int r;
  if (argc > 1) {
    numThreads = atoi(argv[1]);
  }
  if (argc > 2) {
    useHugepages = atoi(argv[2]);
  }

  r = pthread_barrier_init(&barrier, NULL, numThreads);

  pthread_t *worker = static_cast<pthread_t*>(
      calloc(numThreads, sizeof(pthread_t))
      );

  for (uint64_t i = 0; i < numThreads; ++i) {
      r = pthread_create(&worker[i], NULL, do_stuff, (void*)i);
  }  
  
  for (uint64_t i = 0; i < numThreads; ++i) {
      r = pthread_join(worker[i], NULL);
  }

  return EXIT_SUCCESS;
}

