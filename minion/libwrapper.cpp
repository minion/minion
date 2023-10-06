// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
 * Functions for using minion as a library.
 */

#include "libwrapper.h"
#include "minion.h"

extern Globals* globals;

void resetMinion() {
  globals = new Globals();
  // TODO: delete
}
