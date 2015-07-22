#ifndef INPUTFILE_PARSE_H_DACCCNC
#define INPUTFILE_PARSE_H_DACCCNC

#include "mini_stream.h"

template <typename Index, typename Result>
void readLettingStatements(SimpleMap<std::string, MultiDimCon<Index, Result>> &conmap,
                           std::string fname) {
  MiniStream ms = MiniStream(fname, conmap);
}

#endif
