// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/** @help variables;sparsebounds Description
In sparse bounds variables the domain is composed of discrete values
(e.g. {1, 5, 36, 92}), but only the upper and lower bounds of the
domain may be updated during search. Although the domain of these
variables is not a continuous range, any holes in the domains must be
there at time of specification, as they can not be added during the
solving process.
*/

/** @help variables;sparsebounds Notes
Declaration of a sparse bounds variable called myvar containing values
{1,3,4,6,7,9,11} in input file:

SPARSEBOUND myvar {1,3,4,6,7,9,11}

Use of this variable in a constraint:
eq(myvar, 3) #myvar equals 3
*/

#include "../../triggering/constraint_abstract.h"

template <typename T>
struct SparseBoundVarContainer;

template <typename DomType = DomainInt>
struct SparseBoundVarRef_internal {
  static const BOOL isBool = true;
  static const BoundType isBoundConst = Bound_Yes;
  static string name() {
    return "SparseBound";
  }
  BOOL isBound() const {
    return true;
  }

  AnyVarRef popOneMapper() const {
    FATAL_REPORTABLE_ERROR();
  }

  SysInt varNum;

  static SparseBoundVarContainer<DomType>& getCon_Static();
  SparseBoundVarRef_internal() : varNum(-1) {}

  explicit SparseBoundVarRef_internal(SparseBoundVarContainer<DomType>*, DomainInt i)
      : varNum(checked_cast<SysInt>(i)) {}
};

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<VarRefType<SparseBoundVarRef_internal<>>, VAR_INFO_SPARSEBOUND>
    SparseBoundVarRef;
#else
typedef VarRefType<SparseBoundVarRef_internal<>> SparseBoundVarRef;
#endif

template <typename BoundType = DomainInt>
struct SparseBoundVarContainer {

  ExtendableBlock bound_data;
  TriggerList triggerList;
  vector<vector<BoundType>> domains;
  vector<DomainInt> domain_reference;
  vector<vector<AbstractConstraint*>> constraints;
#ifdef WDEG
  vector<UnsignedSysInt> wdegs;
#endif
  UnsignedSysInt varCount_m;

  SparseBoundVarContainer() : triggerList(true), varCount_m(0) {}

  vector<BoundType>& getDomain(SparseBoundVarRef_internal<BoundType> i) {
    return domains[checked_cast<SysInt>(domain_reference[i.varNum])];
  }

  vector<BoundType>& getDomain_from_int(SysInt i) {
    return domains[checked_cast<SysInt>(domain_reference[i])];
  }

  const BoundType& lowerBound(SparseBoundVarRef_internal<BoundType> i) const {
    return ((BoundType*)bound_data())[i.varNum * 2];
  }

  const BoundType& upperBound(SparseBoundVarRef_internal<BoundType> i) const {
    return ((BoundType*)bound_data())[i.varNum * 2 + 1];
  }

  BoundType& lowerBound(SparseBoundVarRef_internal<BoundType> i) {
    return ((BoundType*)bound_data())[i.varNum * 2];
  }

  BoundType& upperBound(SparseBoundVarRef_internal<BoundType> i) {
    return ((BoundType*)bound_data())[i.varNum * 2 + 1];
  }

  /// find the small possible lower bound above newLowerBound.
  /// Does not actually change the lower bound.
  DomainInt findLowerBound(SparseBoundVarRef_internal<BoundType> d, DomainInt newLowerBound) {
    vector<BoundType>& bounds = getDomain(d);
    typename vector<BoundType>::iterator it =
        std::lower_bound(bounds.begin(), bounds.end(), newLowerBound);
    if(it == bounds.end()) {
      getState().setFailed(true);
      return *(it - 1);
    }

    return *it;
  }

  /// find the largest possible upper bound below newUpperBound.
  /// Does not actually change the upper bound.
  DomainInt findUpperBound(SparseBoundVarRef_internal<BoundType>& d, DomainInt newUpperBound) {
    vector<BoundType>& bounds = getDomain(d);

    typename vector<BoundType>::iterator it =
        std::lower_bound(bounds.begin(), bounds.end(), newUpperBound);
    if(it == bounds.end())
      return *(it - 1);

    if(*it == newUpperBound)
      return newUpperBound;

    if(it == bounds.begin()) {
      getState().setFailed(true);
      return bounds.front();
    }

    return *(it - 1);
  }

