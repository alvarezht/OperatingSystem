#include "process.h"

void copy_process_set(const ProcessSet *src, ProcessSet *dst) {
    int i;
    dst->count = src->count;
    for (i = 0; i < src->count; i++) {
        dst->processes[i] = src->processes[i];
    }
}

void reset_process_metrics(ProcessSet *set) {
    int i;
    for (i = 0; i < set->count; i++) {
        set->processes[i].remaining_time = set->processes[i].burst_time;
        set->processes[i].start_time = -1;
        set->processes[i].completion_time = -1;
        set->processes[i].waiting_time = 0;
        set->processes[i].turnaround_time = 0;
    }
}
