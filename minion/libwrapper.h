// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
 * Functions for using minion as a library.
 */

#ifndef _WRAPPER_H
#define _WRAPPER_H

/*
 * void resetMinion:
 *
 *  Reset all global variables in minion to their initial values.
 *
 */
void resetMinion();
int minion_main(int argc, char** argv);

#endif
