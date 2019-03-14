/** \weakgroup MinLib
 * @{
 */

#ifndef MULTIDIM_CON_BOBAS_LK
#define MULTIDIM_CON_BOBAS_LK

#include "basic_sys.hpp"
#include "containers.hpp"
#include "interval.hpp"
#include "macros.hpp"
#include "tostring.hpp"

/// Thrown when a \ref MultiDimCon is dereferenced in an invalid way.
class InvalidDerefException : public std::exception {
  std::string error;

public:
  virtual const char* what() const throw() {
    return error.c_str();
  }

  InvalidDerefException(std::string _error) : error(_error) {}

  ~InvalidDerefException() throw() {}
};

template <typename Index, typename Result>
struct MultiDimCon {
  typedef Index index_type;
  typedef Result result_type;

  std::map<std::vector<Index>, Result> indices;
  std::vector<Index> array_bounds;

  std::string name;

  void setName(std::string _name) {
    name = _name;
  }

  const std::vector<Index>& getBounds() const {
    return array_bounds;
  }

  const std::map<std::vector<Index>, Result>& getIndices() const {
    return indices;
  }

  explicit MultiDimCon(const std::vector<Index>& _bounds)
      : array_bounds(_bounds), name("<unknown>") {
    for(size_t i = 0; i < array_bounds.size(); ++i) {
      D_ASSERT(array_bounds[i] >= 0);
    }
  }

  template <typename T>
  explicit MultiDimCon(const std::vector<Index>& _bounds, T* values)
      : array_bounds(_bounds), name("<unknown>") {
    for(size_t i = 0; i < array_bounds.size(); ++i) {
      D_ASSERT(array_bounds[i] >= 0);
    }

    std::vector<int> v(array_bounds.size(), 0);

    do {
      indices[v] = *values;
      values++;
    } while(incrementVector(v, array_bounds));
  }

  template <typename Index2, typename Result2>
  MultiDimCon(const MultiDimCon<Index2, Result2>& mdc)
      : indices(mdc.indices.begin(), mdc.indices.end()),
        array_bounds(mdc.array_bounds.begin(), mdc.array_bounds.end()),
        name(mdc.name) {}

  MultiDimCon(const Result& r) : name("<unknown single variable>") {
    indices[makeVec<Index>()] = r;
  }

  /// Returns the arity of the members of the container.
  size_t arity() const {
    return array_bounds.size();
  }

  bool empty() const {
    return indices.empty();
  }

  void checkArrayIndices(const std::vector<Index>& assign) const {
    if(assign.size() != array_bounds.size()) {
      std::string error = tostring(array_bounds.size()) + "dimensions, but deref used only " +
                          tostring(assign.size());
      if(!name.empty())
        error += " in " + name;
      throw InvalidDerefException(error);
    }

    for(size_t i = 0; i < assign.size(); ++i) {
      if(assign[i] < 0 || assign[i] >= array_bounds[i]) {
        std::string error =
            "The " + tostring(i) + "th dimension accessed by invalid value " + tostring(assign[i]);
        if(!name.empty())
          error += " in " + name;
        throw InvalidDerefException(error);
      }
    }
  }

  void add(const std::vector<Index>& index, const Result& result) {
    checkArrayIndices(index);
    if(indices.count(index) != 0)
      throw InvalidDerefException(tostring(index) + "Already defined in " + name);

    indices.insert(make_pair(index, result));
  }

  /** @brief Get value at index.
   *  @param indices The index to retrieve
   *  Throws an exception if the index does not exist.
   */
  Result get(const std::vector<Index>& index) const {
    checkArrayIndices(index);
    auto it = indices.find(index);
    if(it == indices.end())
      throw InvalidDerefException(tostring(index) + " not defined in " + name);
    return it->second;
  }

  /// Check if @a index exists in the map.
  bool exists(const std::vector<Index>& index) const {
    checkArrayIndices(index);
    return indices.count(index);
  }

