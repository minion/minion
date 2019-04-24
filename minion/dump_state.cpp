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

#include "minion.h"

bool constraintEntailed(AbstractConstraint* c) {
  AbstractConstraint* rev_c = c->reverseConstraint();
  bool flag;
  GET_ASSIGNMENT(a, rev_c);
  return !flag;
}

template <typename T>
inline string getNameFromVar(const T& v) {
  return getState().getInstance()->vars.getName(v.getBaseVar());
}

void dumpSearchorder(const SearchOrder& order, ostream& os) {

  vector<SysInt> nonAssignedVars;
  for(int i = 0; i < (SysInt)order.varOrder.size(); ++i) {
    AnyVarRef v = BuildCon::getAnyVarRefFromVar(order.varOrder[i]);
    // XXX : To get tester to work.. for now. if(!v.isAssigned())
    nonAssignedVars.push_back(i);
  }

  if(nonAssignedVars.size() == 0)
    return;

  os << "VARORDER ";
  if(order.findOneAssignment)
    os << "AUX ";

  switch(order.order) {
#define Z(x)                                                                                       \
  case ORDER_##x: os << #x << " "; break;
    Z(STATIC)
    Z(SDF)
    Z(SRF)
    Z(LDF)
    Z(ORIGINAL)
    Z(WDEG)
    Z(CONFLICT)
    Z(DOMOVERWDEG)
#undef Z
  default: abort();
  }

  os << "[";
  bool first = true;
  for(SysInt i = 0; i < (SysInt)nonAssignedVars.size(); ++i) {
    AnyVarRef v = BuildCon::getAnyVarRefFromVar(order.varOrder[nonAssignedVars[i]]);
    // XXX : see above! D_ASSERT(!v.isAssigned());
    if(first)
      first = false;
    else
      os << ",";
    if(v.isAssigned())
      os << v.assignedValue();
    else
      os << getNameFromVar(v);
  }
  os << "]\n";

  os << "VALORDER [";
  first = true;
  for(SysInt i = 0; i < (SysInt)nonAssignedVars.size(); ++i) {
    if(first)
      first = false;
    else
      os << ",";
    switch(order.valOrder[nonAssignedVars[i]].type) {
    case VALORDER_ASCEND: os << "a"; break;
    case VALORDER_DESCEND: os << "d"; break;
    case VALORDER_RANDOM: os << "r"; break;
    default: abort();
    }
  }
  os << "]\n";
}

void justDomainDump(ostream& os) {
  VariableContainer& vc = getVars();
  vector<AnyVarRef> vars;
  // booleans;
  for(UnsignedSysInt i = 0; i < vc.boolVarContainer.varCount(); ++i)
    vars.push_back(vc.boolVarContainer.getVarNum(i));

  // bound vars
  for(UnsignedSysInt i = 0; i < vc.boundVarContainer.varCount(); ++i)
    vars.push_back(vc.boundVarContainer.getVarNum(i));

  // bigRangeVar
  for(UnsignedSysInt i = 0; i < vc.bigRangeVarContainer.varCount(); ++i)
    vars.push_back(vc.bigRangeVarContainer.getVarNum(i));

  // sparseBound
  for(UnsignedSysInt i = 0; i < vc.sparseBoundVarContainer.varCount(); ++i)
    vars.push_back(vc.sparseBoundVarContainer.getVarNum(i));

  for(UnsignedSysInt i = 0; i < vars.size(); ++i) {
    os << "find " << getNameFromVar(vars[i]) << " : int(";

    if(!getState().isFailed()) {
      if(vars[i].isBound()) {
        os << vars[i].min() << ".." << vars[i].max();
      } else {
        bool first = true;

        for(DomainInt val = vars[i].min(); val <= vars[i].max(); ++val) {
          if(vars[i].inDomain(val)) {
            if(first)
              first = false;
            else
              os << ",";

            DomainInt rangeStart = val;
            ++val;
            while(vars[i].inDomain(val))
              ++val;
            os << rangeStart << ".." << (val - 1);
          }
        }
      }
    }
    os << ")\n";
  }
}

