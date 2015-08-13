#include "../minion.h"
#include "cheap_stream.h"
#include "../tuple_container.h"

inline TupleList* grab_tuplelist(CheapStream& cs) {
  std::string name;
  cs >> name;
  SysInt len, count;
  cs >> len >> count;
  TupleList* tl = new TupleList(len, count);
  DomainInt* ptr = tl->getPointer();
  for(SysInt i = 0; i < len * count; ++i) {
    cs >> ptr[i];
  }

  return tl;
}

inline std::vector<TupleList*> tiny_parser(istream& is) {
  CheapStream cs(is);
  while(cs.peek() == '#') {
    std::string s = cs.getline();
  }

  std::string s = cs.getline();
  assert(s == "MINION 3");
  s = cs.getline();
  assert(s == "**TUPLELIST**");
  std::vector<TupleList*> tl;
  tl.push_back(grab_tuplelist(cs));
  tl.push_back(grab_tuplelist(cs));
  return tl;
}