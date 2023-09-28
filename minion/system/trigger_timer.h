// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef TRIG_TIMER_H
#define TRIG_TIMER_H

#include <atomic>

/// This function will cause the boolean passed to be set to 'true' after
/// timeout.
void activateTrigger(std::atomic<bool>*, bool timeoutActive, int timeout, bool CPUTime);

// This takes a point to a bool*, which will be switch when ctrl+c is pressed.
void install_ctrlcTrigger(std::atomic<bool>*);

#endif
