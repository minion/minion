// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef JSON_OUTPUT_CDSJ
#define JSON_OUTPUT_CDSJ

#include "../macros.hpp"
#include "json_parser.hpp"

#include <vector>

#ifndef NO_SYSTEM
#include <mutex>
#endif

struct JSONStreamer {
  // This is the stack of currently open terms, telling us if they have had a member printed yet.
  std::vector<bool> element_printed;
  std::vector<std::string> element_type;

  std::ostream* stream;

private:
  void maybe_print_comma() {
    if(!element_printed.empty()) {
      if(element_printed.back())
        (*stream) << ", ";
      else
        element_printed.back() = true;
    }
  }

  void open_term_internal(std::string s) {
    element_printed.push_back(false);
    if(s == "{")
      element_type.push_back("}");
    else if(s == "[")
      element_type.push_back("]");
    else
      abort();
    (*stream) << s;
  }

public:
  bool in_map() const {
    return element_type.size() > 0 && element_type.back() == "}";
  }

  void openVec() {
    D_ASSERT(!in_map());
    maybe_print_comma();
    open_term_internal("[");
  }

  void openMap() {
    D_ASSERT(!in_map());
    maybe_print_comma();
    open_term_internal("{");
  }

  template <typename T>
  void openVecWithKey(const T& t) {
    D_ASSERT(in_map());
    if(element_printed.back() == false)
      element_printed.back() = true;
    else
      (*stream) << ", ";
    (*stream) << "\"" << t << "\" : ";
    open_term_internal("[");
  }

  template <typename T>
  void openMapWithKey(const T& t) {
    D_ASSERT(in_map());
    if(element_printed.back() == false)
      element_printed.back() = true;
    else
      (*stream) << ", ";
    (*stream) << "\"" << t << "\" : ";
    open_term_internal("{");
  }

  void closeTerm() {
    D_ASSERT(element_type.size() > 0);
    (*stream) << element_type.back();
    if(element_type.back() == "}")
      (*stream) << "\n";
    element_printed.pop_back();
    element_type.pop_back();
    if(!element_printed.empty())
      element_printed.back() = true;
  }

  void closeMap() {
    D_ASSERT(element_type.size() > 0 && element_type.back() == "}");
    closeTerm();
  }

  void closeVec() {
    D_ASSERT(element_type.size() > 0 && element_type.back() == "]");
    closeTerm();
  }

  int depth() {
    return element_printed.size();
  }

  void newline() {
    (*stream) << "\n";
  }

  JSONStreamer() : stream(nullptr) {}

  bool isActive() const {
    return stream != nullptr;
  }

  JSONStreamer(std::ostream* o) : stream(o) {
    openMap();
  }

private:
  JSONStreamer(const JSONStreamer&);
  void operator=(const JSONStreamer&);

public:
  JSONStreamer(JSONStreamer&& js)
      : element_printed(std::move(js.element_printed)),
        element_type(std::move(js.element_type)),
        stream(std::move(js.stream)) {
    js.element_printed.clear();
    js.element_type.clear();
    js.stream = nullptr;
  }

  void operator=(JSONStreamer&& js) {
    D_ASSERT(stream == nullptr);
    element_printed = std::move(js.element_printed);
    element_type = std::move(js.element_type);
    stream = std::move(js.stream);
    js.element_printed.clear();
    js.element_type.clear();
    js.stream = nullptr;
  }

  ~JSONStreamer() {
    while(depth() > 0)
      closeTerm();
    if(stream) {
      (*stream) << std::flush;
    }
  }

  template <typename T, typename U>
  void mapElement(const T& t, const U& u) {
    D_ASSERT(element_type.back() == "}")
    maybe_print_comma();
    (*stream) << "\"" << t << "\" : ";
    json_dump(u, *stream);
    (*stream) << " ";
  }

  template <typename T>
  void vec_element(const T& t) {
    D_ASSERT(element_type.back() == "]");
    maybe_print_comma();
    json_dump(t, *stream);
  }
};

#endif
