// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef DYN_TRIG_H_FFJJKKTEA
#define DYN_TRIG_H_FFJJKKTEA

//#define TRIGP(x) std::cerr << x << "\n";
#define TRIGP(x)

class DynamicTriggerList;
class AbstractConstraint;

/// Used in constraints to denote the position of a trigger
struct Con_TrigRef {
  DynamicTriggerList* dtl;
  SysInt triggerListPos;

  Con_TrigRef() : dtl(nullptr), triggerListPos(-1) {}

  Con_TrigRef(DynamicTriggerList* _dtl, SysInt _pos) : dtl(_dtl), triggerListPos(_pos) {}

  bool empty() {
    return dtl == nullptr;
  }

  friend bool operator==(Con_TrigRef lhs, Con_TrigRef rhs) {
    return lhs.dtl == rhs.dtl && lhs.triggerListPos == rhs.triggerListPos;
  }

  friend std::ostream& operator<<(std::ostream& o, Con_TrigRef ctr) {
    return o << "ctr:(" << ctr.dtl << ":" << ctr.triggerListPos << ")";
  }
};

struct Trig_ConRef {
  AbstractConstraint* con;
  SysInt conListPos;

  Trig_ConRef() : con(nullptr), conListPos(-1) {}

  Trig_ConRef(AbstractConstraint* _con, SysInt _pos) : con(_con), conListPos(_pos) {}

  void propagate(DomainDelta);

  bool empty() const {
    return con == nullptr;
  }

  AbstractConstraint* constraint() {
    D_ASSERT(con);
    return con;
  }

  friend bool operator==(Trig_ConRef lhs, Trig_ConRef rhs) {
    return lhs.con == rhs.con && lhs.conListPos == rhs.conListPos;
  }

  friend std::ostream& operator<<(std::ostream& o, Trig_ConRef tcr) {
    return o << "tcr:(" << tcr.con << ":" << tcr.conListPos << ")";
  }
};

// forward declaration

void releaseMergedTrigger(Con_TrigRef, TrigOp op = TO_Default);
void releaseMergedTrigger(Trig_ConRef, TrigOp op = TO_Default);

class DynamicTriggerList {
  vector<Trig_ConRef> elems;
  vector<SysInt> slack;

public:
  Trig_ConRef _getConRef(SysInt pos) {
    return elems[pos];
  }

  DynamicTriggerList() {}

  DynamicTriggerList(const DynamicTriggerList&) {
    abort();
  }

  bool sanityCheckList();

  void add(Trig_ConRef t);

  bool empty() const {
    return elems.size() == 0;
  }
  size_t size() const {
    return elems.size();
  }

  Trig_ConRef operator[](SysInt s) {
    return elems[s];
  }

  void verifySlack() const;

  void tryCompressList();

  void _reportTriggerRemovalToList(SysInt pos) {
    TRIGP("TRL:" << pos << ":" << elems[pos]);
    elems[pos] = Trig_ConRef{};
    slack.push_back(pos);
  }
};

/// Container for a range of triggers
class DynamicTriggerEvent {
  DynamicTriggerList* trigs;

public:
  DynamicTriggerList* event() const {
    return trigs;
  }

  /// The domain delta from the domain change.
  /** This may not contain the actual delta, but contains data from which a
     variable can
     construct it, by passing it to getDomainChange. */
  DomainInt data;
  DynamicTriggerEvent(DynamicTriggerList* t, DomainInt _data) : trigs(t), data(_data) {
    D_ASSERT(data >= DomainInt_Min);
    D_ASSERT(data <= DomainInt_Max);
  }
};

inline void attachTriggerToNullList(Trig_ConRef t, TrigOp op = TO_Default);
inline void _restoreTriggerOnBacktrack(Trig_ConRef tcr);

#endif
