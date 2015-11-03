#ifndef TRIES_MINLIB_VFD
#define TRIES_MINLIB_VFD

#include "minlib/minlib.hpp"

template <typename T>
struct TrieObj {
  T val;
  TrieObj* ptr;
};

namespace TrieBuilder {
template <typename T>
struct earlyTrieObj {
  T val;
  int offset;
};

// Find how many values there are for index 'depth' between tuples
// start_pos and end_pos.
template <typename Tuples>
int get_distinct_values(const Tuples& tuples, int start_pos, int end_pos, int depth) {
  int current_val = tuples[start_pos][depth];
  int found_values = 1;
  for(int i = start_pos; i < end_pos; ++i) {
    if(current_val != tuples[i][depth]) {
      current_val = tuples[i][depth];
      found_values++;
    }
  }
  return found_values;
}

template <typename Value, typename Tuples>
void build_early_trie(std::vector<earlyTrieObj<Value>>& initial_trie, const Tuples& tuples,
                      int start_pos, int end_pos, int depth) {
  const bool last_stage = (depth == (int)tuples[0].size() - 1);
  assert(depth <= (int)tuples[0].size() - 1);
  assert(start_pos <= end_pos);
  int values = get_distinct_values(tuples, start_pos, end_pos, depth);

  int start_section = initial_trie.size();
  // Make space for this list of values.
  // '+1' is for end marker.
  initial_trie.resize(initial_trie.size() + values + 1);

  int current_val = tuples[start_pos][depth];
  int current_start = start_pos;
  int num_of_val = 0;

  for(int i = start_pos; i < end_pos; ++i) {
    if(current_val != tuples[i][depth]) {
      initial_trie[start_section + num_of_val].val = current_val;
      if(last_stage) {
        initial_trie[start_section + num_of_val].offset = -1;
      } else {
        initial_trie[start_section + num_of_val].offset = initial_trie.size();
        build_early_trie(initial_trie, tuples, current_start, i, depth + 1);
      }
      current_val = tuples[i][depth];
      current_start = i;
      num_of_val++;
    }
  }

  // Also have to cover last stretch of values.
  initial_trie[start_section + num_of_val].val = current_val;
  if(last_stage)
    initial_trie[start_section + num_of_val].offset = -1;
  else {
    initial_trie[start_section + num_of_val].offset = initial_trie.size();
    build_early_trie(initial_trie, tuples, current_start, end_pos, depth + 1);
  }

  assert(num_of_val + 1 == values);
  initial_trie[start_section + values].val = std::numeric_limits<Value>::max();
  initial_trie[start_section + values].offset = -1;
}

template <typename Value>
TrieObj<Value>* build_final_trie(const std::vector<earlyTrieObj<Value>>& early_trie) {
  TrieObj<Value>* trie = new TrieObj<Value>[early_trie.size()];
  for(size_t i = 0; i < early_trie.size(); ++i) {
    trie[i].val = early_trie[i].val;
    D_ASSERT(early_trie[i].offset >= -1);
    if(early_trie[i].offset == -1)
      trie[i].ptr = 0;
    else
      trie[i].ptr = trie + early_trie[i].offset;
  }

  return trie;
}

} // end namespace TrieBuilder

template <typename Tuples>
TrieObj<int>* build_trie(const Tuples& tuples) {
  if(tuples.empty()) {
    return 0;
  } else if(tuples[0].empty()) {
    TrieObj<int>* t = new TrieObj<int>;
    t->val = std::numeric_limits<int>::max();
    t->ptr = 0;
    return t;
  } else {
    typedef int Value;
    std::vector<TrieBuilder::earlyTrieObj<Value>> early_trie;
    TrieBuilder::build_early_trie<Value>(early_trie, tuples, 0, tuples.size(), 0);
    return build_final_trie(early_trie);
  }
}

template <typename Value>
void randomise_trie(TrieObj<Value>* to) {
  if(to == NULL)
    return;

  TrieObj<Value>* end = to;
  while(end->val != std::numeric_limits<Value>::max()) {
    if(end->ptr)
      randomise_trie(end->ptr);
    end++;
  }
  std::random_shuffle(to, end);
}

template <typename Value>
std::vector<std::vector<Value>> unroll_trie(TrieObj<Value>* to) {
  std::vector<std::vector<Value>> final_tuples;
  for(; to->val != std::numeric_limits<Value>::max(); to++) {
    std::vector<std::vector<Value>> loop_tuples;
    if(to->ptr == 0)
      loop_tuples.resize(1);
    else
      loop_tuples = unroll_trie(to->ptr);
    for(auto it = loop_tuples.begin(); it != loop_tuples.end(); ++it)
      it->insert(it->begin(), to->val);
    final_tuples.insert(final_tuples.end(), loop_tuples.begin(), loop_tuples.end());
  }
  return final_tuples;
}

#endif
