// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

inline void inputPrint(std::ostream& o, const Var& v) {
  o << getState().getInstance()->vars.getName(v);
}
