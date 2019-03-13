#ifndef TRIGGER_HANDLING_HFPQWENMJFA
#define TRIGGER_HANDLING_HFPQWENMJFA

#include <assert.h>
#include <errno.h>
#include <iostream>
#include <ostream>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

struct EndOfSearch {
  std::string reason;

  EndOfSearch(const std::string& r) : reason(r) {}
};

namespace TriggerEvent {
enum Event { none, ctrl_c, timeout };

volatile Event trigger_event_X;

void setTrigger(Event e) {
  if(trigger_event_X == none) {
    trigger_event_X = e;
  } else {
    abort();
  }
}

Event getTrigger() {
  return trigger_event_X;
}
} // namespace TriggerEvent

void ctrlCFunctionTrigger(int /* signum */) {
  TriggerEvent::setTrigger(TriggerEvent::ctrl_c);
}

void triggerFunction(int /* signum */) {
  TriggerEvent::setTrigger(TriggerEvent::timeout);
}

#ifdef NO_SYSTEM
void setupTriggers(bool, int) {}
#else
void setupTriggers(bool timeoutActive,
                    int timeout) // CPU_time = false -> real time
{
  assert(TriggerEvent::trigger_event_X == TriggerEvent::none);

  struct sigaction act;
  act.sa_flags = 0;
  sigfillset(&act.sa_mask);

  act.sa_handler = triggerFunction;
  sigaction(SIGXCPU, &act, NULL);
  sigaction(SIGALRM, &act, NULL);

  act.sa_handler = ctrlCFunctionTrigger;
  sigaction(SIGINT, &act, NULL);

  if(timeoutActive) {
    rlimit lim;
    lim.rlim_cur = timeout;
    lim.rlim_max = timeout + 10;
    setrlimit(RLIMIT_CPU, &lim);
  }
}
#endif

inline void checkTriggers() {
  if(TriggerEvent::getTrigger() != TriggerEvent::none) {
    if(TriggerEvent::getTrigger() == TriggerEvent::ctrl_c)
      throw EndOfSearch("ctrl+c");
    else
      throw EndOfSearch("time");
  }
}

#endif
