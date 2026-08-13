
struct Backtrackable {
  virtual void mark() {
    cout << "Call to mark on class that does not implement it." << endl;
    abort();
  }

  virtual void pop() {
    cout << "Call to pop on class that does not implement it." << endl;
    abort();
  }
};

struct GenericBacktracker {
private:
  vector<Backtrackable*> things;
  GenericBacktracker(const GenericBacktracker&);

public:
  GenericBacktracker() {}

  void worldPop() {
    int size = things.size();
    for(int i = 0; i < size; i++) {
      things[i]->pop();
    }
  }

  void mark() {
    int size = things.size();
    for(int i = 0; i < size; i++) {
      things[i]->mark();
    }
  }

  void add(Backtrackable* bt_obj) {
    things.push_back(bt_obj);
  }

  int size() const {
    return (int)things.size();
  }

  /// Give every object registered at index `from` or later `count`
  /// extra marks. Used when a constraint registers itself mid-search:
  /// it missed the worldPushes that happened before it existed, but
  /// worldPop still calls pop() on it once per open level, so without
  /// this its pops outnumber its marks and it unwinds off the bottom
  /// of its own backtrack stack.
  void markFrom(int from, int count) {
    int size = things.size();
    for(int i = from; i < size; i++)
      for(int j = 0; j < count; j++)
        things[i]->mark();
  }
};
