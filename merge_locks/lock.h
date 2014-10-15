/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef MERGE_LOCK_H_
#define MERGE_LOCK_H_

#include <pthread.h>
#include <initializer_list>

#ifndef MERGE_LOCKS_PRIVATE
# define MERGE_LOCKS_PRIVATE private
#endif

// Forward declarations.
class MergeLock;

// Represents a critical section.
class CriticalSection {
 public:
  inline explicit CriticalSection(pthread_mutex_t *locked_mutex_)
      : locked_mutex(locked_mutex_) {}

  // Unlock the critical section.
  inline void ReleaseLockSet(void) {
    pthread_mutex_unlock(locked_mutex);
  }

 private:
  CriticalSection(void) = delete;

  // Mutex guarding the associated critical section.
  pthread_mutex_t *locked_mutex;
};

namespace detail {

// Acquires a set of merge locks.
CriticalSection AcquireLockSet(MergeLock **begin, MergeLock **end);

}  // namespace detail

// Acquires a set of merge locks.
template <typename... Locks>
CriticalSection AcquireLockSet(Locks... locks) {
  enum {
    LOCK_SET_SIZE = sizeof...(Locks)
  };
  MergeLock *lock_set[LOCK_SET_SIZE] = {locks...};
  detail::AcquireLockSet(&(lock_set[0]), &(lock_set[LOCK_SET_SIZE]));
}

// A mutual exclusion lock that can be merged with another MergeLock.
class MergeLock {
 public:
  // Initialize the merge lock.
  MergeLock(void);

  // Initialize the merge lock with some specific pthread mutex attribute.
  explicit MergeLock(pthread_mutexattr_t *attr);

  // Destroy the merge lock.
  ~MergeLock(void);

  // Split all merge locks.
  //
  // Note: This should only be invoked during a "quiescence" period where no
  //       MergeLocks are held.
  static void SplitAll(void);

 MERGE_LOCKS_PRIVATE:
  // Returns the parent of this lock at some point in time. The parent pointer
  // if subject to change.
  MergeLock *Id(void);

  // Mutex that guards either:
  //    1) A critical section within the application's code that is logically
  //       associated with one or more locks.
  //    2) The modification of the `parent` pointer and the `generation` value.
  pthread_mutex_t mutex;

  // The parent of this lock. Initially a lock points to itself; however, if it
  // gets merged then it might point to another lock. If it points to another
  // lock, then that means that we should really be acquiring the other lock.
  MergeLock * volatile parent;

  // The current "generation" number. This is a mechanism for splitting locks
  // apart. If the current value of this doesn't match with some global value
  // then it means we can't follow `parent` pointers until `generation` is
  // brought into sync with the expected global value.
  unsigned long generation;
};

#endif  // MERGE_LOCK_H_
