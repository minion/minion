#ifndef _SIMPLE_MAP_HPP_FDS
#define _SIMPLE_MAP_HPP_FDS

/// SimpleMap provides a simple wrapper around std::map
/// which deals with making sure values are not added multiple times
/// and queried values are in the map
#include "basic_sys.hpp"
#include "tostring.hpp"

template <typename T, typename U, typename V>
const U& mapget(const std::map<T, U>& m, const V& t) {
  auto it = m.find(t);
  if(it == m.end()) {
    throw std::runtime_error(tostring(t) + " not defined in map");
  }
  return it->second;
}

template <typename T, typename U, typename V>
U& mapget(std::map<T, U>& m, const V& t) {
  auto it = m.find(t);
  if(it == m.end()) {
    throw std::runtime_error(tostring(t) + " not defined in map");
  }
  return it->second;
}

template <typename T, typename U>
class SimpleMap {
public:
  std::map<T, U> props;
  void add(T name, U prop) {
    if(props.count(name) > 0)
      throw std::runtime_error(tostring(name) + " already defined in map as " +
                               try_tostring(props.find(name)->second));
    props.insert(std::make_pair(name, prop));
  }

  bool has(const T& name) const {
    return props.count(name);
  }

  U& get(const T& name) {
    return mapget(props, name);
  }

  const U& get(const T& name) const {
    return mapget(props, name);
  }

  void remove(const T& name) {
    auto it = props.find(name);
    if(it == props.end())
      throw std::runtime_error(tostring(name) + " not defined in map");
    props.erase(it);
  }

  std::set<T> getPreimageSet() const {
    std::set<T> preimage;
    typedef typename std::map<T, U>::const_iterator map_it;
    for(map_it it = props.begin(); it != props.end(); ++it)
      preimage.insert(it->first);
    return preimage;
  }

  std::set<U> getImageSet() const {
    std::set<U> image;
    typedef typename std::map<T, U>::const_iterator map_it;
    for(map_it it = props.begin(); it != props.end(); ++it)
      image.insert(it->second);
    return image;
  }

  void addToMap(const SimpleMap& sm) {
    typedef typename std::map<T, U>::const_iterator map_it;
    for(map_it it = sm.props.begin(); it != sm.props.end(); ++it)
      this->add(it->first, it->second);
  }

  void addVecToMap(const std::vector<std::pair<T, U>>& vec) {
    typedef typename std::vector<std::pair<T, U>>::const_iterator ittype;
    for(ittype it = vec.begin(); it != vec.end(); ++it) {
      this->add(it->first, it->second);
    }
  }

  friend bool operator==(const SimpleMap& l, const SimpleMap& r) {
    return l.props == r.props;
  }

  friend bool operator<(const SimpleMap& l, const SimpleMap& r) {
    return l.props < r.props;
  }

  friend std::ostream& operator<<(std::ostream& os, const SimpleMap& sm) {
    return os << sm.props;
  }

  typedef typename std::map<T, U>::iterator iterator;
  typedef typename std::map<T, U>::const_iterator const_iterator;

  bool empty() const {
    return props.empty();
  }

  iterator begin() {
    return props.begin();
  }

  iterator end() {
    return props.end();
  }

  const_iterator begin() const {
    return props.begin();
  }

  const_iterator end() const {
    return props.end();
  }

  unsigned size() const {
    return props.size();
  }
};
#endif
