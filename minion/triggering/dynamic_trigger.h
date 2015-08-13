/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

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

  bool sanity_check_list();

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

  void verify_slack() const;

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
