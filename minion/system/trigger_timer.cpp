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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <ostream>

using namespace std;

volatile bool* trig;
volatile bool* ctrl_c_press;

bool check_double_ctrlc;

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#ifdef _WIN32

#define _WIN32_WINNT 0x0500
#include <windows.h>

void CALLBACK TimerProc(void* ,  BOOLEAN)
{ *trig = true; }
    
void activate_trigger(volatile bool* b, int timeout, bool CPU_time)
{
    if(CPU_time)
        cerr << "CPU-time timing not available on windows, falling back on clock" << endl;
    
    trig = b;
    *trig = false;
    
    if(timeout <= 0)
        return;        
    
    HANDLE m_timerHandle;
    
    BOOL success = ::CreateTimerQueueTimer(
    		&m_timerHandle,
    		NULL,
    		TimerProc,
    		NULL,
    		timeout * 1000,
    		,
    		WT_EXECUTEINTIMERTHREAD);
}

void install_ctrlc_trigger(volatile bool*)
{ /* Not implemented on windows */ }

#else

void trigger_function(int /* signum */ )
{ *trig = true; }

void activate_trigger(volatile bool* b, int timeout, bool CPU_time) // CPU_time = false -> real time 
{
  // We still set these, as they are how 'ctrlc' checks if we have got started properly or not.
  trig = b;
  *trig = false;
 
  if(timeout <= 0)
    return;

  signal(SIGXCPU, trigger_function);
  signal(SIGALRM, trigger_function);

  if(CPU_time)
  {
      rlimit lim;
      lim.rlim_cur = timeout;
      lim.rlim_max = timeout + 10;
      setrlimit(RLIMIT_CPU, &lim);
  }
  else
      alarm(timeout);
}

void ctrlc_function(int /* signum */ )
{
  if(check_double_ctrlc)
  {
    cerr << "Ctrl+C pressed twice. Exiting immediately." << endl;
    exit(1);
  }
  
  if(trig == NULL)
  {
    cerr << "Search has not started. Exiting immediately." << endl;
    exit(1);
  }
  
  check_double_ctrlc = true;
  
  cerr << "Ctrl+C pressed. Exiting.\n";
  // This is the quickest way to get things to stop.
  *trig = true;
  *ctrl_c_press = true;
}

void install_ctrlc_trigger(volatile bool* ctrl_c_press_)
{
  check_double_ctrlc = false;
  ctrl_c_press = ctrl_c_press_;
  signal(SIGINT, ctrlc_function);
}
#endif