  /** @brief Destructively removes indices from the map.
   *  @param indices A boolean array, which is false on those arrays to remove.
   *  Asserts if this results in two values mapping to the same reduction.
   */
  void remove_indices(const std::vector<bool>& vb) {
    if(array_bounds.size() != vb.size()) {
      std::string error = "Array " + name + " with " + tostring(array_bounds.size()) +
                          " dimensions derefed using " + tostring(vb.size()) + " indices.";
      throw InvalidDerefException(error);
    }

    std::vector<Index> new_array_bounds;
    for(size_t i = 0; i < vb.size(); ++i) {
      if(vb[i])
        new_array_bounds.push_back(array_bounds[i]);
    }
    array_bounds = new_array_bounds;

    std::map<std::vector<Index>, Result> new_map;

    // Bad behaviour, but we don't define this in the function, else we would
    // have to keep reallocating memory.
    std::vector<Index> v;
    for(auto it = indices.begin(); it != indices.end(); ++it) {
      v.clear();
      for(size_t i = 0; i < vb.size(); ++i) {
        if(vb[i])
          v.push_back(it->first[i]);
      }
      // I believe this has to be false.
      D_ASSERT(new_map.count(v) == 0);
      new_map.insert(make_pair(v, it->second));
    }
    indices = new_map;
  }

  /** @brief Project out not required values
   *  @param ind	   A range for each index, which tells us which values
   * to
   * keep.
   *  @returns 	   The projected container
   *  Asserts if this results in two values mapping to the same reduction.
   */
  MultiDimCon<Index, Result> project(std::vector<INTERVAL<Index>> ind) const {
    D_ASSERT(ind.size() == array_bounds.size());

    std::vector<Index> new_array_bounds;
    for(size_t i = 0; i < ind.size(); ++i) {
      ind[i] = clamp_interval(ind[i], 0, array_bounds[i] - 1);
      if(first(ind[i]) < 0 || last(ind[i]) >= array_bounds[i])
        throw InvalidDerefException("Attempt to project index " + tostring(i) +
                                    " with invalid range " + tostring(ind[i]) + " in " + name);
      new_array_bounds.push_back(last(ind[i]) - first(ind[i]) + 1);
    }

    MultiDimCon<Index, Result> mdc_new(new_array_bounds);
    // Bad behaviour, but we don't define this in the function, else we would
    // have to keep reallocating memory.
    std::vector<Index> v;
    v.resize(array_bounds.size());

    std::vector<Index> it = initializeVector_from_intervals(ind);
    do {
      auto pos = indices.find(it);
      if(pos != indices.end()) {
        for(size_t i = 0; i < ind.size(); ++i)
          v[i] = (pos->first)[i] - first(ind[i]);
        mdc_new.add(v, pos->second);
      }
    } while(incrementVector_from_intervals(it, ind));

    /*		for(auto it = indices.begin(); it != indices.end(); ++it)
                    {
                            bool inrange = true;
                            for(size_t i = 0; inrange && i < ind.size(); ++i)
                            {
                                    if(!contains(ind[i], (it->first)[i]))
                                            inrange = false;
                                    else
                                            v[i] = (it->first)[i] -
       first(ind[i]);
                            }

                            if(inrange)
                            {
                                    mdc_new.add(v, it->second);
                            }
                    }*/
    return mdc_new;
  }

  friend std::ostream& operator<<(std::ostream& o, const MultiDimCon& mdc) {
    return o << "bounds:" << mdc.array_bounds << "  " << mdc.indices;
  }

  template <typename I, typename R>
  friend MultiDimCon<I, R> flatten(const MultiDimCon<I, R>& in);

  template <typename I, typename R>
  friend int mdcIndexSize(const MultiDimCon<I, R>& in, int dim);

  template <typename I, typename R>
  friend std::vector<R> mdc_join_and_merge(const std::vector<MultiDimCon<I, R>>& vec);

  template <typename I, typename R>
  friend bool operator==(const MultiDimCon<I, R>& l, const MultiDimCon<I, R>& r);

  template <typename I, typename R>
  friend MultiDimCon<I, R> mdc_join(const std::vector<MultiDimCon<I, R>>& v);

  template <typename I, typename R>
  friend MultiDimCon<I, R> mdc_make(const std::vector<R>& v);
};

template <typename I, typename R>
int mdcIndexSize(const MultiDimCon<I, R>& in, int dim) {
  if(dim < 0 || dim >= in.array_bounds.size()) {
    std::string error = "Tried to get size of index " + tostring(dim) + " from array with " +
                        tostring(in.array_bounds.size()) + " dimensions, name: " + in.name;
    throw InvalidDerefException(error);
  }
  return in.array_bounds[dim];
}

