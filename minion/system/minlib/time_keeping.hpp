#ifndef TIME_KEEPING_HFDAFAD
#define TIME_KEEPING_HFDAFAD

#ifdef NO_SYSTEM
inline long double initStart_wallTime(int = 0) {
  return 0;
}

inline long double get_wallTime() {
  return 0;
}

inline long double getRaw_wallTime() {
  return 0;
}

inline long double get_cpuTime() {
  return 0;
}

inline long double get_sysTime() {
  return 0;
}

inline long getMax_rss() {
  return 0;
}
#else

#ifdef _WIN32
#include <time.h>
#include <windows.h>

#define ULL unsigned __int64

inline long double get_wallTime() {
  return (long double)(clock()) / CLOCKS_PER_SEC;
}

inline long double getRaw_wallTime() {
  return get_wallTime();
}

inline long double get_cpuTime() {
  FILETIME creat_t, exit_t, kernel_t, user_t;
  if(GetProcessTimes(GetCurrentProcess(), &creat_t, &exit_t, &kernel_t, &user_t))
    return ((long double)(((ULL)user_t.dwHighDateTime << 32) + (ULL)user_t.dwLowDateTime)) / 10000 /
           1000;
  else
    abort();
}

inline long double get_sysTime() {
  FILETIME creat_t, exit_t, kernel_t, user_t;
  if(GetProcessTimes(GetCurrentProcess(), &creat_t, &exit_t, &kernel_t, &user_t))
    return ((long double)(((ULL)kernel_t.dwHighDateTime << 32) + (ULL)kernel_t.dwLowDateTime)) /
           10000 / 1000;
  else
    abort();
}

inline long getMax_rss() {
  // FIXME: implement me
  return 0;
}

#else

#include <assert.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>

// This function is a horrible hack, to let us avoid nasty global variables
// leaking out of this file.
// If you want to call 'get_wallTime', then you must call initStart_wallTime
// first, perferably
// as close to the start of main as possible.

inline long double initStart_wallTime(int specialCheck = 0) {
  static long double startTime = 0;
  if(specialCheck == 0) {
    assert(startTime == 0);
    timeval t;
    gettimeofday(&t, NULL);
    startTime =
        static_cast<long double>(t.tv_sec) + static_cast<long double>(t.tv_usec) / 1000000.0;
    return -1;
  } else {
    assert(startTime != 0);
    return startTime;
  }
}

inline long double get_wallTime() {
  timeval t;
  gettimeofday(&t, NULL);
  long double currentTime =
      static_cast<long double>(t.tv_sec) + static_cast<long double>(t.tv_usec) / 1000000.0;
  return currentTime - initStart_wallTime(1);
}

inline long double getRaw_wallTime() {
  timeval t;
  gettimeofday(&t, NULL);
  long double currentTime =
      static_cast<long double>(t.tv_sec) + static_cast<long double>(t.tv_usec) / 1000000.0;
  return currentTime;
}

inline long double get_cpuTime() {
  rusage r;
  getrusage(RUSAGE_SELF, &r);
  long double cpuTime = r.ru_utime.tv_sec;
  cpuTime += static_cast<long double>(r.ru_utime.tv_usec) / 1000000.0;
  return cpuTime;
}

inline long double get_sysTime() {
  rusage r;
  getrusage(RUSAGE_SELF, &r);
  long double cpuTime = r.ru_stime.tv_sec;
  cpuTime += static_cast<long double>(r.ru_stime.tv_usec) / 1000000.0;
  return cpuTime;
}

inline long getMax_rss() {
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