  void addVariables(const vector<DomainInt>& bounds, SysInt count = 1) {
    D_ASSERT(count > 0);

    SysInt oldCount = varCount_m;

    DomainInt minDomainVal = DomainInt_Min;
    DomainInt maxDomainVal = DomainInt_Max;

    D_ASSERT(bounds.front() >= DomainInt_Min);
    D_ASSERT(bounds.back() <= DomainInt_Max);

    for(SysInt loop = 0; loop < (SysInt)(bounds.size()) - 1; ++loop) {
      D_ASSERT(bounds[loop] < bounds[loop + 1]);
    }

    vector<BoundType> tDom(bounds.size());
    for(UnsignedSysInt j = 0; j < bounds.size(); ++j)
      tDom[j] = bounds[j];

    domains.push_back(tDom);
    for(SysInt j = 0; j < count; ++j)
      domain_reference.push_back(domains.size() - 1);

    minDomainVal = mymin(tDom.front(), minDomainVal);
    maxDomainVal = mymax(tDom.back(), maxDomainVal);

    // TODO: Setting varCount_m to avoid changing other code.. long term, do
    // we need it?
    varCount_m = domain_reference.size();

    constraints.resize(varCount_m);
#ifdef WDEG
    wdegs.resize(varCount_m);
#endif

    if(bound_data.empty()) {
      bound_data =
          getMemory().backTrack().requestBytesExtendable(varCount_m * 2 * sizeof(BoundType));
    } else {
      getMemory().backTrack().resizeExtendableBlock(bound_data,
                                                    varCount_m * 2 * sizeof(BoundType));
    }

    BoundType* bound_ptr = (BoundType*)(bound_data());
    for(UnsignedSysInt i = oldCount; i < varCount_m; ++i) {
      bound_ptr[2 * i] = getDomain_from_int(i).front();
      bound_ptr[2 * i + 1] = getDomain_from_int(i).back();
    }

    std::vector<std::pair<DomainInt, DomainInt>> trigBounds(
        count, make_pair(bounds.front(), bounds.back()));
    triggerList.addVariables(trigBounds);
  }

  BOOL isAssigned(SparseBoundVarRef_internal<BoundType> d) const {
    return lowerBound(d) == upperBound(d);
  }

  DomainInt getAssignedValue(SparseBoundVarRef_internal<BoundType> d) const {
    D_ASSERT(isAssigned(d));
    return lowerBound(d);
  }

  BOOL inDomain(SparseBoundVarRef_internal<BoundType> d, DomainInt i) const {
    // First check against bounds
    if(i < lowerBound(d) || i > upperBound(d)) {
      return false;
    } else {
      return inDomain_noBoundCheck(d, i);
    }
  }

  BOOL inDomain_noBoundCheck(SparseBoundVarRef_internal<BoundType> ref, DomainInt i) const {
    // use binary search to find if the value is in the domain vector.
    // const vector<BoundType>& dom = getDomain(ref);  // why does this not
    // work?
    const vector<BoundType>& dom =
        domains[checked_cast<SysInt>(domain_reference[checked_cast<SysInt>(ref.varNum)])];

    return std::binary_search(dom.begin(), dom.end(), i);
  }

  DomainInt getDomSize(SparseBoundVarRef_internal<BoundType> d) const {
    assert(0);
    return 0; // Just to shut up compiler complaints.
  }

  DomainInt getMin(SparseBoundVarRef_internal<BoundType> d) const {
    return lowerBound(d);
  }

  DomainInt getMax(SparseBoundVarRef_internal<BoundType> d) const {
    return upperBound(d);
  }

  DomainInt initialMin(SparseBoundVarRef_internal<BoundType> d) {
    return getDomain_from_int(d.varNum).front();
  }

  DomainInt initialMax(SparseBoundVarRef_internal<BoundType> d) {
    return getDomain_from_int(d.varNum).back();
  }

  /// This function is provided for convience. It should never be called.
  void removeFromDomain(SparseBoundVarRef_internal<BoundType>, DomainInt) {
    USER_ERROR("Some constraint you are using does not work with SPARSEBOUND "
               "variables\n"
               "Unfortunatly we cannot tell you which one. Sorry!");
  }

  void internalAssign(SparseBoundVarRef_internal<BoundType> d, DomainInt i) {
    vector<BoundType>& bounds = getDomain(d);
    DomainInt minVal = getMin(d);
    DomainInt maxVal = getMax(d);

    if(!binary_search(bounds.begin(), bounds.end(), i)) {
      getState().setFailed(true);
      return;
    }
    if(minVal > i || maxVal < i) {
      getState().setFailed(true);
      return;
    }

    if(minVal == maxVal)
      return;

    triggerList.pushDomainChanged(d.varNum);
    triggerList.push_assign(d.varNum, i);

    // Can't attach triggers to bound vars!

    if(minVal != i) {
      triggerList.pushLower(d.varNum, i - minVal);
    }

    if(maxVal != i) {
      triggerList.pushUpper(d.varNum, maxVal - i);
    }

    upperBound(d) = i;
    lowerBound(d) = i;
  }