template <typename Index, typename Result>
MultiDimCon<Index, Result> flatten(const MultiDimCon<Index, Result>& in) {
  MultiDimCon<Index, Result> mdc(makeVec<Index>(in.indices.size()));
  Index pos = 0;
  for(auto it = in.indices.begin(); it != in.indices.end(); ++it, ++pos) {
    mdc.add(makeVec(pos), it->second);
  }
  return mdc;
}

template <typename Index, typename Result>
std::vector<Result> mdc_join_and_merge(const std::vector<MultiDimCon<Index, Result>>& vec) {
  std::vector<Result> ret;

  for(unsigned i = 0; i < vec.size(); ++i) {
    for(auto it = vec[i].indices.begin(); it != vec[i].indices.end(); ++it) {
      ret.push_back(it->second);
    }
  }
  return ret;
}

template <typename Index, typename Result>
bool operator==(const MultiDimCon<Index, Result>& l, const MultiDimCon<Index, Result>& r) {
  return l.indices == r.indices;
}

template <typename Index, typename Result>
DOM_NOINLINE MultiDimCon<Index, Result> mdc_join(const std::vector<MultiDimCon<Index, Result>>& v) {
  if(v.empty()) {
    return MultiDimCon<Index, Result>(makeVec<Index>(0));
  }

  std::vector<Index> new_bounds = v[0].array_bounds;
  for(size_t i = 1; i < v.size(); ++i) {
    if(v[0].array_bounds.size() != v[i].array_bounds.size()) {
      D_ASSERT(0 && "Attempt to join matrices of different dimensions!");
    }
    for(size_t j = 0; j < new_bounds.size(); ++j)
      new_bounds[j] = std::max(new_bounds[j], v[i].array_bounds[j]);
  }

  new_bounds.insert(new_bounds.begin(), v.size());
  MultiDimCon<Index, Result> mdc(new_bounds);
  // We do this to reduce memory reallocations
  std::vector<Index> tempVec;
  for(size_t i = 0; i < v.size(); ++i) {
    for(auto it = v[i].indices.begin(); it != v[i].indices.end(); ++it) {
      tempVec = it->first;
      tempVec.insert(tempVec.begin(), i);
      mdc.add(tempVec, it->second);
    }
  }
  return mdc;
}

template <typename Index, typename Result>
DOM_NOINLINE MultiDimCon<Index, Result> mdc_make(const std::vector<Result>& v) {
  std::vector<int> temp(1);

  temp[0] = v.size();
  MultiDimCon<Index, Result> mdc(temp);

  for(size_t i = 0; i < v.size(); ++i) {
    temp[0] = i;
    mdc.add(temp, v[i]);
  }

  return mdc;
}

template <typename Index, typename Result, typename... Args>
DOM_NOINLINE MultiDimCon<Index, Result> project(const MultiDimCon<Index, Result>& mdc,
                                                Args... args) {
  /// Projection takes three steps.
  /// A) Project out tuples we don't want.
  /// B) Remove indices that are projected out
  ///    and move index values down where approriate.

  std::vector<bool> is_intervals = makeVec<bool>(is_interval<Args>::val...);
  auto intervals = intervalise_list<Index>(args...);

  if(is_intervals.size() != mdc.arity()) {
    std::string error = mdc.name + " has " + tostring(mdc.arity()) +
                        "dimensions, but deref used only " + tostring(is_intervals.size());
    throw InvalidDerefException(error);
  }

  MultiDimCon<Index, Result> new_con = mdc.project(intervals);
  new_con.remove_indices(is_intervals);
  return new_con;
}

template <typename Index, typename Result>
DOM_NOINLINE MultiDimCon<Index, Result> project(const MultiDimCon<Index, Result>& mdc) {
  return mdc;
}

template <typename Index, typename Result>
DOM_NOINLINE MultiDimCon<Index, Result> flat(const MultiDimCon<Index, Result>& mdc) {
  return mdc.flatten();
}

// template<typename Index, typename Result>
// MultiDimCon<Index, Result> mdc_convert(const MultiDimCon<Index, Result>& mdc)
//{ return mdc; }

template <typename Index, typename Result, typename Index2, typename Result2>
DOM_NOINLINE MultiDimCon<Index, Result> mdc_convert(const MultiDimCon<Index2, Result2>& mdc) {
  return mdc;
}

template <typename Index, typename Result, typename Result2>
DOM_NOINLINE MultiDimCon<Index, Result> mdc_convert(const Result2& r) {
  return MultiDimCon<Index, Result>(static_cast<Result>(r));
}

#endif
/** @}
 */
