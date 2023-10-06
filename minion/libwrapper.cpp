// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
 * Functions for using minion as a library.
 */

#include "libwrapper.h"
#include "minion.h"

#ifdef LIBMINION
extern Globals* globals;

void resetMinion() {
  /*
   * FIXME
   *
   * Would've been nice to give globals a proper destructor,
   * but according to g++, deleting types that are incomplete at
   * initialisation would make them never be deleted.
   *
   * Clang might run it, but compiler warnings tell me its UB.
   *
   * When the below deletes are in ~Globals(), it segfaults.
   */
  delete globals->bools_m;
  delete globals->state_m;
  delete globals->queues_m;
  delete globals->options_m;
  delete globals->parData_m;
  delete globals->varContainer_m;
  delete globals;
  globals = new Globals();
}
#endif
