#ifndef BRIDGE_H
#define BRIDGE_H

#include <pthread.h>
#include <time.h>

#define NUM_STUDENTS 10
#define MAX_ON_BRIDGE 4
#define DIR_RIGHT 0
#define DIR_LEFT 1
#define PRIORITY_WAIT_SECONDS 4.0

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t can_cross;
    int on_bridge;
    int current_dir; /* -1 none, 0 right, 1 left */
    int waiting[2];
    struct timespec oldest_wait_start[2];
    double total_wait_seconds;
    int crossed_count;
} Bridge;

typedef struct {
    int id;
    Bridge *bridge;
} StudentArgs;

void bridge_init(Bridge *b);
void bridge_destroy(Bridge *b);
void access_bridge(Bridge *b, int id, int dir, struct timespec *arrive_time);
void exit_bridge(Bridge *b, int id, int dir);
void *student_thread(void *arg);

#endif
