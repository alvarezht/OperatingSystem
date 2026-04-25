#include <stdio.h>
#include <time.h>
#include "process.h"
#include "dataset.h"
#include "scheduler.h"
#include "report.h"

static void run_one_algorithm(const char *title,
                              const ProcessSet *base,
                              void (*runner)(ProcessSet *, ScheduleResult *),
                              int quantum,
                              int use_quantum) {
    ProcessSet working;
    ScheduleResult result;

    copy_process_set(base, &working);
    reset_process_metrics(&working);

    printf("\n========================================\n");
    if (use_quantum) {
        printf("%s (Quantum %d)\n", title, quantum);
    } else {
        printf("%s\n", title);
    }
    printf("========================================\n");

    if (use_quantum) {
        run_rr(&working, &result, quantum);
    } else {
        runner(&working, &result);
    }

    print_gantt(title, result.timeline, result.timeline_length);
    print_stats(&working);
}

int main(void) {
    ProcessSet dataset;
    unsigned int seed = (unsigned int)time(NULL);

    DATASET_TYPE dataset_type = DATASET_FIXED;
    // DATASET_TYPE dataset_type = DATASET_RANDOM;
    build_dataset(&dataset, dataset_type, seed);
    print_dataset(&dataset);

    run_one_algorithm("FIFO Scheduling", &dataset, run_fifo, 0, 0);
    run_one_algorithm("Round Robin Scheduling", &dataset, NULL, 2, 1);
    run_one_algorithm("SJF Scheduling", &dataset, run_sjf, 0, 0);
    run_one_algorithm("SRTF Scheduling", &dataset, run_srtf, 0, 0);

    return 0;
}
