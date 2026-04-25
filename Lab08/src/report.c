#include <stdio.h>
#include "report.h"

void print_dataset(const ProcessSet *set) {
    int i;
    printf("\n==== Dataset compartido ====\n");
    printf("Proceso | Llegada | Burst\n");
    printf("--------+---------+------\n");
    for (i = 0; i < set->count; i++) {
        printf("P%-6d | %-7d | %-5d\n",
               set->processes[i].id,
               set->processes[i].arrival_time,
               set->processes[i].burst_time);
    }
}

void print_stats(const ProcessSet *set) {
    int i;
    double total_wait = 0.0;
    double total_turnaround = 0.0;

    printf("\nWaiting Times: [");
    for (i = 0; i < set->count; i++) {
        total_wait += set->processes[i].waiting_time;
        printf("%d", set->processes[i].waiting_time);
        if (i + 1 < set->count) {
            printf(", ");
        }
    }
    printf("]\n");

    printf("Turnaround Times: [");
    for (i = 0; i < set->count; i++) {
        total_turnaround += set->processes[i].turnaround_time;
        printf("%d", set->processes[i].turnaround_time);
        if (i + 1 < set->count) {
            printf(", ");
        }
    }
    printf("]\n");

    printf("Avg Waiting Time: %.2f\n", total_wait / set->count);
    printf("Avg Turnaround Time: %.2f\n", total_turnaround / set->count);
}

void print_gantt(const char *algorithm_name, const int timeline[], int length) {
    int i;

    printf("\nGantt (%s):\n", algorithm_name);
    if (length <= 0) {
        printf("[sin ejecucion]\n");
        return;
    }

    printf("|");
    for (i = 0; i < length; i++) {
        if (timeline[i] < 0) {
            printf(" IDLE |");
        } else {
            printf(" P%d |", timeline[i]);
        }
    }
    printf("\n");

    for (i = 0; i <= length; i++) {
        printf("%4d", i);
    }
    printf("\n");
}
