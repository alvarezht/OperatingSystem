#ifndef DATASET_H
#define DATASET_H

#include "process.h"

typedef enum {
    DATASET_FIXED = 0,
    DATASET_RANDOM = 1
} DatasetMode;

void build_dataset(ProcessSet *set, DatasetMode mode, unsigned int seed);

#endif
