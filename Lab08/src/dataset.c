#include <stdlib.h>
#include "dataset.h"

static void fill_fixed_dataset(ProcessSet *set) {
    int bursts[6] = {4, 2, 6, 3, 1, 5};
    int arrivals[6] = {0, 1, 2, 3, 4, 5};
    int i;

    set->count = 6;
    for (i = 0; i < set->count; i++) {
        set->processes[i].id = i;
        set->processes[i].arrival_time = arrivals[i];
        set->processes[i].burst_time = bursts[i];
        set->processes[i].remaining_time = bursts[i];
        set->processes[i].start_time = -1;
        set->processes[i].completion_time = -1;
        set->processes[i].waiting_time = 0;
        set->processes[i].turnaround_time = 0;
    }
}

static void fill_random_dataset(ProcessSet *set, unsigned int seed) {
    int i;
    int count;

    srand(seed);
    count = 5 + (rand() % 11);
    if (count > MAX_PROCESSES) {
        count = MAX_PROCESSES;
    }

    set->count = count;
    for (i = 0; i < set->count; i++) {
        set->processes[i].id = i;
        set->processes[i].arrival_time = rand() % 21;
        set->processes[i].burst_time = 1 + (rand() % 10);
        set->processes[i].remaining_time = set->processes[i].burst_time;
        set->processes[i].start_time = -1;
        set->processes[i].completion_time = -1;
        set->processes[i].waiting_time = 0;
        set->processes[i].turnaround_time = 0;
    }
}

void build_dataset(ProcessSet *set, DatasetMode mode, unsigned int seed) {
    if (mode == DATASET_RANDOM) {
        fill_random_dataset(set, seed);
    } else {
        fill_fixed_dataset(set);
    }
}
