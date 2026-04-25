#include <stdio.h>
#include "scheduler.h"

typedef struct {
    int data[MAX_PROCESSES * 4];
    int front;
    int back;
} Queue;

static void queue_init(Queue *q) {
    q->front = 0;
    q->back = 0;
}

static int queue_is_empty(const Queue *q) {
    return q->front == q->back;
}

static void queue_push(Queue *q, int value) {
    q->data[q->back++] = value;
}

static int queue_pop(Queue *q) {
    return q->data[q->front++];
}

static void clear_result(ScheduleResult *result) {
    result->timeline_length = 0;
}

static void push_timeline(ScheduleResult *result, int process_id) {
    if (result->timeline_length < MAX_TIMELINE) {
        result->timeline[result->timeline_length++] = process_id;
    }
}

static void compute_metrics(ProcessSet *set) {
    int i;
    for (i = 0; i < set->count; i++) {
        Process *p = &set->processes[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->burst_time;
        if (p->waiting_time < 0) {
            p->waiting_time = 0;
        }
    }
}

static void print_arrivals(const ProcessSet *set, int time, int arrived_flags[]) {
    int i;
    for (i = 0; i < set->count; i++) {
        if (!arrived_flags[i] && set->processes[i].arrival_time == time) {
            printf("[t=%d] Process %d (Burst %d): Arrived\n", time, set->processes[i].id, set->processes[i].burst_time);
            arrived_flags[i] = 1;
        }
    }
}

static int all_completed(const ProcessSet *set) {
    int i;
    for (i = 0; i < set->count; i++) {
        if (set->processes[i].remaining_time > 0) {
            return 0;
        }
    }
    return 1;
}

void run_fifo(ProcessSet *set, ScheduleResult *result) {
    int done = 0;
    int time = 0;
    int i;
    int arrived_flags[MAX_PROCESSES] = {0};

    clear_result(result);

    while (done < set->count) {
        int selected = -1;
        int best_arrival = 2147483647;
        int best_id = 2147483647;

        print_arrivals(set, time, arrived_flags);

        for (i = 0; i < set->count; i++) {
            Process *p = &set->processes[i];
            if (p->remaining_time > 0 && p->arrival_time <= time) {
                if (p->arrival_time < best_arrival || (p->arrival_time == best_arrival && p->id < best_id)) {
                    best_arrival = p->arrival_time;
                    best_id = p->id;
                    selected = i;
                }
            }
        }

        if (selected == -1) {
            push_timeline(result, -1);
            time++;
            continue;
        }

        if (set->processes[selected].start_time == -1) {
            set->processes[selected].start_time = time;
            printf("[t=%d] Process %d: Started\n", time, set->processes[selected].id);
        }

        while (set->processes[selected].remaining_time > 0) {
            push_timeline(result, set->processes[selected].id);
            set->processes[selected].remaining_time--;
            time++;
            print_arrivals(set, time, arrived_flags);
        }

        set->processes[selected].completion_time = time;
        printf("[t=%d] Process %d: Completed\n", time, set->processes[selected].id);
        done++;
    }

    compute_metrics(set);
}

void run_rr(ProcessSet *set, ScheduleResult *result, int quantum) {
    int time = 0;
    int completed = 0;
    int arrived_flags[MAX_PROCESSES] = {0};
    int in_queue[MAX_PROCESSES] = {0};
    Queue q;

    clear_result(result);
    queue_init(&q);

    while (completed < set->count) {
        int i;

        print_arrivals(set, time, arrived_flags);
        for (i = 0; i < set->count; i++) {
            Process *p = &set->processes[i];
            if (p->remaining_time > 0 && p->arrival_time <= time && !in_queue[i]) {
                queue_push(&q, i);
                in_queue[i] = 1;
            }
        }

        if (queue_is_empty(&q)) {
            push_timeline(result, -1);
            time++;
            continue;
        }

        {
            int idx = queue_pop(&q);
            int slice;
            Process *p = &set->processes[idx];

            in_queue[idx] = 0;
            if (p->start_time == -1) {
                p->start_time = time;
            }

            printf("[t=%d] Process %d: Started/Resumed\n", time, p->id);
            slice = (p->remaining_time < quantum) ? p->remaining_time : quantum;

            for (i = 0; i < slice; i++) {
                push_timeline(result, p->id);
                p->remaining_time--;
                time++;

                print_arrivals(set, time, arrived_flags);
                {
                    int j;
                    for (j = 0; j < set->count; j++) {
                        Process *np = &set->processes[j];
                        if (np->remaining_time > 0 && np->arrival_time <= time && !in_queue[j] && j != idx) {
                            queue_push(&q, j);
                            in_queue[j] = 1;
                        }
                    }
                }
            }

            if (p->remaining_time == 0) {
                p->completion_time = time;
                completed++;
                printf("[t=%d] Process %d: Completed\n", time, p->id);
            } else {
                queue_push(&q, idx);
                in_queue[idx] = 1;
                printf("[t=%d] Process %d: Preempted (remaining %d)\n", time, p->id, p->remaining_time);
            }
        }
    }

    compute_metrics(set);
}

void run_sjf(ProcessSet *set, ScheduleResult *result) {
    int time = 0;
    int completed = 0;
    int arrived_flags[MAX_PROCESSES] = {0};

    clear_result(result);

    while (completed < set->count) {
        int i;
        int selected = -1;
        int best_burst = 2147483647;
        int best_arrival = 2147483647;

        print_arrivals(set, time, arrived_flags);

        for (i = 0; i < set->count; i++) {
            Process *p = &set->processes[i];
            if (p->remaining_time > 0 && p->arrival_time <= time) {
                if (p->burst_time < best_burst ||
                    (p->burst_time == best_burst && p->arrival_time < best_arrival) ||
                    (p->burst_time == best_burst && p->arrival_time == best_arrival && p->id < set->processes[selected].id)) {
                    selected = i;
                    best_burst = p->burst_time;
                    best_arrival = p->arrival_time;
                }
            }
        }

        if (selected == -1) {
            push_timeline(result, -1);
            time++;
            continue;
        }

        if (set->processes[selected].start_time == -1) {
            set->processes[selected].start_time = time;
            printf("[t=%d] Process %d: Started\n", time, set->processes[selected].id);
        }

        while (set->processes[selected].remaining_time > 0) {
            push_timeline(result, set->processes[selected].id);
            set->processes[selected].remaining_time--;
            time++;
            print_arrivals(set, time, arrived_flags);
        }

        set->processes[selected].completion_time = time;
        completed++;
        printf("[t=%d] Process %d: Completed\n", time, set->processes[selected].id);
    }

    compute_metrics(set);
}

void run_srtf(ProcessSet *set, ScheduleResult *result) {
    int time = 0;
    int current = -1;
    int arrived_flags[MAX_PROCESSES] = {0};

    clear_result(result);

    while (!all_completed(set)) {
        int i;
        int selected = -1;
        int best_remaining = 2147483647;
        int best_arrival = 2147483647;

        print_arrivals(set, time, arrived_flags);

        for (i = 0; i < set->count; i++) {
            Process *p = &set->processes[i];
            if (p->remaining_time > 0 && p->arrival_time <= time) {
                if (p->remaining_time < best_remaining ||
                    (p->remaining_time == best_remaining && p->arrival_time < best_arrival) ||
                    (p->remaining_time == best_remaining && p->arrival_time == best_arrival && p->id < set->processes[selected].id)) {
                    selected = i;
                    best_remaining = p->remaining_time;
                    best_arrival = p->arrival_time;
                }
            }
        }

        if (selected == -1) {
            push_timeline(result, -1);
            current = -1;
            time++;
            continue;
        }

        if (current != selected) {
            if (current != -1 && set->processes[current].remaining_time > 0) {
                printf("[t=%d] Process %d: Preempted (remaining %d)\n", time, set->processes[current].id, set->processes[current].remaining_time);
            }

            if (set->processes[selected].start_time == -1) {
                set->processes[selected].start_time = time;
            }

            printf("[t=%d] Process %d: Started/Resumed\n", time, set->processes[selected].id);
            current = selected;
        }

        push_timeline(result, set->processes[selected].id);
        set->processes[selected].remaining_time--;
        time++;

        if (set->processes[selected].remaining_time == 0) {
            set->processes[selected].completion_time = time;
            printf("[t=%d] Process %d: Completed\n", time, set->processes[selected].id);
            current = -1;
        }
    }

    compute_metrics(set);
}
