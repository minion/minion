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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifndef CHEAPSTREAM_H
#define CHEAPSTREAM_H

#include <algorithm>
#include <istream>
#include <sstream>
#include <string>

#ifdef P
#undef P
#endif

#define P(x)
//#define P(x) cout << x << endl

class CheapStream {
  const char* streamStart;
  const char* streamEnd;
  const char* streamPos;

  std::string s;

public:
  bool failFlag;

  SysInt getRawPos() {
    return streamPos - streamStart;
  }

  void resetStream() {
    streamPos = streamStart;
    failFlag = false;
  }

  CheapStream(const char* _streamStart, const char* _streamEnd)
      : streamStart(_streamStart),
        streamEnd(_streamEnd),
        streamPos(_streamStart),
        failFlag(false) {}

  template <typename IStream>
  CheapStream(IStream& i, const char* filename = "") : failFlag(false) {
    std::ostringstream iss;
    iss << i.rdbuf();
    s = iss.str();
    if(!s.empty()) {
      streamStart = &*s.begin();
      streamEnd = &*s.begin() + s.length();
      streamPos = streamStart;
    } else {
      streamStart = streamEnd = streamPos = NULL;
    }
  }

  bool fail() {
    return failFlag;
  }

  bool operator!() {
    return streamPos == NULL;
  }

  char get() {
    char x = *streamPos;
    P("Get '" << x << "'");
    streamPos++;
    return x;
  }

  char peek() {
    return *streamPos;
  }

  void putback(char c) {
    streamPos--;
  }

  bool eof() {
    return streamPos == streamEnd;
  }

  string getline(char deliminator = '\n') {
    std::vector<char> output;
    while(streamPos != streamEnd) {
      if(*streamPos == deliminator) {
        streamPos++;
        return string(output.begin(), output.end());
      } else {
        output.push_back(*streamPos);
        streamPos++;
      }
    }
    // reached end of stream
    return string(output.begin(), output.end());
  }
};

template <typename T>
void getNum(CheapStream& cs, T& ret) {
  SysInt neg_flag = 1;

  long long i = 1;
  long long limit = std::numeric_limits<SysInt>::max() / 2;

  while(isspace(cs.peek()))
    cs.get();

  if(cs.peek() == '-') {
    cs.get();
    neg_flag = -1;
  }

  if(cs.peek() >= '0' && cs.peek() <= '9') {
    i *= (cs.get() - '0');
  } else {
    cs.failFlag = true;
    return;
  }

  while(cs.peek() >= '0' && cs.peek() <= '9') {
    char c = cs.get();
    i = i * 10 + c - '0';
    if(i > limit) {
      std::cerr << "Magnitude of number too large!\n";
      cs.failFlag = true;
      return;
    }
    P(": '" << c << "' :" << i);
  }

  ret = i * neg_flag;
  P(">>SysInt Got: " << i);

  return;
}

inline CheapStream& operator>>(CheapStream& cs, SysInt& si) {
  getNum(cs, si);
  return cs;
}

template <typename T>
CheapStream& operator>>(CheapStream& cs, Wrapper<T>& ret) {
  T t = 0;
  cs >> t;
  ret = Wrapper<T>(t);
  return cs;
}

inline CheapStream& operator>>(CheapStream& cs, char& c) {
  while(isspace(cs.peek()))
    cs.get();
  c = cs.get();
  return cs;
}

inline CheapStream& operator>>(CheapStream& cs, std::string& s) {
  while(!cs.eof() && isspace(cs.peek()))
    cs.get();
  while(!cs.eof() && !isspace(cs.peek())) {
    s += cs.get();
  }
  return cs;
}
#endif
