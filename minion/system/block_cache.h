
struct BlockCache
{
  vector<char*> blocks;
  
  BlockCache(int size)
  { blocks.resize(size); }
  
  char* do_malloc(size_t size)
  {
    if(blocks.empty())
      return static_cast<char*>(malloc(size));
    else
    {
      char* ret = blocks.back();
      blocks.pop_back();
      return static_cast<char*>(realloc(ret, size));
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
  { for(int i = 0; i < blocks.size(); ++i) free(blocks[i]); }
};