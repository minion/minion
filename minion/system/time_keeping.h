/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// This file is designed to encapsulate all time keeping done by Minion.
#ifdef _WIN32
#define ULL unsigned __int64

inline long double get_wall_time()
{
    return (long double)(clock()) / CLOCKS_PER_SEC;
}

inline long double get_cpu_time()
{
    FILETIME creat_t, exit_t, kernel_t, user_t;
     if (GetProcessTimes(GetCurrentProcess(), &creat_t, &exit_t, &kernel_t, &user_t))
      return ((long double) (((ULL) user_t.dwHighDateTime << 32) + (ULL) user_t.dwLowDateTime)) / 10000 / 1000;
     else
         abort();   
}

inline long double get_sys_time()
{
    FILETIME creat_t, exit_t, kernel_t, user_t;
     if (GetProcessTimes(GetCurrentProcess(), &creat_t, &exit_t, &kernel_t, &user_t))
      return ((long double) (((ULL) kernel_t.dwHighDateTime << 32) + (ULL) kernel_t.dwLowDateTime)) / 10000 / 1000;
     else
         abort();   
}

inline long get_max_rss()
{
    // FIXME: implement me
    return 0;
}


#else
#include <sys/time.h>
#include <sys/resource.h>

inline long double get_wall_time()
{
    timeval t;
    gettimeofday(&t, NULL);
    return static_cast<long double>(t.tv_sec) + static_cast<long double>(t.tv_usec) / 1000000.0;
}

inline long double get_cpu_time()
{
    rusage r;
    getrusage(RUSAGE_SELF, &r);
    long double cpu_time = r.ru_utime.tv_sec;
    cpu_time += static_cast<long double>(r.ru_utime.tv_usec) / 1000000.0;
    return cpu_time;
}

inline long double get_sys_time()
{
    rusage r;
    getrusage(RUSAGE_SELF, &r);
    long double cpu_time = r.ru_stime.tv_sec;
    cpu_time += static_cast<long double>(r.ru_stime.tv_usec) / 1000000.0;
    return cpu_time;
}

inline long get_max_rss()
{
    rusage r;
    getrusage(RUSAGE_SELF, &r);
#if __APPLE__ & __MACH__
    return r.ru_maxrss / 1024;
#else
    return r.ru_maxrss;
#endif
}

#endif
/*
inline void get_current_time(TIME_STRUCT& t) { gettimeofday(&t, NULL); }
inline void get_current_cpu_time(CPU_TIME_STRUCT& t) { getrusage(RUSAGE_SELF, &t); }
  
inline long double diff_cpu_time(const CPU_TIME_STRUCT& end_time, const CPU_TIME_STRUCT& start_time)
{
  long double time = end_time.ru_utime.tv_sec - start_time.ru_utime.tv_sec;
  time += (end_time.ru_utime.tv_usec - start_time.ru_utime.tv_usec) / 1000000.0;
  return time;
}

inline long double diff_sys_time(const CPU_TIME_STRUCT& end_time, const CPU_TIME_STRUCT& start_time)
{
  long double sys_time = end_time.ru_stime.tv_sec - start_time.ru_stime.tv_sec;
  sys_time += (end_time.ru_stime.tv_usec - start_time.ru_stime.tv_usec) / 1000000.0;
  return sys_time;
}

inline long double diff_time(const TIME_STRUCT& end_wallclock, const TIME_STRUCT& start_wallclock)
{
  // Get final wallclock time.
  long double time_wallclock = end_wallclock.tv_sec - start_wallclock.tv_sec;
  time_wallclock += (end_wallclock.tv_usec - start_wallclock.tv_usec) / 1000000.0;
  return time_wallclock;
}

inline long double cpu_time_elapsed(CPU_TIME_STRUCT& start_time)
{ 
  CPU_TIME_STRUCT t;
  get_current_cpu_time(t);
  return diff_cpu_time(t, start_time);
}
*/

enum Output_Type

{ Output_Always, Output_1, Output_2};
  
class TimerClass
{
  long double _internal_cpu_start_time;
  long double _internal_sys_start_time;
  long double _last_check_time;
  long double start_wallclock;

  Output_Type output;
public:
  
  void setOutputType(int version)
  {
    switch(version)
    {
      case 1 : output = Output_1; return;
      case 2 : output = Output_2; return;
      default:
        cerr << "This copy of Minion doesn't support output format " + to_string(version);
        abort();
    }
  }
  
  TimerClass() : output(Output_1) 
  { 
    cout.setf(ios::fixed); 
    startClock(); 
  }
  
void startClock()
{
  _internal_cpu_start_time = get_cpu_time();
  _internal_sys_start_time = get_sys_time();
  _last_check_time = _internal_cpu_start_time;
  start_wallclock = get_wall_time();
}

bool checkTimeout(unsigned seconds)
{ return get_cpu_time() - _internal_cpu_start_time >= seconds; }

void printTimestepWithoutReset(Output_Type t, const char* time_name)
{ 
  if(t != Output_Always && t != output)
    return;
  cout << time_name << get_cpu_time() - _last_check_time << endl; 
} 

void maybePrintTimestepStore(Output_Type t, const char* time_name, const char* store_name, TableOut & tableout, bool toprint)
{
  if(t != Output_Always && t != output)
    return;

  long double temp_time = get_cpu_time();
  long double diff = temp_time - _last_check_time;
  if(toprint) cout << time_name << diff << endl;
  _last_check_time = temp_time;
  tableout.set(string(store_name), to_string(diff));
}

void maybePrintFinaltimestepStore(const char* time_name, const char* store_name, TableOut & tableout, bool toprint)
{
  long double time_wallclock = get_wall_time() - start_wallclock;
  
  long double end_cpu_time = get_cpu_time();
  long double end_sys_time = get_sys_time();
  
  maybePrintTimestepStore(Output_Always, time_name, store_name, tableout, toprint);
  if(toprint) 
  {
    cout << "Total Time: " << end_cpu_time - _internal_cpu_start_time << endl;
    cout << "Total System Time: " << end_sys_time - _internal_sys_start_time << endl;
    cout << "Total Wall Time: " << time_wallclock << endl;
    cout << "Maximum Memory (kB): " << get_max_rss() << endl;
  }
  tableout.set(string("TotalTime"), end_cpu_time - _internal_cpu_start_time );
}
};


