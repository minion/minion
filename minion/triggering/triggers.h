/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

#ifndef _TRIGGER_H
#define _TRIGGER_H

#include "../system/system.h"
#include "../constants.h"

class AbstractConstraint;

/** @brief Represents a change in domain.
 *
 * This is used instead of a simple SysInt as the use of various mappers on
 * variables might mean the domain change needs
 * to be corrected. Every variable should implement the function getDomainChange
 * which uses this and corrects the domain.
 */
class DomainDelta {
  DomainInt domain_change;

public:
  /// This function shouldn't be called directly. This object should be passed
  /// to a variables, which will do any "massaging" which
  /// is required.
  DomainInt XXX_get_domain_diff() {
    return domain_change;
  }

  DomainDelta(DomainInt i) : domain_change(i) {}

  static DomainDelta empty() {
    return DomainDelta(0);
  }
};

#endif
