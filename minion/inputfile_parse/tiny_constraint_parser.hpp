#include "../minion.h"
#include "../tuple_container.h"
#include "cheap_stream.h"

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
