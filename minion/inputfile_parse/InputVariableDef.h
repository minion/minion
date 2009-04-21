#ifndef DOG
#define DOG

#include <ostream>

/// The currently accepted types of Variables.
enum VariableType
{
  VAR_BOOL,
  VAR_NOTBOOL,
  VAR_BOUND,
  VAR_SPARSEBOUND,
  VAR_DISCRETE,
  VAR_SPARSEDISCRETE,
  VAR_CONSTANT,
  VAR_MATRIX,
  VAR_INVALID
};

namespace ProbSpec
{
struct CSPInstance;

class Var
{
  VariableType type_m;
  int pos_m;
public:
  int pos() const { return pos_m; }
  VariableType type() const { return type_m; }
  void setType(VariableType v) { type_m = v; }
  
  Var(VariableType _type, int _pos) : type_m(_type), pos_m(_pos)
  { }
  
  Var(const Var& v) : type_m(v.type_m), pos_m(v.pos_m)
  {}
  
  Var() : type_m(VAR_INVALID), pos_m(-1)
  {}
  
  friend std::ostream& operator<<(std::ostream& o, const Var& v)
  { return o << "Var. Type:" << v.type_m << " Pos:" << v.pos_m << "."; }
   
   bool operator==(const Var& var) const
   { return type_m == var.type_m && pos_m == var.pos_m; }
   
   bool operator<(const Var& var) const
   { return (type_m < var.type_m) || (type_m == var.type_m && pos_m < var.pos_m); }
};

}

using namespace ProbSpec;

#endif
