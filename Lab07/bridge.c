#include "bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static double diff_seconds(const struct timespec *start, const struct timespec *end) {
    double s = (double)(end->tv_sec - start->tv_sec);
    double ns = (double)(end->tv_nsec - start->tv_nsec) / 1e9;
    return s + ns;
}

static const char *dir_name(int dir) {
    return dir == DIR_LEFT ? "Left" : "Right";
}

static const char *bridge_dir_name(int dir) {
    if (dir == DIR_LEFT) return "Left";
    if (dir == DIR_RIGHT) return "Right";
    return "None";
}

static unsigned int prng_next(unsigned int *state) {
    *state = (*state * 1103515245u) + 12345u;
    return *state;
}

static int is_priority_direction(Bridge *b, int dir) {
    if (b->waiting[dir] <= 0) return 0;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return diff_seconds(&b->oldest_wait_start[dir], &now) >= PRIORITY_WAIT_SECONDS;
}

static void print_status(Bridge *b, const char *action, int id, int dir) {
    printf("Inge %02d %s %s (on bridge: %d, queue L:%d R:%d, bridge dir:%s)\n",
           id, action, dir_name(dir), b->on_bridge, b->waiting[DIR_LEFT],
           b->waiting[DIR_RIGHT], bridge_dir_name(b->current_dir));
    fflush(stdout);
}

void bridge_init(Bridge *b) {
    pthread_mutex_init(&b->lock, NULL);
    pthread_cond_init(&b->can_cross, NULL);
    b->on_bridge = 0;
    b->current_dir = -1;
    b->waiting[DIR_RIGHT] = 0;
    b->waiting[DIR_LEFT] = 0;
    b->total_wait_seconds = 0.0;
    b->crossed_count = 0;
}

void bridge_destroy(Bridge *b) {
    pthread_mutex_destroy(&b->lock);
    pthread_cond_destroy(&b->can_cross);
}

void access_bridge(Bridge *b, int id, int dir, struct timespec *arrive_time) {
    pthread_mutex_lock(&b->lock);

    clock_gettime(CLOCK_REALTIME, arrive_time);
    if (b->waiting[dir] == 0) {
        b->oldest_wait_start[dir] = *arrive_time;
    }
    b->waiting[dir]++;
    print_status(b, "arrives wanting to go", id, dir);

    while (1) {
        int other = 1 - dir;
        int can_direction = (b->current_dir == -1 || b->current_dir == dir);
        int can_capacity = (b->on_bridge < MAX_ON_BRIDGE);

        /* Simple priority when bridge is empty. */
        int priority_block = 0;
        if (b->current_dir == -1 && b->waiting[other] > 0) {
            int other_priority = is_priority_direction(b, other);
            int this_priority = is_priority_direction(b, dir);
            if (other_priority && !this_priority) {
                priority_block = 1;
            }
        }

        if (can_direction && can_capacity && !priority_block) break;
        pthread_cond_wait(&b->can_cross, &b->lock);
    }

    b->waiting[dir]--;
    if (b->waiting[dir] == 0) {
        b->oldest_wait_start[dir].tv_sec = 0;
        b->oldest_wait_start[dir].tv_nsec = 0;
    }

    if (b->current_dir == -1) b->current_dir = dir;
    b->on_bridge++;

    struct timespec entered;
    clock_gettime(CLOCK_REALTIME, &entered);
    b->total_wait_seconds += diff_seconds(arrive_time, &entered);
    b->crossed_count++;

    print_status(b, "crosses to the", id, dir);
    pthread_mutex_unlock(&b->lock);
}

void exit_bridge(Bridge *b, int id, int dir) {
    pthread_mutex_lock(&b->lock);
    b->on_bridge--;
    print_status(b, "exits bridge from", id, dir);

    if (b->on_bridge == 0) {
        b->current_dir = -1;
    }

    pthread_cond_broadcast(&b->can_cross);
    pthread_mutex_unlock(&b->lock);
}

void *student_thread(void *arg) {
    StudentArgs *s = (StudentArgs *)arg;

    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(s->id * 2654435761u);
    int arrive_delay = (int)(prng_next(&seed) % 6u); /* 0-5s */
    sleep((unsigned int)arrive_delay);

    int dir = (int)(prng_next(&seed) % 2u); /* 0 right, 1 left */

    struct timespec arrive_time;
    access_bridge(s->bridge, s->id, dir, &arrive_time);

    int crossing_time = 1 + (int)(prng_next(&seed) % 3u); /* 1-3s */
    sleep((unsigned int)crossing_time);

    exit_bridge(s->bridge, s->id, dir);
    return NULL;
}
