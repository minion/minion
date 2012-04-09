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

#ifndef CHEAPSTREAM_H
#define CHEAPSTREAM_H

#include <algorithm>
#include <sstream>
#include <istream>
#include <string>

#ifdef P
#undef P
#endif

#define P(x)
//#define P(x) cout << x << endl

class CheapStream
{
    const char* stream_start;
    const char* stream_end;
    const char* stream_pos;
    
    std::string s;
    
public:    
    
    bool fail_flag;
    
    int get_raw_pos()
    { return stream_pos - stream_start; }
        
    void reset_stream()
    {
        stream_pos = stream_start;
        fail_flag = false;
    }
    
    CheapStream(const char* _stream_start, const char* _stream_end) :
    stream_start(_stream_start), stream_end(_stream_end), stream_pos(_stream_start),
    fail_flag(false)
    { }
    
    template<typename IStream>
    CheapStream(IStream& i, const char* filename = "") : fail_flag(false)
    {
        std::ostringstream iss;
        iss << i.rdbuf();
        s = iss.str();
        if(!s.empty())
        {
            stream_start = &*s.begin();
            stream_end = &*s.begin() + s.length();
            stream_pos = stream_start;
        }
        else
        {
            stream_start = stream_end = stream_pos = NULL;
        }
    }
    
    
    bool fail()
    { return fail_flag; }
    
    bool operator!()
    { return stream_pos == NULL; }
        
    char get()
    {
        char x = *stream_pos;
        P("Get '" << x << "'");
        stream_pos++;
        return x;
    }
    
    char peek()
    {
        return *stream_pos;
    }
    
    void putback(char c)
    { stream_pos--; }
        
    bool eof()
    { return stream_pos == stream_end; }
        
    
    void getline(char* buf, int buf_length, char deliminator = '\n')
    {
        int length = std::min((int)buf_length, (int)(stream_end - stream_pos));
        P(length << ":");
        for(int i = 0; i < length; ++i)
        {
            P(i << ":" << *stream_pos << ":");
             if(*stream_pos == deliminator)
             {
                 *buf = '\0';
                 stream_pos++;
                 return;
             }
            *buf = *stream_pos;
            buf++; stream_pos++;           
        }
        *buf = '\0';
    }
    
};


void operator>>(CheapStream& cs, int& ret)
{
    int neg_flag = 1;
    
    long long i;
    long long limit = 1 << 30;

    // Set initial value
    i = 1;
    while(isspace(cs.peek()))
        cs.get();
    
    if(cs.peek() == '-')
    {
        cs.get();
        neg_flag = -1;
    }
    
    if(cs.peek() >= '0' && cs.peek() <= '9')
    {
        i *= (cs.get() - '0');
    }
    else
    {
        cs.fail_flag = true;
        return;
    }
    
    while(cs.peek() >= '0' && cs.peek() <= '9')
    {
        char c = cs.get();
        i = i * 10 + c - '0';
        if(i > limit)
        {
            std::cerr << "Magnitude of number too large!\n";
            cs.fail_flag = true;
            return;
        }
        P(": '" << c << "' :" << i);
    }
    
    ret = i * neg_flag;
    P(">>int Got: " << i);
}


void operator>>(CheapStream& cs, char& c)
{
    while(isspace(cs.peek()))
      cs.get();
    c = cs.get();
}
#endif
