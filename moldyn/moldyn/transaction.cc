/* Copyright 2014 Peter Goodman, all rights reserved. */

#include "transaction.h"

std::mutex Transaction::index_locks[MAX_INDEX];
