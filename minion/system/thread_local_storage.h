// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef MINION_THREAD_LOCAL_STORAGE_H
#define MINION_THREAD_LOCAL_STORAGE_H

#include <memory>
#include <stdexcept>

#ifdef __MINGW32__
#include <pthread.h>
#endif

namespace Minion {

// One value of T per thread, destroyed when that thread exits.
template <typename T>
T& threadLocalValue() {
#ifdef __MINGW32__
  // MinGW can free GCC's emulated TLS storage before calling the C++
  // destructor (msys2/MINGW-packages#2519). Keep the object on a separate
  // allocation owned by a pthread key instead. The key lives for the
  // lifetime of the process; its callback frees each thread's value.
  static const pthread_key_t key = []() {
    pthread_key_t result;
    if(pthread_key_create(&result, [](void* value) {
         delete static_cast<T*>(value);
       }) != 0) {
      throw std::runtime_error("cannot create Minion thread-local storage key");
    }
    return result;
  }();
  auto* value = static_cast<T*>(pthread_getspecific(key));
  if(value == nullptr) {
    std::unique_ptr<T> created(new T());
    if(pthread_setspecific(key, created.get()) != 0) {
      throw std::runtime_error("cannot set Minion thread-local storage value");
    }
    value = created.release();
  }
  return *value;
#else
  static thread_local T value;
  return value;
#endif
}

} // namespace Minion

#endif
