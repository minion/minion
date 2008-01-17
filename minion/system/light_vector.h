#define LIGHT_VECTOR
#ifdef VECSIZE
template<typename T>
class light_vector
{
  size_t vec_size;
  T* data;

public:

  typedef T value_type;
  typedef T* iterator;
  
  size_t size() const
  { return vec_size; }
  
  light_vector() : vec_size(-1), data(0)
  { }
  
  light_vector(size_t _size):
  vec_size(_size), data(new T[_size])
  { }
  
  light_vector(const light_vector& t) :
  vec_size(t.vec_size), data(new T[t.vec_size])
  {
    for(int i = 0; i < vec_size; ++i)
      data[i] = t.data[i];
  }
  
  void operator=(const light_vector& t)
  {
	if(&t == this)
	  return;
	delete data;
	vec_size = t.vec_size;
	data = new T[vec_size];
	for(int i = 0; i < vec_size; ++i)
	  data[i] = t.data[i];
  }
	
  T* begin()
  { return data; }
  
  T* end()
  { return data + vec_size; }
  
  void pop_back()
  { 
	D_ASSERT(vec_size >= 1);
	--vec_size; 
  }
  
  T& back()
  { return *(data + vec_size - 1); }
  
  T& operator[](int i)
  { 
	D_ASSERT(i >= 0 && i < vec_size);
	return *(data + i); 
  }
  
  const T& operator[](int i) const
  { 
	D_ASSERT(i >= 0 && i < vec_size);
	return *(data + i); 
  }
  
  ~light_vector()
  { 
	delete[] data;
    data = (T*)1;
  }
};

#else

template<typename T>
class light_vector
{

  T* data;
  T* data_end;
  
public:
	
  typedef T value_type;
  typedef T* iterator;
  
  size_t size() const
  { return data_end - data; }
  
  light_vector() :  data(0), data_end(0)
  { }
  
  light_vector(size_t _size):
	 data(new T[_size]), data_end(data + _size)
  { }
  
  light_vector(const light_vector& t) :
	data(new T[t.data_end - t.data])
  {
	  int vec_size = t.data_end - t.data;
	  data_end = data + vec_size;
	  for(int i = 0; i < vec_size; ++i)
		data[i] = t.data[i];
  }
  
  void operator=(const light_vector& t)
  {
	if(&t == this)
	  return;
	delete data;
	int vec_size = t.end - t.data_end;
	data = new T[vec_size];
	end = data + vec_size;
	for(int i = 0; i < vec_size; ++i)
	  data[i] = t.data[i];
  }
  
  T* begin()
  { return data; }
  
  T* end()
  { return data_end; }
  
  void pop_back()
  { 
	D_ASSERT(data_end > data);
	--data_end; 
  }
  
  T& back()
  { return *(data_end - 1); }
  
  T& operator[](int i)
  { 
	D_ASSERT(i >= 0 && i < (data_end - data));
	return *(data + i); 
  }
  
  const T& operator[](int i) const
  { 
	D_ASSERT(i >= 0 && i < (data_end - data));
	return *(data + i); 
  }
  
  ~light_vector()
  { 
	delete[] data;
    data = (T*)1;
  }
};

#endif
