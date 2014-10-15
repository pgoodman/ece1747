/* Copyright 2014 Peter Goodman, all rights reserved. */

#define MERGE_LOCKS_PRIVATE public

#include "lock.h"

#include <algorithm>
#include <cassert>

namespace {
static unsigned long g_generation = 0;

static MergeLock **SortIds(MergeLock **begin_, MergeLock **end) {
  for (auto begin = begin_; begin < end; ++begin) {
    *begin = (*begin)->Id();
  }
  std::sort(begin_, end);
  return std::unique(begin_, end);
}

// Acquire the combined lock of a lock set.
static pthread_mutex_t *AcquireLock(MergeLock *lock) {
  for (;;) {
    lock = lock->Id();
    pthread_mutex_lock(&(lock->mutex));
    if (lock == lock->parent) {
      return &(lock->mutex);
    }
    pthread_mutex_unlock(&(lock->mutex));
  }
  __builtin_unreachable();
  return nullptr;
}

}  // namespace
namespace detail {

// Acquires a set of merge locks.
CriticalSection AcquireLockSet(MergeLock **begin, MergeLock **end) {
  MergeLock *prev = nullptr;

  end = SortIds(begin, end);
  auto max_ptr = end - 1;
  auto max = *max_ptr;

  for (; begin < max_ptr; ) {
    auto min = *begin;

    // Don't need to process this lock. The locks go in ascending order so if
    // we come across a "smaller" lock than the previous one then we should
    // have already processed it.
    if (prev >= min) {
      assert(prev == min);
      ++begin;
      continue;
    }

    prev = min;
    pthread_mutex_lock(&(min->mutex));

    // The parent pointer of `min` was changed by a concurrently executing
    // thread since the invocation of `SortIds`.
    if (min != min->parent) {
      *begin = min->parent->Id();  // Add the parent of `min` to the set.
      end = SortIds(begin, end);  // Re-sort an calculate the max.
      max_ptr = end - 1;
      max = *max_ptr;

    } else {
      ++begin;  // We don't need to add anything new to the set.
    }

    assert(min <= max);
    min->parent = max;  // Merge the lock.
    min->generation = g_generation;  // Fix the generation number.

    pthread_mutex_unlock(&(min->mutex));
  }
  return CriticalSection(AcquireLock(max));
}

}  // namespace detail

// Initialize the merge lock.
MergeLock::MergeLock(void)
    : parent(this),
      generation(g_generation) {
  pthread_mutex_init(&mutex, NULL);
}

// Initialize the merge lock with some specific pthread mutex attribute.
MergeLock::MergeLock(pthread_mutexattr_t *attr)
    : parent(this),
      generation(g_generation) {
  pthread_mutex_init(&mutex, attr);
}

// Destroy the merge lock.
MergeLock::~MergeLock(void) {
  pthread_mutex_destroy(&mutex);
}

// Split all merge locks.
void MergeLock::SplitAll(void) {
  ++g_generation;
}

// Returns the parent of this lock at some point in time. The parent pointer
// if subject to change.
MergeLock *MergeLock::Id(void) {
  if (generation != g_generation) {
    return this;
  }

  auto curr = this;
  auto next = parent;
  do {
    curr = next;
    next = next->parent;
  } while (curr != next);

  return curr;
}
