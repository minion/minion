
#include <stdlib.h>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <utility>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

// This is a v.simple logging component.
// Call tableout.set("PropertyName", 127)

// A small container which contains an integer or a string.
struct datum
{
    bool is_string;
    int i;
    string s;
    
    datum(string in)
    {
        is_string=true;
        i=0;
        s=in;
    }
    
    datum(int in)
    {
        is_string=false;
        i=in;
        s=string("");
    }
    
    datum()
    {
        // 0 integer
        is_string=false;
        i=0;
        s=string("");
    }
    
    datum(const datum& obj)  { *this = obj; }
    
    string get_string()
    {
        if(!is_string)
        {
            cerr << "Something funny happened in datum get_string" <<endl;
        }
        return s;
    }
    
    int get_int()
    {
        if(is_string)
        {
            cerr << "Something funny happened in datum get_int" << endl;
        }
        return i;
    }
    
    string get_print()
    {
        // get value for printing
        if(is_string)
        {
            return s;
        }
        else
        {
            return to_string(i);
        }
    }
};

class TableOut
{
    private:
    // All the data for this run is kept in the map    
    map<string, datum> data;
    string filename;
    
    public:
    void set(string propname, int value)
    {
        // create a new, or overwrite the, entry with name propname
        data[propname] = datum(value);
    }
    
    void set(string propname, string value)
    {
        data[propname]=datum(value);
    }
    
    void addto(string propname, int addvalue)
    {
        data[propname]=datum(data[propname].get_int() + addvalue);
    }
    
    void debug_printall()
    {
        std::map<string, datum>::iterator it;
        
        for(it = data.begin(); it != data.end(); it++)
        {
            cout<< (*it).first << "," << (*it).second.get_print() << endl;
        }
    }
    
    void print_line()
    {
        // First version: this checks if we are at the top of a file. If so, prints column headers. 
        // Then
        // outputs a line. 
        
        // If the column headers are unexpected, should print something to cerr.
        
        // Second version should be able to cope with different column headers? At least different orders?
        
        fstream f(filename.c_str(), ios::app | ios::out);  // Open with append mode.
        
        if(!f)
        {
            cerr << "tableout.cpp: failed to open file to output table." << endl;
        }
        
        // if file position is the beginning of the file, then output the column headers.
        if(f.tellp()==streampos(0))
        {
            f << "#";
            
            map<string, datum>::iterator it;
            for(it = data.begin(); it != data.end(); it++)
            {
                f<<  "\"" << (*it).first << "\" " ;
            }
            f <<endl;
        }
        
        // This doesn't work with strings that have spaces in them. Fix in datum probably.
        
        map<string, datum>::iterator it;
        for(it = data.begin(); it != data.end(); it++)
        {
            f << (*it).second.get_print() << " " ;
        }
        f << endl;
        
        f.close();
    }
    
    void set_filename(string file)
    {
        filename=file;
    }
};

// Design assumption: Column headings will always be sorted in alphabetical order. ??

