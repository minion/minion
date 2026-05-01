// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "../minion.h"
#include "work_steal.h"

namespace WorkSteal {

void bootstrapFirstWorker(WorkStealController& c) {
  c.busyWorkers.fetch_add(1, std::memory_order_acq_rel);
}

void markIdle(WorkStealController& c) {
  // A worker that was busy is leaving busy state. The idle counter is
  // managed by popOrFinish itself (incremented on entry, decremented on
  // dequeue), so we only decrement busy here. We still notify so any
  // sibling polling the all-done predicate re-evaluates it now that
  // busyWorkers may have hit zero.
  c.busyWorkers.fetch_sub(1, std::memory_order_acq_rel);
  {
    std::lock_guard<std::mutex> lock(c.queue_mutex);
  }
  c.queue_cv.notify_all();
}

void donate(WorkStealController& c, WorkItem item) {
  // Claim the donation slot first so popOrFinish's all-done predicate
  // sees us as in-flight even before we grab the mutex.
  c.donationInFlight.fetch_add(1, std::memory_order_acq_rel);
  c.donationsMade.fetch_add(1, std::memory_order_relaxed);
  {
    std::lock_guard<std::mutex> lock(c.queue_mutex);
    c.queue.push(std::move(item));
  }
  c.queue_cv.notify_one();
}

bool popOrFinish(WorkStealController& c, WorkItem& out) {
  std::unique_lock<std::mutex> lock(c.queue_mutex);
  // Mark this worker idle on entry. Donating workers check
  // idleWorkers > 0 before picking a branch to donate; we want them to
  // see us right away so we don't block waiting for a donation that
  // never comes.
  c.idleWorkers.fetch_add(1, std::memory_order_acq_rel);
  for(;;) {
    if(c.stopRequested.load(std::memory_order_acquire)) {
      // Leaving idle without picking up work; restore the counter.
      c.idleWorkers.fetch_sub(1, std::memory_order_acq_rel);
      return false;
    }
    if(!c.queue.empty()) {
      out = std::move(c.queue.front());
      c.queue.pop();
      // The donor that pushed this item incremented donationInFlight;
      // we now own that slot. Decrement it AND increment busy under the
      // queue mutex so the all-done predicate sees a consistent snapshot.
      c.donationInFlight.fetch_sub(1, std::memory_order_acq_rel);
      c.idleWorkers.fetch_sub(1, std::memory_order_acq_rel);
      c.busyWorkers.fetch_add(1, std::memory_order_acq_rel);
      c.workItemsTaken.fetch_add(1, std::memory_order_relaxed);
      return true;
    }
    // All-done predicate: queue empty, no busy workers, no donations
    // mid-push. Read both under the mutex — donate() incrementing
    // donationInFlight is racy w.r.t. our wait, but the donor's
    // notify_one is serialised on the same mutex as our predicate, so
    // we'll be woken by it.
    int busy = c.busyWorkers.load(std::memory_order_acquire);
    int inflight = c.donationInFlight.load(std::memory_order_acquire);
    if(busy == 0 && inflight == 0) {
      // Nobody is searching, nobody can donate. Tell the rest of the
      // pool to stop and return.
      c.stopRequested.store(true, std::memory_order_release);
      c.idleWorkers.fetch_sub(1, std::memory_order_acq_rel);
      lock.unlock();
      c.queue_cv.notify_all();
      return false;
    }
    // Someone is busy; wait for either a donation or for them to go idle.
    c.queue_cv.wait(lock);
  }
}

}  // namespace WorkSteal
