#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

#define MAX_TIMELINE 2000

typedef struct {
    int timeline[MAX_TIMELINE];
    int timeline_length;
} ScheduleResult;

void run_fifo(ProcessSet *set, ScheduleResult *result);
void run_rr(ProcessSet *set, ScheduleResult *result, int quantum);
void run_sjf(ProcessSet *set, ScheduleResult *result);
void run_srtf(ProcessSet *set, ScheduleResult *result);

#endif
