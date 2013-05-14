#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/vm.h"
   #include "../constraints/constraint_constant.h"

template <typename T>
AbstractConstraint*
output_table_vm(StateObj* stateObj,const T& t1, ConstraintBlob& b, char const* type)
{ 
    char name[] = "~/tableout/table.XXXXXXXX";
    int f = mkstemp(name);
    if(f == -1)
    {
        std::cerr << "This outputs to ~/tableout. Please create that directory\n";
        abort();
    }

    dprintf(f,"{\n");
    dprintf(f,"\"doms\" : [");
    for(int i = 0; i < t1.size(); ++i)
    {
        if(i > 0) dprintf(f, ",");
        dprintf(f,"[");
        dprintf(f,"%d",checked_cast<SysInt>(t1[i].getInitialMin()));
        for(DomainInt j = t1[i].getInitialMin() + 1; j <= t1[i].getInitialMax(); ++j)
        {
            dprintf(f,",%d",checked_cast<SysInt>(j));
        }
        dprintf(f,"]");
    }
    dprintf(f,"],\n");

    dprintf(f, "\"table\" : [");
    for(int i = 0; i < b.tuples->size(); ++i)
    {
        vector<DomainInt> v = b.tuples->get_vector(i);
        if(i > 0) dprintf(f, ",\n");
        dprintf(f,"[");
        dprintf(f,"%d",checked_cast<SysInt>(v[0]));
        for(int j = 1; j < v.size(); ++j)
        {
            dprintf(f,",%d",checked_cast<SysInt>(v[j]));
        }
        dprintf(f,"]");
    }
    dprintf(f,"],\n");

    dprintf(f, "\"type\" : \"%s\" }\n", type);

    return (new ConstantConstraint<false>(stateObj)); 

} 


template <typename T>
AbstractConstraint*
BuildCT_TABLE_VM(StateObj* stateObj,const T& t1, ConstraintBlob& b)
{ return output_table_vm(stateObj, t1, b, "pos"); }

BUILD_CT(CT_TABLE_VM, 1)

template <typename T>
AbstractConstraint*
BuildCT_NEG_TABLE_VM(StateObj* stateObj,const T& t1, ConstraintBlob& b)
{ return output_table_vm(stateObj, t1, b, "neg"); }

BUILD_CT(CT_NEG_TABLE_VM, 1)