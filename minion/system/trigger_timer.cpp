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

#include "../minion.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

volatile bool* trig;

StateObj* stateObj;

bool check_double_ctrlc;

void trigger_function(int /* signum */ )
{
  *trig = true;
}

void activate_trigger(volatile bool* b, int timeout)
{
  if(timeout <= 0)
    return;
  
  trig = b;
  *trig = false;
  
  rlimit lim;
  lim.rlim_cur = timeout;
  lim.rlim_max = timeout + 10;
  setrlimit(RLIMIT_CPU, &lim);
  signal(SIGXCPU, trigger_function);
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
  getState(stateObj).setCtrlcPressed();
}

void install_ctrlc_trigger(void* _stateObj)
{
  check_double_ctrlc = false;
  stateObj = (StateObj*)_stateObj;
  signal(SIGINT, ctrlc_function);
}
