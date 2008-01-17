#define NO_MAIN

#include <string>
#include <iostream>
#include <ostream>
#include <istream>
#include <algorithm>
#include <iomanip>
#include <map>

#include "system/system.h"
#include "constants.h"

using namespace std;

#include "get_info/get_info.h"

// XXX : Provide access to this.
const int ShowUnused = 0;

// In a class so the vectors can be initialized.
class initvectors
{
    public:
    vector<string> VarEvents;
    vector<string> VarNames;
    vector<string> Checks;
    vector<string> Changes;
    
    initvectors()
    {
        VarNames.push_back("Bool"); VarNames.push_back("Bound"); VarNames.push_back("SparseBound");
        VarNames.push_back("Range"); VarNames.push_back("BigRange");
        
        Checks.push_back("inDomain"); Checks.push_back("inDomain_noBC"); 
        Checks.push_back("getMin"); Checks.push_back("getMax");
        Checks.push_back("getAssignedValue"); Checks.push_back("isAssigned");
        Checks.push_back("getInitialMax"); Checks.push_back("getInitialMin");
        
        Changes.push_back("removeFromDomain"); Changes.push_back("setMin"); Changes. push_back("setMax");
        Changes.push_back("uncheckedAssign"); Changes.push_back("propagateAssign");
        if(VarNames.size()!=VarTypeSize)
        {
            cout << "Enum for VarType in get_info.h does not match VarNames in get_info.cpp"<<endl;
            exit(1);
        }
        
        VarEvents.push_back("construct");
        VarEvents.push_back("copy");
        VarEvents.push_back("isAssigned");
        VarEvents.push_back("getAssignedValue");
        VarEvents.push_back("isAssignedValue");
        VarEvents.push_back("inDomain");
        VarEvents.push_back("inDomain_noBoundCheck");
        VarEvents.push_back("getMax");
        VarEvents.push_back("getMin");
        VarEvents.push_back("getInitialMax");
        VarEvents.push_back("getInitialMin");
        VarEvents.push_back("setMax");
        VarEvents.push_back("setMin");
        VarEvents.push_back("uncheckedAssign");
        VarEvents.push_back("propagateAssign");
        VarEvents.push_back("removeFromDomain");
        VarEvents.push_back("addTrigger");
        VarEvents.push_back("getDomainChange");
        VarEvents.push_back("addDynamicTrigger");
        if(VarEvents.size()!=VarEventSize)
        {
            cout << "Enum for VarEvent in get_info.h does not match VarEvents in get_info.cpp"<<endl;
            exit(1);
        }
    }
};

initvectors init;

long long int vareventcounters[VarTypeSize][VarEventSize];
map<string, long long int> concount;
map<string, long long int> propcount;

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
{
    vareventcounters[type][event]++;
}

void ConInfoAddone(string type)
{ 
    if(concount.find(type) == concount.end())
        concount[type]=1;
    else
        concount[type]++;
}

void PropInfoAddone(string type)
{
    if(propcount.find(type) == propcount.end())
        propcount[type]=1;
    else
        propcount[type]++;
}

void print_search_info()
{
    // Collect event names which are in use.
    vector<string> EventNames;
    vector<string> Other;  // All the events in EventNames which are not in Changes or Checks.
    // First put some in in a standard order.
    EventNames.push_back("inDomain");EventNames.push_back("removeFromDomain");
    EventNames.push_back("getMin");EventNames.push_back("getMax");
    EventNames.push_back("setMin");EventNames.push_back("setMax");
    for(int i=0; i<VarTypeSize; i++)
    {
        for(int j=0; j<VarEventSize; j++)
        {
            string name=init.VarEvents[j];
            if(find(EventNames.begin(), EventNames.end(), name)==EventNames.end()
                && vareventcounters[i][j]!=0)
            {
                EventNames.push_back(name);
                if(find(init.Checks.begin(), init.Checks.end(), name)==init.Checks.end() &&
                    find(init.Changes.begin(), init.Changes.end(), name)==init.Changes.end())
                {
                    Other.push_back(name);
                }
            }
        }
    }
    
    sort(EventNames.begin()+6, EventNames.end());  // Sort all except the first 6
    
    // Print the table heading.
    
   cout << pad("");
   for(int i = 0; i < VarTypeSize; ++i)
   {
     cout << pad_start(init.VarNames[i]);
   }
   cout << pad_start("Total");
   cout << endl;
   
   // Print the table
   for(int j = 0; j < EventNames.size(); ++j)
   {
     string varevent=EventNames[j];
     
     int vareventindex=distance(EventNames.begin(), find(EventNames.begin(), EventNames.end(), varevent));
     long long int total = 0;
     for(int i = 0; i < VarTypeSize; ++i)
     {
         total += vareventcounters[i][vareventindex];
     }
     
     cout << pad(varevent);
    for(int i = 0; i < VarTypeSize; ++i)
    {
       cout << setiosflags(ios::right) << setw(12) <<vareventcounters[i][vareventindex];
    }
    cout << setiosflags(ios::right) << setw(12) << total;
    cout << endl;
     
   }
   
   // Print rows for the totals.
   for(int totaltype=0; totaltype<3; totaltype++) // checks, changes, other -> 0,1,2
   {
       vector<string> & eventstosum=((totaltype==0)? init.Checks:((totaltype==1)?init.Changes:Other));
       if(totaltype == 0)
       {
         cout << pad("TotalChecks");
         eventstosum=init.Checks;
       }
       else if(totaltype==1)
       {
         cout << pad("TotalChanges");
         eventstosum=init.Changes;
       }
       else
       {
         cout << pad("TotalOther");
         eventstosum=Other;
       }
       long long int total=0;
       
       for(int i = 0; i < init.VarNames.size(); ++i)
       {  
            long long int minortotal = 0;
            
            for(int j = 0; j < EventNames.size(); ++j)
            {
                string varevent=EventNames[j];
                int vareventindex=distance(EventNames.begin(), find(EventNames.begin(), EventNames.end(), varevent));
                pair<string, string> key=pair<string, string>(init.VarNames[i], EventNames[j]);
                if(find(eventstosum.begin(), eventstosum.end(), EventNames[j])!=eventstosum.end() )
                {
                     minortotal+= vareventcounters[i][vareventindex];
                     total += vareventcounters[i][vareventindex];
                }
            }
            
            cout << setiosflags(ios::right) << setw(12) <<
             minortotal;
       }
       cout << setiosflags(ios::right) << setw(12) <<
                total;
       cout << endl;
   }
   
  cout << "  ** Constraints" << endl;
  map<string, long long int>::iterator it;
  
  for(it = propcount.begin(); it != propcount.end(); it++)
  {
      cout << pad((*it).first ) << 
          setiosflags(ios::right) << setw(12) <<
          (*it).second << endl;
  }
  
  cout << "  ** Queue Events" << endl;
  
  for(it=concount.begin(); it != concount.end(); it++)
  {
      cout << pad((*it).first) << 
          setiosflags(ios::right) << setw(12) <<
          (*it).second << endl;
  }
}
