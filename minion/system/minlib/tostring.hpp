#ifndef TOSTRING_DSCDSCDS
#define TOSTRING_DSCDSCDS

#include "basic_sys.hpp"
#include "printing_containers.hpp"
#include "tiny-templatelib.hpp"
#include <ostream>
#include <sstream>
#include <string>

template <typename T>
T fromstring(const std::string& str) {
  std::istringstream iss(str);
  T obj;
  iss >> std::ws >> obj >> std::ws;
  if(!iss.eof())
    throw std::runtime_error("Failed conversion of '" + str + "'");
  return obj;
}

template <typename T>
bool is_fromstring(const std::string& str) {
  try {
    fromstring<T>(str);
  } catch(const std::runtime_error&) { return false; }

  return true;
}

template <typename T>
std::string tostring(const T& t) {
  std::ostringstream streamOut;
  streamOut << t;
  return streamOut.str();
}

template <typename T1, typename T2>
std::string tostring(const T1& t1, const T2& t2) {
  std::ostringstream streamOut;
  streamOut << t1 << " " << t2;
  return streamOut.str();
}

template <typename T>
decltype((std::cout << std::declval<T>(), std::true_type())) is_printable(const T& t);

std::false_type is_printable(AnyGrab);

template <typename T>
std::string try_tostring_impl(const T& t, std::true_type) {
  return tostring(t);
}

template <typename T>
std::string try_tostring_impl(const T& t, std::false_type) {
  return std::string("<no printer for ") + typeid(t).name() + ">";
}

template <typename T>
std::string try_tostring(const T& t) {
  decltype(is_printable(t)) d;
  return try_tostring_impl(t, d);
}

#endif
