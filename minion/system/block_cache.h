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

#include <vector>

struct BlockCache
{
  std::vector<char*> blocks;
  
  BlockCache(SysInt size)
  { blocks.resize(size); }
  
  char* doMalloc(size_t size)
  {
    // This is because realloc/malloc will sometimes return 0 with size=0
    if(size == 0)
        return (char*)(0);

    if(blocks.empty())
    {
      char* ptr = static_cast<char*>(malloc(size));
      if(ptr == NULL)
      { D_FATAL_ERROR("Malloc failed - Memory exausted! Aborting."); }
      return ptr;      
    }
    else
    {      
      char* ret = blocks.back();
      blocks.pop_back();
      char* ptr = static_cast<char*>(realloc(ret, size));
      if(ptr == NULL)
      { D_FATAL_ERROR("Realloc failed - Memory exausted! Aborting."); }
      return ptr;
    }
  }
  
  void do_free(char* ptr)
  {
    if(blocks.size() == blocks.capacity())
      free(ptr);
    else
      blocks.push_back(ptr);
  }
  
  ~BlockCache()
  { for(SysInt i = 0; i < blocks.size(); ++i) free(blocks[i]); }
};
