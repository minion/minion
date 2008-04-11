/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: AnyVarRef.h 1262 2008-03-27 16:10:41Z ncam $
*/

#ifndef IDENTITIES
#define IDENTITIES

#include "../system/system.h"

#include <vector>
#include <iostream>
#include <ostream>

using namespace std;

enum varType {boolVarT, boundsT, sparseT, discreteT, constT, trivialBoundT};

enum mapType {noneT, negT, notVarT, shiftT, stretchT, switch_negT, constantT};

struct VarIdent {
  varType vt;
  vector<mapType> mt_v; //mt_v[0] is the current mapper, mt_v[1] first nested mapper, ...
  int varNo; //unique identity for variable of that type
  vector<int> mapInfo_v; //constant, multiplier or offset

  VarIdent(varType _vt, mapType _mt, int _varNo, int _mapInfo) :
    vt(_vt), varNo(_varNo)
  {
    mt_v.push_back(_mt);
    mapInfo_v.push_back(_mapInfo);
  }

  VarIdent(mapType _mt, int _mapInfo, const VarIdent& nestedVi) :
    vt(nestedVi.vt), varNo(nestedVi.varNo)
  {
    D_ASSERT(_mt != noneT); //cannot have something nested in a primitive type
    D_ASSERT(_mt != constantT); //cannot have something nested in a constant
    mt_v.push_back(_mt);
    mt_v.insert(mt_v.end(), nestedVi.mt_v.begin(), nestedVi.mt_v.end());
    mapInfo_v.push_back(_mapInfo);
    mapInfo_v.insert(mapInfo_v.end(), nestedVi.mapInfo_v.begin(), nestedVi.mapInfo_v.end());
  }
  
  DomainInt underlyingVal(DomainInt v) const
  { 
    const size_t s = mt_v.size();
    for(int i = 0; i < s; i++) {
      switch(mt_v[i]) {
      case(negT) :  v *= -1; break;
      case(notVarT): v = -v + 1; break;
      case(shiftT): v -= mapInfo_v[i]; break;
      case(stretchT): v /= mapInfo_v[i]; break;
      case(switch_negT): v *= mapInfo_v[i]; break;
      case(constantT): D_ASSERT(i == s - 1 && v == mapInfo_v[i]); return v; break;
      case(noneT): D_ASSERT(i == s - 1); return v; break;
      default: D_ASSERT(false); break;
      }
    }
    return v;
  }

  //true iff the underlying representation is the same
  //e.g. both underlying variables are the same
  bool same_underlying(const VarIdent& i) const
  { return vt == i.vt && varNo == i.varNo; } 
  
  bool same_var(const VarIdent& i) const //true iff the var is completely the same
  { 
    return vt == i.vt && varNo == i.varNo && 
      mt_v == i.mt_v && mapInfo_v == i.mapInfo_v; 
  }

  varType type() const 
  { return vt; }

  mapType map() const 
  { return mt_v[0]; }
};

inline ostream& operator<<(ostream& output, const VarIdent& p) {
  output << "(vt=" << p.vt << ",id=" << p.varNo << ",mt=" << p.mt_v << ",mapInfo_v=" 
	 << p.mapInfo_v << ")";
  return output;
}

#endif
