#ifndef TIME_KEEPING_HFDAFAD
#define TIME_KEEPING_HFDAFAD

#ifdef NO_SYSTEM
inline long double init_start_wall_time(int = 0) {
  return 0;
}

inline long double get_wall_time() {
  return 0;
}

inline long double get_raw_wall_time() {
  return 0;
}

inline long double get_cpu_time() {
  return 0;
}

inline long double get_sys_time() {
  return 0;
}

inline long get_max_rss() {
  return 0;
}
#else

#ifdef _WIN32
#include <time.h>
#include <windows.h>

#define ULL unsigned __int64

inline long double get_wall_time() {
  return (long double)(clock()) / CLOCKS_PER_SEC;
}

inline long double get_raw_wall_time() {
  return get_wall_time();
}

inline long double get_cpu_time() {
  FILETIME creat_t, exit_t, kernel_t, user_t;
  if(GetProcessTimes(GetCurrentProcess(), &creat_t, &exit_t, &kernel_t, &user_t))
    return ((long double)(((ULL)user_t.dwHighDateTime << 32) + (ULL)user_t.dwLowDateTime)) / 10000 /
           1000;
  else
    abort();
}

inline long double get_sys_time() {
  FILETIME creat_t, exit_t, kernel_t, user_t;
  if(GetProcessTimes(GetCurrentProcess(), &creat_t, &exit_t, &kernel_t, &user_t))
    return ((long double)(((ULL)kernel_t.dwHighDateTime << 32) + (ULL)kernel_t.dwLowDateTime)) /
           10000 / 1000;
  else
    abort();
}

inline long get_max_rss() {
  // FIXME: implement me
  return 0;
}

#else

#include <sys/time.h>
#include <sys/resource.h>
#include <assert.h>
#include <time.h>

// This function is a horrible hack, to let us avoid nasty global variables
// leaking out of this file.
// If you want to call 'get_wall_time', then you must call init_start_wall_time
// first, perferably
// as close to the start of main as possible.

inline long double init_start_wall_time(int special_check = 0) {
  static long double start_time = 0;
  if(special_check == 0) {
    assert(start_time == 0);
    timeval t;
    gettimeofday(&t, NULL);
    start_time =
        static_cast<long double>(t.tv_sec) + static_cast<long double>(t.tv_usec) / 1000000.0;
    return -1;
  } else {
    assert(start_time != 0);
    return start_time;
  }
}

inline long double get_wall_time() {
  timeval t;
  gettimeofday(&t, NULL);
  long double current_time =
      static_cast<long double>(t.tv_sec) + static_cast<long double>(t.tv_usec) / 1000000.0;
  return current_time - init_start_wall_time(1);
}

inline long double get_raw_wall_time() {
  timeval t;
  gettimeofday(&t, NULL);
  long double current_time =
      static_cast<long double>(t.tv_sec) + static_cast<long double>(t.tv_usec) / 1000000.0;
  return current_time;
}

inline long double get_cpu_time() {
  rusage r;
  getrusage(RUSAGE_SELF, &r);
  long double cpu_time = r.ru_utime.tv_sec;
  cpu_time += static_cast<long double>(r.ru_utime.tv_usec) / 1000000.0;
  return cpu_time;
}

inline long double get_sys_time() {
  rusage r;
  getrusage(RUSAGE_SELF, &r);
  long double cpu_time = r.ru_stime.tv_sec;
  cpu_time += static_cast<long double>(r.ru_stime.tv_usec) / 1000000.0;
  return cpu_time;
}

inline long get_max_rss() {
  rusage r;
  getrusage(RUSAGE_SELF, &r);
#if __APPLE__ & __MACH__
  return r.ru_maxrss / 1024;
#else
  return r.ru_maxrss;
#endif
}
#endif
#endif

#endif
