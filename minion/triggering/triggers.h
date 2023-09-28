// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _TRIGGER_H
#define _TRIGGER_H

#include "../constants.h"
#include "../system/system.h"

class AbstractConstraint;

/** @brief Represents a change in domain.
 *
 * This is used instead of a simple SysInt as the use of various mappers on
 * variables might mean the domain change needs
 * to be corrected. Every variable should implement the function getDomainChange
 * which uses this and corrects the domain.
 */
class DomainDelta {
  DomainInt domainChange;

public:
  /// This function shouldn't be called directly. This object should be passed
  /// to a variables, which will do any "massaging" which
  /// is required.
  DomainInt XXX_getDomain_diff() {
    return domainChange;
  }

  DomainDelta(DomainInt i) : domainChange(i) {}

  static DomainDelta empty() {
    return DomainDelta(0);
  }
};

#endif
