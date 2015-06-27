#undef ANYVARREF_MAINCLASS

#define  FUN0(RET,NAME) virtual RET NAME() = 0;
#define CFUN0(RET,NAME) virtual RET NAME() const = 0;
#define  FUN1(RET,NAME,ARG) virtual RET NAME(ARG) = 0;
#define CFUN1(RET,NAME,ARG) virtual RET NAME(ARG) const = 0;
#define  FUN2(RET,NAME,ARG1,ARG2) virtual RET NAME(ARG1,ARG2) = 0;
#define CFUN2(RET,NAME,ARG1,ARG2) virtual RET NAME(ARG1,ARG2) const = 0;

#define CLASS(NAME) struct NAME ## _Abstract { \
	virtual void do_output(std::ostream& o) = 0; \
	virtual ~ NAME ## _Abstract() {}
#define ENDCLASS };

#include ANYREF_HEADER

#undef FUN0
#undef CFUN0
#undef FUN1
#undef CFUN1
#undef FUN2
#undef CFUN2
#undef CLASS
#undef ENDCLASS


#define  FUN0(RET,NAME) RET NAME() { return data.NAME(); }
#define CFUN0(RET,NAME) RET NAME() const { return data.NAME(); }
#define  FUN1(RET,NAME,ARG) RET NAME(ARG a) { return data.NAME(a); }
#define CFUN1(RET,NAME,ARG) RET NAME(ARG a) const { return data.NAME(a); }
#define  FUN2(RET,NAME,ARG1,ARG2) RET NAME(ARG1 a1, ARG2 a2) { return data.NAME(a1,a2); }
#define CFUN2(RET,NAME,ARG1,ARG2) RET NAME(ARG1 a1, ARG2 a2) const { return data.NAME(a1,a2); }

#define CLASS(NAME) \
  template<typename T> \
  struct NAME ## _Concrete : public NAME ## _Abstract { \
  T data; \
  virtual void do_output(std::ostream& o) { o << data; } \
  NAME ## _Concrete() : data() {} \
  NAME ## _Concrete(const T& t) : data(t) {} \

#define ENDCLASS };

#include ANYREF_HEADER

#undef FUN0
#undef CFUN0
#undef FUN1
#undef CFUN1
#undef FUN2
#undef CFUN2
#undef CLASS
#undef ENDCLASS


#define  FUN0(RET,NAME) RET NAME() { return data->NAME(); }
#define CFUN0(RET,NAME) RET NAME() const { return data->NAME(); }
#define  FUN1(RET,NAME,ARG) RET NAME(ARG a) { return data->NAME(a); }
#define CFUN1(RET,NAME,ARG) RET NAME(ARG a) const { return data->NAME(a); }
#define  FUN2(RET,NAME,ARG1,ARG2) RET NAME(ARG1 a1, ARG2 a2) { return data->NAME(a1,a2); }
#define CFUN2(RET,NAME,ARG1,ARG2) RET NAME(ARG1 a1, ARG2 a2) const { return data->NAME(a1,a2); }

#define CLASS(NAME) \
  struct NAME { \
  friend std::ostream& operator<<(std::ostream& o, const NAME& so) \
  { so.data->do_output(o); return o; } \
  SHARED_PTR< NAME ## _Abstract > data; \
  template<typename T> \
  NAME(const T& t) \
  { data = SHARED_PTR<NAME ## _Abstract>(new NAME##_Concrete <T>(t)); } \
  NAME(const NAME& n) : data(n.data) {} \
  NAME(NAME&& n) : data(n.data) {} \
  NAME() {} \
\
  NAME& operator=(const NAME& n ) \
  { this->data = n.data; return *this; } \
\
  NAME& operator=(NAME&& n ) \
  { this->data = std::move(n.data); return *this; }

#define ANYVARREF_MAINCLASS

#define ENDCLASS };

#include ANYREF_HEADER

#undef FUN0
#undef CFUN0
#undef FUN1
#undef CFUN1
#undef FUN2
#undef CFUN2
#undef CLASS
#undef ENDCLASS
