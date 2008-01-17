#define NO_MAIN

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
 "construct",
 "copy",
 "isAssigned",
 "getAssignedValue",
   "isAssignedValue",
   "inDomain",
   "inDomain_noBC",
   "getMax",
   "getMin",
   "getInitialMax",
   "getInitialMin",
  "setMax",
  "setMin",
  "uncheckedAssign",
  "propogateAssign",
  "RemoveFromDomain",
  "addTrigger",
  "getDomainChange",
  "addDynamicTrigger"
};

// 0 - ignore, 1 - read, 2 - write
int EventCategory[] = {
 0,
 0,
 0,
 1,
 1,
 1,
 1,
 1,
 1,
 1,
 1,
 1,
 2,
 2,
 2,
 2,
 2,
 0,
 0,
 0,
 
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
};

string PropEventNames[] = {
"CheckAssign",
 "BoundTable",
 "Reify",
 "ReifyTrue",
 "Table",
 "ArrayNeq",
 "BinaryNeq",
 "NonGACElement",
 "GACElement",
 "Lex",
 "FullSum",
 "BoolSum",
 "LightSum",
 "WeightBoolSum",
 "ReifyEqual",
 "Equal",
 "BinaryLeq",
 "Min",
 "OccEqual",
 "Pow",
 "And",
 "Product",
 "DynSum",
 "DynSumSat",
 "Dyn3SAT",
 "Dyn2SAT",
 "DynLitWatch",
 "DynElement",
 "DynVecNeq",
 "DynGACTable",
 "Mod"
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

void VarInfoAddone(VarType type, VarEvent event)
{ var_info.counters[type][event]++; }

void ConInfoAddone(ConEvent type)
{ var_info.concount[type]++; }

void PropInfoAddone(PropEvent type)
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
