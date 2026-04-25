#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 32

typedef struct {
    int id;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int start_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
} Process;

typedef struct {
    Process processes[MAX_PROCESSES];
    int count;
} ProcessSet;

void copy_process_set(const ProcessSet *src, ProcessSet *dst);
void reset_process_metrics(ProcessSet *set);

#endif
