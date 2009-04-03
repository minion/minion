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

class TableOut
{
    private:
    // All the data for this run is kept in the map    
    map<string, string> data;
    string filename;
    
    // Don't allow copying!
    TableOut(const TableOut& t); 
    public:
    TableOut() {}
      
    template <class Valtype>
    void set(string propname, Valtype value)
    {
        // create a new, or overwrite the, entry with name propname
        data[propname] = to_string(value);
    }
    
    void debug_printall()
    {
        std::map<string, string>::iterator it;
        
        for(it = data.begin(); it != data.end(); it++)
        {
            cout<< (*it).first << "," << (*it).second << endl;
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
            
            map<string, string>::iterator it;
            for(it = data.begin(); it != data.end(); it++)
            {
                f<<  "\"" << (*it).first << "\" " ;
            }
            f <<endl;
        }
        
        // This doesn't work with strings that have spaces in them. 
        
        map<string, string>::iterator it;
        for(it = data.begin(); it != data.end(); it++)
        {
            f << (*it).second << " " ;
        }
        f << endl;
        
        f.close();
    }
    
    void set_filename(string file)
    {
        filename=file;
    }
};

// Provide a global singleton of the above class. Not threadsafe!
inline TableOut& getTableOut()
{
  static TableOut t;
  return t;
}


// Design assumption: Column headings will always be sorted in alphabetical order. ??

