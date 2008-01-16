
/*============================================================
 *  User defined propagator for enforcing bounds consistency
 *  on the alldiff constraint.
 */

typedef struct {
  int min, max;		// start, end of interval
  int minrank, maxrank; // rank of min & max in bounds[] of an adcsp
} interval;

enum PropType { WithOutValueRemoval, WithValueRemoval };

template<typename VarArray>
class AllDiffConstraint : public Constraint
{
  virtual string constraint_name()
  { return "AllDiff"; }
  
  typedef typename VarArray::value_type VarRef;
  
  AllDiffConstraint(const VarArray& vars, PropType& prop ) : _vars(vars), _prop(prop)
  {}
  virtual triggerCollection setup_internal();
  
    ~AllDiffConstraint();
//    virtual void setup_internal();
    PROPAGATE_FUNCTION(int,DomainDelta);
    void propagateValue();
  private:
    VarArray _vars;
    ReversibleInt currentLevel;
    int lastLevel;
    PropType _prop;
    int n;
    int *t;		// tree links
    int *d;		// diffs between critical capacities
    int *h;		// hall interval links
    interval *iv;
    interval **minsorted;
    interval **maxsorted;
    int *bounds;  // bounds[1..nb] hold set of min & max in the niv intervals
                  // while bounds[0] and bounds[nb+1] allow sentinels
    int nb;
    void sortit();
    int filterlower();
    int filterupper();
};

#include "constraint_alldiff.hpp"


template<typename VarArray>
Constraint*
AllDiffCon(VarArray _var_array)
{ 
  return 
  (new AllDiffConstraint<VarArray>(_var_array,WithValueRemoval)); 
}


//IlcConstraint IlcNewAllDiff( IlcIntVarArray vars, PropType prop );