void dumpSolver(ostream& os, bool justDomains) {
  if(justDomains) {
    justDomainDump(os);
    return;
  }
  os << "# Redumped during search" << endl;
  os << "MINION 3" << endl;
  os << "**VARIABLES**" << endl;
  VariableContainer& vc = getVars();

  // booleans;
  for(UnsignedSysInt i = 0; i < vc.boolVarContainer.varCount(); ++i) {
    BoolVarRef bv = vc.boolVarContainer.getVarNum(i);
    if(!bv.isAssigned())
      os << "BOOL " << getNameFromVar(bv) << endl;
  }

  // bound vars
  for(UnsignedSysInt i = 0; i < vc.boundVarContainer.varCount(); ++i) {
    BoundVarRef bv = vc.boundVarContainer.getVarNum(i);
    if(!bv.isAssigned()) {
      os << "BOUND " << getNameFromVar(bv) << " ";
      os << "{" << bv.min() << ".." << bv.max() << "}" << endl;
    }
  }

  // bigRangeVar
  for(UnsignedSysInt i = 0; i < vc.bigRangeVarContainer.varCount(); ++i) {
    BigRangeVarRef bv = vc.bigRangeVarContainer.getVarNum(i);
    if(!bv.isAssigned()) {
      os << "DISCRETE " << getNameFromVar(bv) << " ";
      os << "{" << bv.min() << ".." << bv.max() << "}" << endl;
    }
    vector<DomainInt> deletedValues;
    for(DomainInt i = bv.min() + 1; i < bv.max(); ++i) {
      if(!bv.inDomain(i))
        deletedValues.push_back(i);
    }
    if(!deletedValues.empty()) {
      os << "**CONSTRAINTS**" << endl;
      if((DomainInt)deletedValues.size() < bv.max() - bv.min() + 1) {
        os << "w-notinset(" << getNameFromVar(bv) << ", [";
        bool first = true;
        for(size_t i = 0; i < deletedValues.size(); ++i) {
          if(first)
            first = false;
          else
            os << ",";
          os << deletedValues[i];
        }
        os << "])" << endl;
      } else {
        os << "w-inset(" << getNameFromVar(bv) << ", [";
        os << bv.min();
        for(DomainInt i = bv.min() + 1; i <= bv.max(); ++i) {
          if(bv.inDomain(i))
            os << "," << i;
        }
        os << "])" << endl;
      }
      os << "**VARIABLES**" << endl;
    }
  }

  // sparseBound
  for(UnsignedSysInt i = 0; i < vc.sparseBoundVarContainer.varCount(); ++i) {
    SparseBoundVarRef bv = vc.sparseBoundVarContainer.getVarNum(i);
    vector<DomainInt> dom = vc.sparseBoundVarContainer.getRawDomain(i);
    if(!bv.isAssigned()) {

      os << "SPARSEBOUND " << getNameFromVar(bv) << " ";
      os << "{";
      bool first = true;
      for(size_t j = 0; j < dom.size(); ++j) {
        if(bv.inDomain(dom[j])) {
          if(first)
            first = false;
          else
            os << ",";
          os << dom[j];
        }
      }
      os << "}" << endl;
    }
  }

  // tuples
  os << "**TUPLELIST**" << endl;
  SearchState& searchState = getState();

  for(SysInt i = 0; i < searchState.getTupleListContainer()->size(); ++i) {
    TupleList* tl = searchState.getTupleListContainer()->getTupleList(i);
    os << tl->getName() << " " << tl->size() << " " << tl->tupleSize() << "\n";
    for(SysInt i = 0; i < tl->size() * tl->tupleSize(); ++i) {
      os << (tl->getPointer())[i] << " ";
    }
    os << endl;
  }

  os << "**SHORTTUPLELIST**" << endl;

  for(SysInt i = 0; i < searchState.getShortTupleListContainer()->size(); ++i) {
    ShortTupleList* tl = searchState.getShortTupleListContainer()->getShortTupleList(i);
    os << tl->getName() << " " << tl->size() << "\n";

    const vector<vector<pair<SysInt, DomainInt>>>& tupleRef = *(tl->tuplePtr());

    for(SysInt j = 0; j < (SysInt)tupleRef.size(); ++j) {
      os << "[";
      bool first = true;
      for(SysInt k = 0; k < (SysInt)tupleRef[j].size(); ++k) {
        if(first)
          first = false;
        else
          os << ", ";
        os << "(" << tupleRef[j][k].first << "," << tupleRef[j][k].second << ")";
      }
      os << "]\n";
    }

    os << endl;
  }

  os << "**SEARCH**" << endl;
  if(!getState().getRawOptimiseVars().empty()) {
    if(getState().isMaximise())
      os << "MAXIMISING ";
    else
      os << "MINIMISING ";
    output_mapped_container(
        os, getState().getRawOptimiseVars(),
        [](const AnyVarRef& v) { return v.isAssigned() ? tostring(v.assignedValue()) : getNameFromVar(v); },
        true);
  }
  os << "PRINT ";
  os << ConOutput::print_vars(getState().getPrintMatrix());
  os << endl;

  for(UnsignedSysInt i = 0; i < getState().getInstance()->searchOrder.size(); ++i) {
    dumpSearchorder(getState().getInstance()->searchOrder[i], os);
  }

  os << "**CONSTRAINTS**" << endl;

  if(getState().isFailed())
    os << "false()" << endl;
  else {
    for(UnsignedSysInt i = 0; i < searchState.getConstraintList().size(); ++i) {
      AbstractConstraint* c = searchState.getConstraintList()[i];
      // If it wasn't for the fact we are going to exit straight after printing
      // this out, this would be a disaster!
      if(!constraintEntailed(c))
        os << searchState.getConstraintList()[i]->fullOutputName() << "\n";
    }
  }

  os << "**EOF**" << endl;
}

void dumpSolver(string filename, bool justDomains) {
  if(filename == "" || filename == "--") {
    dumpSolver(cout, justDomains);
  } else {
    ofstream ofs(filename.c_str());
    dumpSolver(ofs, justDomains);
  }
  exit(0);
}
