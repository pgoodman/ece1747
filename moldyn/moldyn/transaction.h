/* Copyright 2014 Peter Goodman, all rights reserved. */


#ifndef MOLDYN_MOLDYN_SRC_TRANSACTION_H_
#define MOLDYN_MOLDYN_SRC_TRANSACTION_H_

#include <algorithm>
#include <initializer_list>
#include <mutex>
#include <cstdint>

enum {
  MAX_NUM_INDICES = 2,
  MAX_INDEX = 4096 * 2
};

class Transaction {
 public:

  template <typename...Ts>
  inline explicit Transaction(Ts&... addresses)
      : Transaction({&addresses...}) {}

  inline explicit Transaction(std::initializer_list<void *> addresses) {
    max_index = &(indices[0]);
    for (auto address : addresses) {
      *max_index++ = HashAddress(address);
    }
    std::sort(indices, max_index);
    LockIndices();
  }

  ~Transaction(void) {
    UnlockIndices();
  }

 private:
  Transaction(void) = delete;

  inline static uintptr_t HashAddress(void *address) {
    return reinterpret_cast<uintptr_t>(address) % MAX_INDEX;
  }

  inline void LockIndices(void) {
    auto prev_index = std::numeric_limits<uintptr_t>::max();
    for (auto i = indices; i < max_index; ++i) {
      auto index = *i;
      if (index != prev_index) {
        index_locks[index].lock();
        prev_index = index;
      }
    }
  }

  inline void UnlockIndices(void) {
    auto prev_index = std::numeric_limits<uintptr_t>::max();
    for (auto i = max_index; i-- > indices; ) {
      auto index = *i;
      if (index != prev_index) {
        index_locks[index].unlock();
        prev_index = index;
      }
    }
  }

  uintptr_t indices[MAX_NUM_INDICES];
  uintptr_t *max_index;

  // Per-index locks.
  static std::mutex index_locks[MAX_INDEX];
};

#endif  // MOLDYN_MOLDYN_SRC_TRANSACTION_H_