  void assign(SparseBoundVarRef_internal<BoundType> d, DomainInt i) {
    internalAssign(d, i);
  }

  // TODO : Optimise
  void uncheckedAssign(SparseBoundVarRef_internal<BoundType> d, DomainInt i) {
    internalAssign(d, i);
  }

  void setMax(SparseBoundVarRef_internal<BoundType> d, DomainInt i) {
    // Note, this just finds a new upper bound, it doesn't set it.
    i = findUpperBound(d, i);

    DomainInt lowBound = lowerBound(d);

    if(i < lowBound) {
      getState().setFailed(true);
      return;
    }

    DomainInt upBound = upperBound(d);

    if(i < upBound) {
      triggerList.pushUpper(d.varNum, upBound - i);
      triggerList.pushDomainChanged(d.varNum);
      // Can't attach triggers to bound vars!

      upperBound(d) = i;
      if(lowBound == i) {
        triggerList.push_assign(d.varNum, i);
      }
    }
  }

  void setMin(SparseBoundVarRef_internal<BoundType> d, DomainInt i) {
    i = findLowerBound(d, i);

    DomainInt upBound = upperBound(d);

    if(i > upBound) {
      getState().setFailed(true);
      return;
    }

    DomainInt lowBound = lowerBound(d);

    if(i > lowBound) {
      triggerList.pushLower(d.varNum, i - lowBound);
      triggerList.pushDomainChanged(d.varNum);
      // Can't attach triggers to bound vars!
      lowerBound(d) = i;
      if(upBound == i) {
        triggerList.push_assign(d.varNum, i);
      }
    }
  }

  //  SparseBoundVarRef get_new_var();
  template <typename T>
  SparseBoundVarRef get_new_var(const vector<T>&);
  SparseBoundVarRef getVarNum(DomainInt i);

  vector<DomainInt> getRawDomain(DomainInt i) {
    return this->domains[checked_cast<SysInt>(i)];
  }

  UnsignedSysInt varCount() {
    return varCount_m;
  }

  vector<AbstractConstraint*>* getConstraints(const SparseBoundVarRef_internal<BoundType>& b) {
    return &constraints[b.varNum];
  }

  void addConstraint(const SparseBoundVarRef_internal<BoundType>& b, AbstractConstraint* c) {
    constraints[b.varNum].push_back(c);
#ifdef WDEG
    wdegs[b.varNum] += c->getWdeg(); // add constraint score to base var wdeg
#endif
  }

  DomainInt getBaseVal(const SparseBoundVarRef_internal<BoundType>& b, DomainInt v) const {
    D_ASSERT(inDomain(b, v));
    return v;
  }

  Var getBaseVar(const SparseBoundVarRef_internal<BoundType>& b) const {
    return Var(VAR_SPARSEBOUND, b.varNum);
  }

  vector<Mapper> getMapperStack() const {
    return vector<Mapper>();
  }

#ifdef WDEG
  DomainInt getBaseWdeg(const SparseBoundVarRef_internal<BoundType>& b) {
    return wdegs[b.varNum];
  }

  void incWdeg(const SparseBoundVarRef_internal<BoundType>& b) {
    wdegs[b.varNum]++;
  }
#endif

  void addDynamicTrigger(SparseBoundVarRef_internal<BoundType> b, Trig_ConRef t, TrigType type,
                         DomainInt pos = NoDomainValue, TrigOp op = TO_Default) {
    if(type == DomainRemoval) {
      USER_ERROR("Some constraint you are using does not work with SPARSEBOUND "
                 "variables\n"
                 "Unfortunatly we cannot tell you which one. Sorry!");
    }
    triggerList.addDynamicTrigger(b.varNum, t, type, pos, op);
  }

  operator std::string() {
    stringstream s;
    SysInt charCount = 0;
    for(UnsignedSysInt i = 0; i < varCount_m; i++) {
      if(!isAssigned(SparseBoundVarRef_internal<BoundType>(i)))
        s << "X";
      else {
        s << (getAssignedValue(SparseBoundVarRef_internal<BoundType>(i)) ? 1 : 0);
      }
      charCount++;
      if(charCount % 7 == 0)
        s << endl;
    }
    return s.str();
  }
};

template <typename T>
inline SparseBoundVarRef SparseBoundVarContainer<T>::getVarNum(DomainInt i) {
  D_ASSERT(i < (SysInt)varCount_m);
  return SparseBoundVarRef(SparseBoundVarRef_internal<T>(this, i));
}
