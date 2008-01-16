/* Minion
* Copyright (C) 2006
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


struct MemOffset
{
  MemOffset(const MemOffset& b);
  void* ptr;
  void* get_ptr() const { return ptr; }
  void request_bytes(int i);
  MemOffset();
  ~MemOffset();  
};

struct VirtualMemOffset
{
  void* ptr;
  void* get_ptr() const { return ptr; }
  VirtualMemOffset();
  void operator=(const VirtualMemOffset&);
  VirtualMemOffset(MemOffset& b, int offset);
  VirtualMemOffset(const VirtualMemOffset&);
  ~VirtualMemOffset();  
};


struct MemoryBlock
{
  char* current_data;
  unsigned allocated_bytes;
  bool lock_m;
  bool final_lock_m;
  set<MemOffset*> offsets;
  MAP_TYPE<void*, pair<int,int> > offset_positions;
  MAP_TYPE<VirtualMemOffset*, pair<MemOffset*, int> > virtual_ptrs;
  MemoryBlock() : allocated_bytes(0), lock_m(false), final_lock_m(false)
  {}
  
  MemOffset get_bytes(unsigned byte_count)
  { 
    D_ASSERT(!lock_m);
    MemOffset new_mem;
    // :(
    char* ptr = new char[byte_count];
    std::fill(ptr, ptr + byte_count, 0);
    offset_positions[ptr] = make_pair(allocated_bytes, byte_count);
    new_mem.ptr = ptr;
    allocated_bytes += byte_count; 
    return new_mem;
  }
  
  
  void addToTracker(MemOffset* bto)
  {
    D_ASSERT(!lock_m);
    D_ASSERT(offsets.count(bto) == 0);
    offsets.insert(bto);
  }
  
  void removeFromTracker(MemOffset* bto)
  {
    D_ASSERT(!lock_m || Controller::finished);
    offsets.erase(bto);
  }
  
  void addToVirtualTracker(VirtualMemOffset* bto, const VirtualMemOffset* original)
  {
    if(!final_lock_m)
	{
      D_ASSERT(virtual_ptrs.count(bto) == 0);
      if(!lock_m)
      {
        if(virtual_ptrs.count(const_cast<VirtualMemOffset*>(original)))
	      virtual_ptrs[bto] = virtual_ptrs[const_cast<VirtualMemOffset*>(original)];
      }
	}
  }
  
  void addToVirtualTracker(VirtualMemOffset* bto, MemOffset* original, int offset)
  {
    if(!lock_m && !final_lock_m)
    {
	  D_ASSERT(virtual_ptrs.count(bto) == 0);
      virtual_ptrs[bto] = make_pair(original, offset);
	}
  }
  
  void removeFromVirtualTracker(VirtualMemOffset* bto)
  {
    if(!final_lock_m || Controller::finished)
	{ virtual_ptrs.erase(bto); }
  }
  
  
  void final_lock()
  { 
    final_lock_m = true;
    virtual_ptrs.clear();
    offsets.clear();
    offset_positions.clear();
  }
  
  void lock()
  {
    D_ASSERT(!lock_m);
    D_ASSERT(!final_lock_m);
    lock_m = true;
    current_data = new char[allocated_bytes];
    MAP_TYPE<void*, pair<int,int> > offset_positions_backup(offset_positions);
    // std::fill(current_data, current_data + allocated_bytes,0);
    // Code like this makes baby Jesus cry, but is suprisingly legal C++
    // The apparently equivalent "offsets[i]->ptr = (int)offset" isn't.
    for(set<MemOffset*>::iterator it=offsets.begin();it!=offsets.end();++it)
    {
      char* old_ptr = static_cast<char*>((*it)->ptr);
      D_ASSERT(offset_positions.count(old_ptr) == 1);
      (*it)->ptr = current_data + offset_positions[old_ptr].first;
      copy(old_ptr, old_ptr + offset_positions[old_ptr].second,
	   static_cast<char*>((*it)->ptr));
      delete[] old_ptr;
      D_DATA(offset_positions.erase(old_ptr));
    }
    D_ASSERT(offset_positions.size() == 0);
    
    for(MAP_TYPE<VirtualMemOffset*, pair<MemOffset*, int> >::iterator it = virtual_ptrs.begin(); 
	it != virtual_ptrs.end(); ++it)
    {
      VirtualMemOffset* ptr = it->first;
      const MemOffset* master = it->second.first;
      int offset_val = it->second.second;
      D_ASSERT(master->ptr != NULL);
      ptr->ptr = static_cast<char*>(master->ptr) + offset_val;
    }
    
  }
  
   
};

VARDEF(MemoryBlock memory_block);

inline MemOffset::MemOffset() : ptr(NULL)
{memory_block.addToTracker(this);}

inline VirtualMemOffset::VirtualMemOffset() : ptr(NULL)
{ }

inline MemOffset::MemOffset(const MemOffset& b) : ptr(b.ptr)
{memory_block.addToTracker(this);}

inline VirtualMemOffset::VirtualMemOffset(const VirtualMemOffset& b) : ptr(b.ptr)
{ 
  memory_block.addToVirtualTracker(this, &b); 
}

inline void VirtualMemOffset::operator=(const VirtualMemOffset& b)
{
  memory_block.removeFromVirtualTracker(this);
  memory_block.addToVirtualTracker(this, &b); 
}

inline VirtualMemOffset::VirtualMemOffset(MemOffset& b, int offset) : ptr(b.ptr)
{ memory_block.addToVirtualTracker(this, &b, offset); }

inline MemOffset::~MemOffset()
{ memory_block.removeFromTracker(this); }

inline VirtualMemOffset::~VirtualMemOffset()
{ memory_block.removeFromVirtualTracker(this); }

inline void MemOffset::request_bytes(int i)
{ *this = memory_block.get_bytes(i); }
