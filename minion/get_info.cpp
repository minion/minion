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

#include <string>
#include <iostream>
#include <ostream>
#include <istream>
#include <iomanip>

#include "system/system.h"
#include "constants.h"

using namespace std;

#include "get_info/get_info.h"

// XXX : Provide access to this.
const int ShowUnused = 0;

string EventNames[] = {
#define VAR_EVENT(x,y) #x ,
#include "get_info/PropEvents.h"
#undef VAR_EVENT
""
};

// 0 - ignore, 1 - read, 2 - write
int EventCategory[] = {
#define NULL_EVENT 0
#define READ_EVENT 1
#define WRITE_EVENT 2
#define VAR_EVENT(x,y) y ,
#include "get_info/PropEvents.h"
#undef VAR_EVENT
0
};

string VarNames[] =
{ "Bool" , "Bound", "SparseBound", "Range", "BigRange"};

string ConEventNames[] =  {
"StaticTrigger",
"DynamicTrigger",
"SpecialTrigger",
"DynamicMovePtr",
"AddSpecialToQueue",
"AddConToQueue",
"AddDynToQueue",
  "SearchTrie",
  "LoopSearchTrie",
};

string PropEventNames[] = {
#define PROP_EVENT(x) #x ,
#include "get_info/PropEvents.h"
#undef PROP_EVENT
 };

 
 
// This is put in an object so we can zero it at start time.
struct VarInfo
{
  long long int counters[VarTypeCount][VarEventCount];
  long long int concount[ConEventCount];
  long long int propcount[PropEventCount];
  VarInfo()
  {
   for(int i = 0; i < VarTypeCount; ++i)
     for(int j = 0; j < VarEventCount; ++j)
       counters[i][j] = 0;
       
       
   for(int i = 0; i < ConEventCount; ++i)
     concount[i] = 0;
     
   for(int i = 0; i < PropEventCount; ++i)
     propcount[i] = 0;
  }
};

VarInfo var_info;

string pad(string s, int length = 18)
{
  s.resize(length,' ');
  return s;
}

// There must be a neater way of doing this...
string pad_start(string s, int length = 12)
{
  string output;
  output.resize(length - s.size(), ' ');
  return output + s;
}

void VarInfoAddone(Info_VarType type, Info_VarEvent event)
{ var_info.counters[type][event]++; }

void ConInfoAddone(Info_ConEvent type)
{ var_info.concount[type]++; }

void PropInfoAddone(Info_PropEvent type)
{ var_info.propcount[type]++; }

void print_search_info()
{
   cout << pad("");
   for(int i = 0; i < VarTypeCount; ++i)
     cout << pad_start(VarNames[i]);
   cout << pad_start("Total");
   cout << endl;
  
   for(int j = 0; j < VarEventCount; ++j)  
   {
     long long int total = 0;
     for(int i = 0; i < VarTypeCount; ++i)
       total += var_info.counters[i][j];
       
     if(ShowUnused || total != 0)
     {
       cout << pad(EventNames[j]);
       for(int i = 0; i < VarTypeCount; ++i)
       {
         cout << setiosflags(ios::right) << setw(12) <<
                 var_info.counters[i][j];
       }
       cout << setiosflags(ios::right) << setw(12) << total;
       cout << endl;
     }
   }
   
   for(int check_type = 1; check_type < 3; check_type++)
   {
     long long int total = 0;
     for(int i = 0; i < VarTypeCount; ++i)
       for(int j = 0; j < VarEventCount; ++j)
       {
         if (EventCategory[j] == check_type)
          total += var_info.counters[i][j];
       }
        
     if(ShowUnused || total != 0)
     {
       if(check_type == 1)
         cout << pad("TotalChecks");
       else
         cout << pad("TotalChanges");
     
       for(int i = 0; i < VarTypeCount; ++i)
       {
         long long int checks = 0;
         for(int j = 0; j < VarEventCount; ++j)  
         {
           if (EventCategory[j] == check_type)
             checks += var_info.counters[i][j];
         }
         cout << setiosflags(ios::right) << setw(12) <<
                 checks;
       }
        cout << setiosflags(ios::right) << setw(12) <<
                total;
       cout << endl;
     }
   }
  
  cout << "  ** Constraints" << endl;
  for(int i = 0; i < PropEventCount; ++i)
  {
    if(ShowUnused || var_info.propcount[i] != 0)
      cout << pad(PropEventNames[i]) << 
              setiosflags(ios::right) << setw(12) <<
              var_info.propcount[i] << endl;
  }
  
  cout << "  ** Queue Events" << endl;
  for(int i = 0; i < ConEventCount; ++i)
  {
    if(ShowUnused || var_info.concount[i] != 0)
      cout << pad(ConEventNames[i]) << 
              setiosflags(ios::right) << setw(12) <<
              var_info.concount[i] << endl;
  }
}
