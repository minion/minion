// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef MINION_INPUT_VAR_H
#define MINION_INPUT_VAR_H

#include "../system/sys_types.h"
#include <iosfwd>
#include <string>

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

namespace ProbSpec {
// This is the canonical value layout used by both Minion and its bindings.
// Method implementations live in InputVariableDef.h with the solver utilities.
class Var {
  VariableType type_m;
  SysInt pos_m;

public:
  bool isValid() const;
  SysInt pos() const;
  VariableType type() const;
  void setType(VariableType v);
  Var(VariableType type, DomainInt pos);
  Var(const Var&) = default;
  Var& operator=(const Var&) = default;
  Var();
  friend std::ostream& operator<<(std::ostream& o, const Var& v);
  std::string get_name() const;
  bool operator==(const Var& var) const;
  bool operator!=(const Var& var) const;
  bool operator<(const Var& var) const;
};
} // namespace ProbSpec

#endif
