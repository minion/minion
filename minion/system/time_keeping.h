/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
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

#include <sys/time.h>
#include <sys/resource.h>

#define TIME_STRUCT timeval

#define CPU_TIME_STRUCT rusage

inline void get_current_time(TIME_STRUCT& t) { gettimeofday(&t, NULL); }
inline void get_current_cpu_time(CPU_TIME_STRUCT& t) { getrusage(RUSAGE_SELF, &t); }
  
inline double diff_cpu_time(const CPU_TIME_STRUCT& end_time, const CPU_TIME_STRUCT& start_time)
{
  double time = end_time.ru_utime.tv_sec - start_time.ru_utime.tv_sec;
  time += (end_time.ru_utime.tv_usec - start_time.ru_utime.tv_usec) / 1000000.0;
  return time;
}

inline double diff_sys_time(const CPU_TIME_STRUCT& end_time, const CPU_TIME_STRUCT& start_time)
{
  double sys_time = end_time.ru_stime.tv_sec - start_time.ru_stime.tv_sec;
  sys_time += (end_time.ru_stime.tv_usec - start_time.ru_stime.tv_usec) / 1000000.0;
  return sys_time;
}

inline double diff_time(const TIME_STRUCT& end_wallclock, const TIME_STRUCT& start_wallclock)
{
  // Get final wallclock time.
  double time_wallclock = end_wallclock.tv_sec - start_wallclock.tv_sec;
  time_wallclock += (end_wallclock.tv_usec - start_wallclock.tv_usec) / 1000000.0;
  return time_wallclock;
}

inline double cpu_time_elapsed(CPU_TIME_STRUCT& start_time)
{ 
  CPU_TIME_STRUCT t;
  get_current_cpu_time(t);
  return diff_cpu_time(t, start_time);
}


enum Output_Type

{ Output_Always, Output_1, Output_2};
  
class TimerClass
{
  CPU_TIME_STRUCT _internal_start_time;
  CPU_TIME_STRUCT _last_check_time;
  TIME_STRUCT start_wallclock;

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
  get_current_cpu_time(_internal_start_time);
  _last_check_time = _internal_start_time;
  get_current_time(start_wallclock);
}

bool checkTimeout(unsigned seconds)
{ return cpu_time_elapsed(_internal_start_time) >= seconds; }

void printTimestepWithoutReset(Output_Type t, const char* time_name)
{ 
  if(t != Output_Always && t != output)
    return;
  cout << time_name << cpu_time_elapsed(_last_check_time) << endl; 
} 

void maybePrintTimestepStore(Output_Type t, const char* time_name, const char* store_name, TableOut & tableout, bool toprint)
{
  if(t != Output_Always && t != output)
    return;
    
  CPU_TIME_STRUCT temp_time;
  get_current_cpu_time(temp_time);
  double diff = diff_cpu_time(temp_time, _last_check_time);
  if(toprint) cout << time_name << diff << endl;
  _last_check_time = temp_time;
  tableout.set(string(store_name), to_string(diff));
}

void maybePrintFinaltimestepStore(const char* time_name, const char* store_name, TableOut & tableout, bool toprint)
{
  TIME_STRUCT end_wallclock;
  get_current_time(end_wallclock);
  
  double time_wallclock = diff_time(end_wallclock, start_wallclock);
  
  CPU_TIME_STRUCT end_time;
  get_current_cpu_time(end_time);
    
  maybePrintTimestepStore(Output_Always, time_name, store_name, tableout, toprint);
  if(toprint) 
  {
    cout << "Total Time: " << diff_cpu_time(end_time, _internal_start_time) << endl;
    cout << "Total System Time: " << diff_sys_time(end_time, _internal_start_time) << endl;
    cout << "Total Wall Time: " << time_wallclock << endl;
  }
  tableout.set(string("TotalTime"), diff_cpu_time(end_time, _internal_start_time) );
}
};


