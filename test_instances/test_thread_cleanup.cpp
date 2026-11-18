// Thread-local values must stay intact until their destructors have run.
// MinGW's emulated C++ TLS used to free their storage first, corrupting the
// heap when solver workers exited. Also check isolation between live threads.

#include "system/thread_local_storage.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

static std::atomic<int> destroyed{0};

struct Completion {
  ~Completion() {
    ++destroyed;
  }
};

struct ThreadState {
  // Members are destroyed in reverse order: count completion only after
  // payload's destructor has successfully released its allocation.
  Completion completion;
  std::vector<int> payload;
  int expected = -1;

  ~ThreadState() {
    if(expected >= 0 && (payload.size() != 256 || payload.front() != expected ||
                         payload.back() != expected)) {
      std::abort();
    }
  }
};

int main() {
  constexpr int threadCount = 8;
  constexpr int rounds = 8;
  for(int round = 0; round < rounds; ++round) {
    std::atomic<int> ready{0};
    std::vector<std::thread> threads;
    for(int i = 0; i < threadCount; ++i) {
      threads.emplace_back([&, i]() {
        auto& state = Minion::threadLocalValue<ThreadState>();
        state.expected = round * threadCount + i;
        state.payload.assign(256, state.expected);
        ++ready;
        while(ready.load() != threadCount)
          std::this_thread::yield();
        auto& current = Minion::threadLocalValue<ThreadState>();
        if(current.payload.front() != round * threadCount + i)
          std::abort();
      });
    }
    for(auto& thread : threads)
      thread.join();
  }
  if(destroyed != threadCount * rounds) {
    std::cerr << "Thread-local destructors did not complete: " << destroyed << '\n';
    return 1;
  }
  std::cout << "Thread-local values stayed isolated and all destructors completed.\n";
}
