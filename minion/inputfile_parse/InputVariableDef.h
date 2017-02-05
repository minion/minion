#ifndef DOG
#define DOG

#include <ostream>
/// The currently accepted types of Variables.
enum VariableType {
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

enum MapperType { MAP_NEG, MAP_NOT, MAP_MULT, MAP_SHIFT, MAP_SWITCH_NEG, MAP_INVALID };

class Mapper {
  MapperType type_m;
  DomainInt val_m;

public:
  Mapper(MapperType m, DomainInt v = 0) : type_m(m), val_m(v) {}

  Mapper() : type_m(MAP_INVALID), val_m(0) {}

  MapperType type() const {
    D_ASSERT(type_m != MAP_INVALID);
    return type_m;
  }

  DomainInt val() const {
    D_ASSERT(type_m != MAP_NEG && type_m != MAP_NOT && type_m != MAP_INVALID);
    return val_m;
  }

  friend bool operator==(Mapper m1, Mapper m2) {
    return (m1.type_m == m2.type_m) && (m1.val_m == m2.val_m);
  }

  friend bool operator!=(Mapper m1, Mapper m2) {
    return !(m1 == m2);
  }
};

namespace ProbSpec {
struct CSPInstance;

class Var {
  VariableType type_m;
  DomainInt pos_m;

public:

  bool isValid() const {
    return type_m != VAR_INVALID;
  }

  DomainInt pos() const {
    D_ASSERT(isValid());
    return pos_m;
  }

  VariableType type() const {
    D_ASSERT(isValid());
    return type_m;
  }
  void setType(VariableType v) {
    type_m = v;
  }

  Var(VariableType _type, DomainInt _pos) : type_m(_type), pos_m(_pos) {}

  Var(const Var& v) : type_m(v.type_m), pos_m(v.pos_m) {}

  Var() : type_m(VAR_INVALID), pos_m(-1) {}

  friend std::ostream& operator<<(std::ostream& o, const Var& v) {
    return o << "Var. Type:" << v.type_m << " Pos:" << v.pos_m << ".";
  }

  string get_name() const {
    ostringstream o;
    switch(type_m) {
    case VAR_BOOL: o << "bool"; break;
    case VAR_NOTBOOL: o << "!bool"; break;
    case VAR_BOUND: o << "bound"; break;
    case VAR_SPARSEBOUND: o << "sparsebound"; break;
    case VAR_DISCRETE: o << "discrete"; break;
    case VAR_SPARSEDISCRETE: o << "sparsedis"; break;
    case VAR_CONSTANT: break; // No need for name, just the number!
    default: abort();
    }

    o << pos_m;
    return o.str();
  }

  bool operator==(const Var& var) const {
    return type_m == var.type_m && pos_m == var.pos_m;
  }

  bool operator<(const Var& var) const {
    return (type_m < var.type_m) || (type_m == var.type_m && pos_m < var.pos_m);
  }
};

size_t inline hash_value(Var v) {
  return checked_cast<SysInt>(v.pos()) + v.type() * 10000;
}
}

using namespace ProbSpec;

#endif
