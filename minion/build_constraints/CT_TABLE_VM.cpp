#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/vm.h"
#include "../constraints/constraint_constant.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


template <typename T>
AbstractConstraint*
output_table_vm(StateObj* stateObj,const T& t1, ConstraintBlob& b, char const* type)
{ 
    std::ostringstream oss;
    oss << "{\n";
    oss << "\"doms\" : [";
    for(int i = 0; i < t1.size(); ++i)
    {
        if(i > 0) oss <<  ",";
        oss << "[";
        oss << checked_cast<SysInt>(t1[i].getInitialMin());
        for(DomainInt j = t1[i].getInitialMin() + 1; j <= t1[i].getInitialMax(); ++j)
        {
            oss << "," << checked_cast<SysInt>(j);
        }
        oss << "]";
    }
    oss << "],\n";

    oss <<  "\"table\" : [";
    for(int i = 0; i < b.tuples->size(); ++i)
    {
        vector<DomainInt> v = b.tuples->get_vector(i);
        if(i > 0) oss <<  ",\n";
        oss << "[";
        oss << checked_cast<SysInt>(v[0]);
        for(int j = 1; j < v.size(); ++j)
        {
            oss << "," << checked_cast<SysInt>(v[j]);
        }
        oss << "]";
    }
    oss << "],\n";

    oss <<  "\"type\" : \"" << type << "\" }\n";

    std::string s = oss.str();

    std::string hash = sha1_hash(s);

    std::string outname = "tableout/table." + hash;
    int f = open(outname.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
    if(f >= 0)
    {
        dprintf(f, "%s", s.c_str());
        close(f);
    }
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