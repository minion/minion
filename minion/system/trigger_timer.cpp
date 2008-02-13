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

#include "trigger_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

volatile bool* trig;

void trigger_function(int /* signum */ )
{
  *trig = true;
}


void activate_trigger(volatile bool* b)
{
  trig = b;
  *trig = false;
  
  struct itimerval timer;
          
  signal(SIGALRM, trigger_function);
	timer.it_value.tv_sec = 1;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
}