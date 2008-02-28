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

class TimerClass
{
  rusage _internal_start_time;
  rusage _last_check_time;
  timeval start_wallclock;

public:
  
void startClock()
{
  getrusage(RUSAGE_SELF, &_internal_start_time);
  gettimeofday(&start_wallclock, NULL);
  _last_check_time = _internal_start_time;
}

double getTimeDiff(const rusage& end_time, const rusage& start_time)
{
  double time = end_time.ru_utime.tv_sec - start_time.ru_utime.tv_sec;
  time += (end_time.ru_utime.tv_usec - start_time.ru_utime.tv_usec) / 1000000.0;
  return time;
}

double getSecsSince(const rusage& start_time)
{
  struct rusage temp_time;
  getrusage(RUSAGE_SELF, &temp_time);
  return getTimeDiff(temp_time, start_time);
}

bool checkTimeout(unsigned seconds)
{ 
  return ( getSecsSince(_internal_start_time) >= seconds );
}

void printTimestepWithoutReset(const char* time_name)
{
  cout << time_name << getSecsSince(_last_check_time) << endl;
} 

void maybePrintTimestepStore(const char* time_name, const char* store_name, TableOut & tableout, bool toprint)
{
  struct rusage temp_time;
  getrusage(RUSAGE_SELF, &temp_time);
  double diff=getTimeDiff(temp_time, _last_check_time);
  if(toprint) cout << time_name << diff << endl;
  _last_check_time = temp_time;
  tableout.set(string(store_name), to_string(diff));
}


void maybePrintFinaltimestepStore(const char* time_name, const char* store_name, TableOut & tableout, bool toprint)
{
  timeval end_wallclock;
  gettimeofday(&end_wallclock, NULL);
  
  // Get final wallclock time.
  double time_wallclock = end_wallclock.tv_sec - start_wallclock.tv_sec;
  time_wallclock += (end_wallclock.tv_usec - start_wallclock.tv_usec) / 1000000.0;
  
  // Get used system time.
  double sys_time = _last_check_time.ru_stime.tv_sec - _internal_start_time.ru_stime.tv_sec;
  sys_time += (_last_check_time.ru_stime.tv_usec - _internal_start_time.ru_stime.tv_usec) / 1000000.0;
  
  maybePrintTimestepStore(time_name, store_name, tableout, toprint);
  if(toprint) 
  {
    cout << "Total Time: " << getTimeDiff(_last_check_time, _internal_start_time) << endl;
    cout << "Total System Time: " << sys_time << endl;
    cout << "Total Wall Time: " << time_wallclock << endl;
  }
  tableout.set(string("TotalTime"), to_string(getTimeDiff(_last_check_time, _internal_start_time)) );
}
};


