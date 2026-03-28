#include "bridge.h"

#include <pthread.h>
#include <stdio.h>

int main(void) {
    pthread_t threads[NUM_STUDENTS];
    StudentArgs args[NUM_STUDENTS];
    Bridge bridge;

    bridge_init(&bridge);

    for (int i = 0; i < NUM_STUDENTS; i++) {
        args[i].id = i + 1;
        args[i].bridge = &bridge;
        if (pthread_create(&threads[i], NULL, student_thread, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < NUM_STUDENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    double avg_wait = 0.0;
    if (bridge.crossed_count > 0) {
        avg_wait = bridge.total_wait_seconds / bridge.crossed_count;
    }

    printf("\nAverage waiting time: %.3f seconds\n", avg_wait);

    bridge_destroy(&bridge);
    return 0;
}
