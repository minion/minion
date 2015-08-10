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

bool constraint_entailed(AbstractConstraint *c) {
  AbstractConstraint *rev_c = c->reverse_constraint();
  bool flag;
  GET_ASSIGNMENT(a, rev_c);
  return !flag;
}

template <typename T>
inline string getNameFromVar(const T &v) {
  return getState().getInstance()->vars.getName(v.getBaseVar());
}

void dump_searchorder(const SearchOrder &order, ostream &os) {

  vector<SysInt> non_assigned_vars;
  for (int i = 0; i < (SysInt)order.var_order.size(); ++i) {
    AnyVarRef v = BuildCon::get_AnyVarRef_from_Var(order.var_order[i]);
    // XXX : To get tester to work.. for now. if(!v.isAssigned())
    non_assigned_vars.push_back(i);
  }

  if (non_assigned_vars.size() == 0)
    return;

  os << "VARORDER ";
  if (order.find_one_assignment)
    os << "AUX ";

  switch (order.order) {
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
  for (SysInt i = 0; i < (SysInt)non_assigned_vars.size(); ++i) {
    AnyVarRef v = BuildCon::get_AnyVarRef_from_Var(order.var_order[non_assigned_vars[i]]);
    // XXX : see above! D_ASSERT(!v.isAssigned());
    if (first)
      first = false;
    else
      os << ",";
    if (v.isAssigned())
      os << v.getAssignedValue();
    else
      os << getNameFromVar(v);
  }
  os << "]\n";

  os << "VALORDER [";
  first = true;
  for (SysInt i = 0; i < (SysInt)non_assigned_vars.size(); ++i) {
    if (first)
      first = false;
    else
      os << ",";
    switch (order.val_order[non_assigned_vars[i]]) {
    case VALORDER_ASCEND: os << "a"; break;
    case VALORDER_DESCEND: os << "d"; break;
    case VALORDER_RANDOM: os << "r"; break;
    default: abort();
    }
  }
  os << "]\n";
}

void just_domain_dump(ostream &os) {
  VariableContainer &vc = getVars();
  vector<AnyVarRef> vars;
  // booleans;
  for (UnsignedSysInt i = 0; i < vc.boolVarContainer.var_count(); ++i)
    vars.push_back(vc.boolVarContainer.get_var_num(i));

  // bound vars
  for (UnsignedSysInt i = 0; i < vc.boundVarContainer.var_count(); ++i)
    vars.push_back(vc.boundVarContainer.get_var_num(i));

  // bigRangeVar
  for (UnsignedSysInt i = 0; i < vc.bigRangeVarContainer.var_count(); ++i)
    vars.push_back(vc.bigRangeVarContainer.get_var_num(i));

  // sparseBound
  for (UnsignedSysInt i = 0; i < vc.sparseBoundVarContainer.var_count(); ++i)
    vars.push_back(vc.sparseBoundVarContainer.get_var_num(i));

  for (UnsignedSysInt i = 0; i < vars.size(); ++i) {
    os << "find " << getNameFromVar(vars[i]) << " : int(";

    if (!getState().isFailed()) {
      if (vars[i].isBound()) {
        os << vars[i].getMin() << ".." << vars[i].getMax();
      } else {
        bool first = true;

        for (DomainInt val = vars[i].getMin(); val <= vars[i].getMax(); ++val) {
          if (vars[i].inDomain(val)) {
            if (first)
              first = false;
            else
              os << ",";

            DomainInt range_start = val;
            ++val;
            while (vars[i].inDomain(val))
              ++val;
            os << range_start << ".." << (val - 1);
          }
        }
      }
    }
    os << ")\n";
  }
}

void dump_solver(ostream &os, bool just_domains) {
  if (just_domains) {
    just_domain_dump(os);
    return;
  }
  os << "# Redumped during search" << endl;
  os << "MINION 3" << endl;
  os << "**VARIABLES**" << endl;
  VariableContainer &vc = getVars();

  // booleans;
  for (UnsignedSysInt i = 0; i < vc.boolVarContainer.var_count(); ++i) {
    BoolVarRef bv = vc.boolVarContainer.get_var_num(i);
    if (!bv.isAssigned())
      os << "BOOL " << getNameFromVar(bv) << endl;
  }

  // bound vars
  for (UnsignedSysInt i = 0; i < vc.boundVarContainer.var_count(); ++i) {
    BoundVarRef bv = vc.boundVarContainer.get_var_num(i);
    if (!bv.isAssigned()) {
      os << "BOUND " << getNameFromVar(bv) << " ";
      os << "{" << bv.getMin() << ".." << bv.getMax() << "}" << endl;
    }
  }

  // bigRangeVar
  for (UnsignedSysInt i = 0; i < vc.bigRangeVarContainer.var_count(); ++i) {
    BigRangeVarRef bv = vc.bigRangeVarContainer.get_var_num(i);
    if (!bv.isAssigned()) {
      os << "DISCRETE " << getNameFromVar(bv) << " ";
      os << "{" << bv.getMin() << ".." << bv.getMax() << "}" << endl;
    }
    vector<DomainInt> deleted_values;
    for (DomainInt i = bv.getMin() + 1; i < bv.getMax(); ++i) {
      if (!bv.inDomain(i))
        deleted_values.push_back(i);
    }
    if (!deleted_values.empty()) {
      os << "**CONSTRAINTS**" << endl;
      if ((DomainInt)deleted_values.size() < bv.getMax() - bv.getMin() + 1) {
        os << "w-notinset(" << getNameFromVar(bv) << ", [";
        bool first = true;
        for (size_t i = 0; i < deleted_values.size(); ++i) {
          if (first)
            first = false;
          else
            os << ",";
          os << deleted_values[i];
        }
        os << "])" << endl;
      } else {
        os << "w-inset(" << getNameFromVar(bv) << ", [";
        os << bv.getMin();
        for (DomainInt i = bv.getMin() + 1; i <= bv.getMax(); ++i) {
          if (bv.inDomain(i))
            os << "," << i;
        }
        os << "])" << endl;
      }
      os << "**VARIABLES**" << endl;
    }
  }

  // sparseBound
  for (UnsignedSysInt i = 0; i < vc.sparseBoundVarContainer.var_count(); ++i) {
    SparseBoundVarRef bv = vc.sparseBoundVarContainer.get_var_num(i);
    vector<DomainInt> dom = vc.sparseBoundVarContainer.get_raw_domain(i);
    if (!bv.isAssigned()) {

      os << "SPARSEBOUND " << getNameFromVar(bv) << " ";
      os << "{";
      bool first = true;
      for (size_t j = 0; j < dom.size(); ++j) {
        if (bv.inDomain(dom[j])) {
          if (first)
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
  SearchState &search_state = getState();

  for (SysInt i = 0; i < search_state.getTupleListContainer()->size(); ++i) {
    TupleList *tl = search_state.getTupleListContainer()->getTupleList(i);
    os << tl->getName() << " " << tl->size() << " " << tl->tuple_size() << "\n";
    for (SysInt i = 0; i < tl->size() * tl->tuple_size(); ++i) {
      os << (tl->getPointer())[i] << " ";
    }
    os << endl;
  }

  os << "**SHORTTUPLELIST**" << endl;

  for (SysInt i = 0; i < search_state.getShortTupleListContainer()->size(); ++i) {
    ShortTupleList *tl = search_state.getShortTupleListContainer()->getShortTupleList(i);
    os << tl->getName() << " " << tl->size() << "\n";

    const vector<vector<pair<SysInt, DomainInt>>> &tupleRef = *(tl->tuplePtr());

    for (SysInt j = 0; j < (SysInt)tupleRef.size(); ++j) {
      os << "[";
      bool first = true;
      for (SysInt k = 0; k < (SysInt)tupleRef[j].size(); ++k) {
        if (first)
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
  if (getState().getRawOptimiseVar()) {
    if (getState().isMaximise())
      os << "MAXIMISING ";
    else
      os << "MINIMISING ";
    if (getState().getRawOptimiseVar()->isAssigned())
      os << getState().getRawOptimiseVar()->getAssignedValue() << "\n";
    else
      os << getNameFromVar(*getState().getRawOptimiseVar()) << "\n";
  }
  os << "PRINT ";
  os << ConOutput::print_vars(getState().getPrintMatrix());
  os << endl;

  for (UnsignedSysInt i = 0; i < getState().getInstance()->search_order.size(); ++i) {
    dump_searchorder(getState().getInstance()->search_order[i], os);
  }

  os << "**CONSTRAINTS**" << endl;

  if (getState().isFailed())
    os << "false()" << endl;
  else {
    for (UnsignedSysInt i = 0; i < search_state.getConstraintList().size(); ++i) {
      AbstractConstraint *c = search_state.getConstraintList()[i];
      // If it wasn't for the fact we are going to exit straight after printing
      // this out, this would be a disaster!
      if (!constraint_entailed(c))
        os << search_state.getConstraintList()[i]->full_output_name() << "\n";
    }
  }

  os << "**EOF**" << endl;
}

void dump_solver(string filename, bool just_domains) {
  if (filename == "" || filename == "--") {
    dump_solver(cout, just_domains);
  } else {
    ofstream ofs(filename.c_str());
    dump_solver(ofs, just_domains);
  }
  exit(0);
}
