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

class TimerClass
{
  clock_t _internal_start_time;
  clock_t _last_check_time;

public:
  
void startClock()
{
  _internal_start_time = clock();
  _last_check_time = _internal_start_time;
}

bool checkTimeout(unsigned seconds)
{ return (clock() - _internal_start_time) >= (seconds * CLOCKS_PER_SEC); }

void printTimestepWithoutReset(const char* time_name)
{
  cout << time_name << (clock() - _last_check_time) / (1.0 * CLOCKS_PER_SEC) << endl;
} 

void maybePrintTimestepStore(const char* time_name, const char* store_name, TableOut & tableout, bool toprint)
{
    clock_t current_time = clock();
    double diff=(current_time - _last_check_time) / (1.0 * CLOCKS_PER_SEC);
    if(toprint) cout << time_name << diff << endl;
    _last_check_time = current_time;
    tableout.set(string(store_name), to_string(diff));
}


void maybePrintFinaltimestepStore(const char* time_name, const char* store_name, TableOut & tableout, bool toprint)
{
  maybePrintTimestepStore(time_name, store_name, tableout, toprint);
  if(toprint) 
    cout << "Total Time: " << (_last_check_time - _internal_start_time) / (1.0 * CLOCKS_PER_SEC) << endl;
  tableout.set(string("TotalTime"), to_string((_last_check_time - _internal_start_time) / (1.0 * CLOCKS_PER_SEC)));
}
};


