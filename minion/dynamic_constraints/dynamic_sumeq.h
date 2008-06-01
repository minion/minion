/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: dynamic_sum.h 398 2006-10-17 09:49:19Z gentian $
*/

/* Minion
* Copyright (C) 2006
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef NO_PRINT //for debug messages want complex string output
#include <string>
#endif

//ONLY WORKS FOR VARS WHERE ARBITRARY VALUES CAN BE REMOVED AT ANY TIME

template<typename VarRef1, typename VarRef2, typename VarRef3>
struct SumEqConstraintDynamic : public AbstractConstraint
{
#ifndef NO_PRINT
  char output[200];
#endif

  virtual string constraint_name()
  { return "SumEqDynamic"; }
  
  VarRef1 x;
  VarRef2 y;
  VarRef3 z;
  
  int xmult, ymult;

  SumEqConstraintDynamic(StateObj* _stateObj, int _xmult, int _ymult, VarRef1 _x, VarRef2 _y, VarRef3 _z) :
    AbstractConstraint(stateObj), xmult(_xmult), ymult(_ymult), x(_x), y(_y), z(_z), vals(-1)
  { 
#ifndef WATCHEDLITERALS
    cerr << "This almost certainly isn't going to work... sorry" << endl;
#endif
    if(xmult < 1 || ymult < 1)
      INPUT_ERROR("Multipliers on gacsum must be > 0")
    D_INFO(1, DI_TABLECON, "sumeq: Constructor here!");
  }

  int vals;
  
  int dynamic_trigger_count() //need two watched literals per value in each var (one per support) 
  {
    D_INFO(1, DI_TABLECON, "sumeq: counting WLs");
    if(vals == -1) { //not already calculated 
      vals = 0;
      DomainInt max = x.getMax(); 
      for(DomainInt i = x.getMin(); i <= max; i++) 
        if(x.inDomain(i)) vals++; 
      max = y.getMax(); 
      for(DomainInt i = y.getMin(); i <= max; i++) 
        if(y.inDomain(i)) vals++; 
      max = z.getMax(); 
      for(DomainInt i = z.getMin(); i <= max; i++) 
        if(z.inDomain(i)) vals++; 
    } 
    //printf("Count: %d\n", vals);
    return 2 * vals; 
    //return 1000;
  }
  
  vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> a;
    a.reserve(3); a.push_back(x); a.push_back(y); a.push_back(z);
    return a;
  }

  BOOL check_assignment(DomainInt* v, int v_size) {
    D_INFO(1, DI_TABLECON, "sumeq: checking assignment");
    return xmult*v[0] + ymult*v[1] == v[2];
  }

  //use smart algorithm to find out if any a + b = c in linear time
  //puts supports for c in resArr and returns T, or F if none exist
  BOOL get_sumsupport(VarRef1& a, VarRef2& b, DomainInt c, int (&resArr)[2]) {
    D_INFO(1, DI_TABLECON, "sumeq: looking for sum support");
    DomainInt bMin = b.getMin();
    DomainInt bMax = b.getMax();
    DomainInt aCurr = max(a.getMin(), (c - bMax*ymult)/xmult);
    DomainInt aMax = min(a.getMax(), (c - bMin*ymult)/xmult);

    while(aCurr <= aMax && !a.inDomain(aCurr))
      aCurr++;
    while(aCurr <= aMax) {
      DomainInt required = c - aCurr*xmult;
      DomainInt reqB = required / ymult;
      DomainInt modB = required % ymult;
      if(modB == 0 && b.inDomain(reqB)) {
        resArr[0] = aCurr;
        resArr[1] = reqB;
        D_ASSERT(resArr[0]*xmult + resArr[1]*ymult == c);
        return 1;
      } else {
        aCurr++;
        while(aCurr <= aMax && !(a.inDomain(aCurr)))
          aCurr++;
      }
    }
    return 0; //no supports possible
  }

  //variation on get_sumsupport to do a - b*bmult = c
  template<typename VarType1>
  BOOL get_diffsupport(VarRef3& a, VarType1& b, int bmult, DomainInt c, int (&resArr)[2]) {
    D_INFO(1, DI_TABLECON, "sumeq: looking for diff support");
    DomainInt bMin = b.getMin();
    DomainInt bMax = b.getMax();
    DomainInt aCurr = max(a.getMin(), (c + bMin*bmult));
    DomainInt aMax = min(a.getMax(), (c + bMax*bmult));
    while(aCurr <= aMax && !a.inDomain(aCurr))
      aCurr++;
    while(aCurr <= aMax) {
      DomainInt required = aCurr - c;
      DomainInt reqB = required / bmult;
      DomainInt modB = required % bmult;
      if(modB == 0 && b.inDomain(reqB)) {
        resArr[0] = aCurr;
        resArr[1] = reqB;
        D_ASSERT(resArr[0] - resArr[1]*bmult == c);
        return 1;
      } else {
        aCurr++;
        while(aCurr <= aMax && !(a.inDomain(aCurr)))
          aCurr++;	
      }
    }
    return 0; //no supports possible    
  }

  //a little data structure we maintain per WL
  struct WLdata {
    int other; //sequence number of the other WL helping to support the varval
    int isForX : 1; //the supported var
    int isForY : 1; //NB. the supported val is stored using dt->trigger_info()
    int isForZ : 1;
  };

  //a mapping from WL number to struct of data
  vector<WLdata> wlToData;
  
  // find a couple of supports for every single value, put watches on
  // these, also build data structures to allow mapping from WL number
  // to var/val as well as the other WL on this value
  virtual void full_propagate()
  {
    D_INFO(1, DI_TABLECON, "sumeq: full prop");

    //start placing WLs in the right places
    DynamicTrigger* dt = dynamic_trigger_start();  

    int index = 0;
    WLdata workingData;
    int supp[2] = {0, 0};

    wlToData.reserve(dynamic_trigger_count());

    //place a WL for each value
    //z first:
    int max = z.getMax();
    for(int i = z.getMin(); i <= max; i++) {
      if(z.inDomain(i)) {
	if(!get_sumsupport(x, y, i, supp)) {
#ifndef NO_PRINT
	  sprintf(output, "sumeq: removing %d in z during full prop", i);
	  D_INFO(1, DI_TABLECON, output);
#endif
	  z.removeFromDomain(i);
	} else {
#ifndef NO_PRINT
	  sprintf(output, "sumeq: %d in z supported by (%d,%d)", i, supp[0], supp[1]);
	  D_INFO(1, DI_TABLECON, output);
#endif
	  x.addDynamicTrigger(dt, DomainRemoval, supp[0]);
	  y.addDynamicTrigger(dt+1, DomainRemoval, supp[1]);
	  dt->trigger_info() = i;     //keep a note of the value it supports
	  (dt+1)->trigger_info() = i;
	  workingData.other = index + 1;
	  workingData.isForX = 0; workingData.isForY = 0; workingData.isForZ = 1;
	  wlToData.push_back(workingData);
	  workingData.other = index;
	  wlToData.push_back(workingData);
	  index += 2;
	  dt += 2;
	}
      }
    }
    //supports for x:
    max = x.getMax();
    for(int i = x.getMin(); i <= max; i++) {
      if(x.inDomain(i)) {
	if(!get_diffsupport(z, y, ymult, i*xmult, supp)) {
#ifndef NO_PRINT
	  sprintf(output, "sumeq: removing %d in x during full prop", i);
	  D_INFO(1, DI_TABLECON, output);
#endif
	  x.removeFromDomain(i);
	} else {
#ifndef NO_PRINT
	  sprintf(output, "sumeq: %d in x supported by (%d,%d)", i, supp[0], supp[1]);
	  D_INFO(1, DI_TABLECON, output);
#endif
	  z.addDynamicTrigger(dt, DomainRemoval, supp[0]);
	  y.addDynamicTrigger(dt + 1, DomainRemoval, supp[1]);
	  dt->trigger_info() = i;
	  (dt+1)->trigger_info() = i;
	  workingData.other = index + 1;
	  workingData.isForX = 1; workingData.isForY = 0; workingData.isForZ = 0;
	  wlToData.push_back(workingData);
	  workingData.other = index;
	  wlToData.push_back(workingData);
	  index += 2;
	  dt += 2;
	}
      }
    }
    //supports for y:
    max = y.getMax();
    for(int i = y.getMin(); i <= max; i++) {
      if(y.inDomain(i)) {
	if(!get_diffsupport(z, x, xmult, i*ymult, supp)) {
#ifndef NO_PRINT
	  sprintf(output, "sumeq: removing %d in y during full prop", i);
	  D_INFO(1, DI_TABLECON, output);
#endif
	  y.removeFromDomain(i);
	} else {
#ifndef NO_PRINT
	  sprintf(output, "sumeq: %d in y supported by (%d,%d)", i, supp[0], supp[1]);
	  D_INFO(1, DI_TABLECON, output);
#endif
	  z.addDynamicTrigger(dt, DomainRemoval, supp[0]);
	  x.addDynamicTrigger(dt + 1, DomainRemoval, supp[1]);
	  dt->trigger_info() = i;
	  (dt+1)->trigger_info() = i;
	  workingData.other = index + 1;
	  workingData.isForX = 0; workingData.isForY = 1; workingData.isForZ = 0;
	  wlToData.push_back(workingData);
	  workingData.other = index;
	  wlToData.push_back(workingData);
	  index += 2;
	  dt += 2;
	}
      }
    }
  }

  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    D_INFO(1, DI_TABLECON, "sumeq: dynamic prop");
    int value = dt->trigger_info(); //the value formerly supported by dt
    DynamicTrigger* dts = dynamic_trigger_start();
    int wl_no = dt - dts; //sequence number of WL
    WLdata data = wlToData[wl_no];
    if(!(data.isForX ? x.inDomain(value) :
	 (data.isForY ? y.inDomain(value) : z.inDomain(value))))
      return;
    int supp[2] = {0, 0};
    DynamicTrigger* first_dt = (dt < dts + data.other) ? dt : dt - 1;
    if(data.isForX) {
      if(!get_diffsupport(z, y, ymult, value*xmult, supp)) {
#ifndef NO_PRINT
	sprintf(output, "sumeq: removing %d in x during dynamic prop", value);
	D_INFO(1, DI_TABLECON, output);
#endif
	x.removeFromDomain(value);
      } else {
#ifndef NO_PRINT
	sprintf(output, "sumeq: %d in x supported by (%d,%d)", value, supp[0], supp[1]);
	D_INFO(1, DI_TABLECON, output);
#endif
	z.addDynamicTrigger(first_dt, DomainRemoval, supp[0]);
	y.addDynamicTrigger(first_dt + 1, DomainRemoval, supp[1]);
      }
    } else if(data.isForY) {
      if(!get_diffsupport(z, x, xmult, value*ymult, supp)) {
#ifndef NO_PRINT
	sprintf(output, "sumeq: removing %d in y during dynamic prop", value);
	D_INFO(1, DI_TABLECON, output);
#endif
	y.removeFromDomain(value);
      } else {
#ifndef NO_PRINT
	sprintf(output, "sumeq: %d in y supported by (%d,%d)", value, supp[0], supp[1]);
	D_INFO(1, DI_TABLECON, output);
#endif
	z.addDynamicTrigger(first_dt, DomainRemoval, supp[0]);
	x.addDynamicTrigger(first_dt + 1, DomainRemoval, supp[1]);
      }
    } else {
      if(!get_sumsupport(x, y, value, supp)) {
#ifndef NO_PRINT
	sprintf(output, "sumeq: removing %d in z during dynamic prop", value);
	D_INFO(1, DI_TABLECON, output);
#endif
	z.removeFromDomain(value);
      } else {
#ifndef NO_PRINT
	sprintf(output, "sumeq: %d in z supported by (%d,%d)", value, supp[0], supp[1]);
	D_INFO(1, DI_TABLECON, output);
#endif
	x.addDynamicTrigger(first_dt, DomainRemoval, supp[0]);
	y.addDynamicTrigger(first_dt + 1, DomainRemoval, supp[1]);
      }
    }
  }
};

template<typename VarArray, typename Var>
AbstractConstraint*
SumEqConDynamic(StateObj* stateObj, const light_vector<int>& consts, const VarArray& _var_array, const Var& var)
{
  typedef typename VarArray::value_type ValT;
  typedef typename Var::value_type VarT;
  
  if(consts.size() != 2 || _var_array.size() != 2)
    INPUT_ERROR("gacsum only accepts two variables on the left hand side.");

  return new SumEqConstraintDynamic<ValT,ValT,VarT>(stateObj, 
   consts[0], consts[1], _var_array[0], _var_array[1], var[0]);
}

BUILD_DYNAMIC_CONSTRAINT3(CT_GACSUM, SumEqConDynamic);
