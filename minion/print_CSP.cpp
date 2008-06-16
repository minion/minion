#include "CSPSpec.h"

#include <sstream>

using namespace ProbSpec;

void CSPInstance::print(ostringstream& oss) const
{   
  oss << "MINION 3" << endl;
  oss << "**VARIABLES**" << endl;
  this->vars.print(oss, *this);
  oss << "**CONSTRAINTS**" << endl;
  for(list<ConstraintBlob>::const_iterator it = constraints.begin(); 
      it != constraints.end(); ++it)
  {
    it->print(oss, *this);
  }
  oss << "**EOF**" << endl;
}
  
void ConstraintBlob::print(ostringstream& oss, const CSPInstance& csp) const
{
    
  if(this->reified)
  {
    oss << "reify( ";
    this->reify_var.print(oss, csp);
  }
  
  if(this->implied_reified)
  {
    oss << "reifyimply( ";
    this->reify_var.print(oss,csp);
  }
  
  oss << this->constraint->name;
  oss << "(";
  
  
  oss << ")";
  
  if(this->reified || this->implied_reified)
    oss << " )";
  oss << endl;  
}
  
void VarContainer::print(ostringstream& oss, const CSPInstance& csp) const
  { 

  }
  
void Var::print(ostringstream& oss, const CSPInstance& csp) const
{ oss << csp.vars.getName(*this); }
