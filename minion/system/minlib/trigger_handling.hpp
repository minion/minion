#ifndef TRIGGER_HANDLING_HFPQWENMJFA
#define TRIGGER_HANDLING_HFPQWENMJFA

#include <ostream>
#include <iostream>
#include <sys/types.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

struct EndOfSearch {
  std::string reason;

  EndOfSearch(const std::string &r) : reason(r) {}
};

namespace TriggerEvent {
enum Event { none, ctrl_c, timeout };

volatile Event trigger_event_X;

void set_trigger(Event e) {
  if (trigger_event_X == none) {
    trigger_event_X = e;
  } else {
    abort();
  }
}

Event get_trigger() { return trigger_event_X; }
}

void ctrlc_function_trigger(int /* signum */) { TriggerEvent::set_trigger(TriggerEvent::ctrl_c); }

void trigger_function(int /* signum */) { TriggerEvent::set_trigger(TriggerEvent::timeout); }

#ifdef NO_SYSTEM
void setup_triggers(bool, int) {}
#else
void setup_triggers(bool timeout_active,
                    int timeout) // CPU_time = false -> real time
{
  assert(TriggerEvent::trigger_event_X == TriggerEvent::none);

  struct sigaction act;
  act.sa_flags = 0;
  sigfillset(&act.sa_mask);

  act.sa_handler = trigger_function;
  sigaction(SIGXCPU, &act, NULL);
  sigaction(SIGALRM, &act, NULL);

  act.sa_handler = ctrlc_function_trigger;
  sigaction(SIGINT, &act, NULL);

  if (timeout_active) {
    rlimit lim;
    lim.rlim_cur = timeout;
    lim.rlim_max = timeout + 10;
    setrlimit(RLIMIT_CPU, &lim);
  }
}
#endif

inline void check_triggers() {
  if (TriggerEvent::get_trigger() != TriggerEvent::none) {
    if (TriggerEvent::get_trigger() == TriggerEvent::ctrl_c)
      throw EndOfSearch("ctrl+c");
    else
      throw EndOfSearch("time");
  }
}

#endif
