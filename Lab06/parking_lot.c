#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "parking_lot.h"

#define MAX_LOG_LINES 512
#define MAX_LOG_TEXT 160

typedef struct {
    int id;
} CarArgs;

static sem_t parking_semaphore;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t rng_mutex = PTHREAD_MUTEX_INITIALIZER;

static char event_log[MAX_LOG_LINES][MAX_LOG_TEXT];
static int event_count = 0;

static int parking_capacity = 3;
static int configured_total_cars = 10;

static int cars_arrived = 0;
static int cars_waiting = 0;
static int cars_currently_parked = 0;
static int cars_completed = 0;

static int total_cars_parked = 0;
static double total_wait_time = 0.0;

static int simulation_done = 0;
static int simulation_running = 0;
static int resources_ready = 0;
static int threads_joined = 1;

static pthread_t *car_threads = NULL;
static CarArgs *car_args = NULL;
static int *car_states = NULL;
static int *car_slots = NULL;
static int *slot_owners = NULL;

static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)ms * 1000U);
#endif
}

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int random_between(int min_inclusive, int max_inclusive) {
    int value;
    int span = max_inclusive - min_inclusive + 1;

    pthread_mutex_lock(&rng_mutex);
    value = min_inclusive + (rand() % span);
    pthread_mutex_unlock(&rng_mutex);

    return value;
}

static void format_timestamp(char *buffer, size_t len) {
    time_t now = time(NULL);
    struct tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    strftime(buffer, len, "%a %b %d %H:%M:%S %Y", &tm_now);
}

static void log_event(const char *fmt, ...) {
    char text[MAX_LOG_TEXT];
    char stamp[64];
    va_list args;

    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    format_timestamp(stamp, sizeof(stamp));

    pthread_mutex_lock(&log_mutex);
    if (event_count < MAX_LOG_LINES) {
        snprintf(event_log[event_count], sizeof(event_log[event_count]), "[%s] %s", stamp, text);
        event_count++;
    }
    pthread_mutex_unlock(&log_mutex);
}

static void reset_state(int capacity, int total_cars) {
    parking_capacity = capacity;
    configured_total_cars = total_cars;

    cars_arrived = 0;
    cars_waiting = 0;
    cars_currently_parked = 0;
    cars_completed = 0;
    total_cars_parked = 0;
    total_wait_time = 0.0;
    simulation_done = 0;
    event_count = 0;

    for (int i = 0; i < total_cars; i++) {
        car_states[i] = CAR_NOT_ARRIVED;
        car_slots[i] = -1;
    }

    for (int i = 0; i < capacity; i++) {
        slot_owners[i] = -1;
    }
}

static void *car_thread(void *arg) {
    CarArgs *car = (CarArgs *)arg;
    double wait_start;
    double waited;
    int assigned_slot = -1;

    sleep_ms(random_between(0, 1000));
    log_event("Car %d: Arrived at parking lot", car->id);

    pthread_mutex_lock(&stats_mutex);
    cars_arrived++;
    cars_waiting++;
    car_states[car->id] = CAR_WAITING;
    pthread_mutex_unlock(&stats_mutex);

    wait_start = now_seconds();
    sem_wait(&parking_semaphore);
    waited = now_seconds() - wait_start;

    pthread_mutex_lock(&stats_mutex);
    cars_waiting--;
    cars_currently_parked++;
    total_cars_parked++;
    total_wait_time += waited;

    for (int i = 0; i < parking_capacity; i++) {
        if (slot_owners[i] == -1) {
            slot_owners[i] = car->id;
            assigned_slot = i;
            break;
        }
    }

    car_slots[car->id] = assigned_slot;
    car_states[car->id] = CAR_PARKED;
    pthread_mutex_unlock(&stats_mutex);

    log_event("Car %d: Parked successfully (waited %.2f seconds)", car->id, waited);

    sleep_ms(random_between(1, 5) * 1000);

    log_event("Car %d: Leaving parking lot", car->id);

    pthread_mutex_lock(&stats_mutex);
    cars_currently_parked--;
    cars_completed++;
    if (assigned_slot >= 0 && assigned_slot < parking_capacity) {
        slot_owners[assigned_slot] = -1;
    }
    car_slots[car->id] = -1;
    car_states[car->id] = CAR_DONE;
    if (cars_completed == configured_total_cars) {
        simulation_done = 1;
    }
    pthread_mutex_unlock(&stats_mutex);

    sem_post(&parking_semaphore);
    return NULL;
}

static void join_all_threads_if_needed(void) {
    if (!car_threads || threads_joined) {
        return;
    }

    for (int i = 0; i < configured_total_cars; i++) {
        pthread_join(car_threads[i], NULL);
    }

    threads_joined = 1;
    simulation_running = 0;
}

static void release_resources(void) {
    if (resources_ready) {
        sem_destroy(&parking_semaphore);
        resources_ready = 0;
    }

    free(car_threads);
    car_threads = NULL;
    free(car_args);
    car_args = NULL;
    free(car_states);
    car_states = NULL;
    free(car_slots);
    car_slots = NULL;
    free(slot_owners);
    slot_owners = NULL;
}

int parking_start(int capacity, int total_cars) {
    static int rng_seeded = 0;

    if (capacity <= 0 || total_cars <= 0 || total_cars > PARKING_MAX_CARS || capacity > PARKING_MAX_CARS) {
        return -1;
    }

    pthread_mutex_lock(&stats_mutex);
    if (simulation_running) {
        pthread_mutex_unlock(&stats_mutex);
        return -1;
    }
    pthread_mutex_unlock(&stats_mutex);

    if (!rng_seeded) {
        srand((unsigned int)time(NULL));
        rng_seeded = 1;
    }

    release_resources();

    car_threads = (pthread_t *)calloc((size_t)total_cars, sizeof(pthread_t));
    car_args = (CarArgs *)calloc((size_t)total_cars, sizeof(CarArgs));
    car_states = (int *)calloc((size_t)total_cars, sizeof(int));
    car_slots = (int *)calloc((size_t)total_cars, sizeof(int));
    slot_owners = (int *)calloc((size_t)capacity, sizeof(int));
    if (!car_threads || !car_args || !car_states || !car_slots || !slot_owners) {
        release_resources();
        return -1;
    }

    reset_state(capacity, total_cars);

    if (sem_init(&parking_semaphore, 0, (unsigned int)capacity) != 0) {
        release_resources();
        return -1;
    }
    resources_ready = 1;

    for (int i = 0; i < total_cars; i++) {
        car_args[i].id = i;
        if (pthread_create(&car_threads[i], NULL, car_thread, &car_args[i]) != 0) {
            simulation_done = 1;
            for (int j = 0; j < i; j++) {
                pthread_join(car_threads[j], NULL);
            }
            release_resources();
            return -1;
        }
    }

    threads_joined = 0;

    pthread_mutex_lock(&stats_mutex);
    simulation_running = 1;
    pthread_mutex_unlock(&stats_mutex);

    return 0;
}

int parking_is_running(void) {
    int running;
    pthread_mutex_lock(&stats_mutex);
    running = simulation_running;
    pthread_mutex_unlock(&stats_mutex);
    return running;
}

void parking_get_snapshot(ParkingSnapshot *snapshot) {
    if (!snapshot) {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));

    pthread_mutex_lock(&stats_mutex);
    snapshot->total_cars = configured_total_cars;
    snapshot->capacity = parking_capacity;
    snapshot->arrived = cars_arrived;
    snapshot->waiting = cars_waiting;
    snapshot->parked = cars_currently_parked;
    snapshot->completed = cars_completed;
    snapshot->total_parked = total_cars_parked;
    snapshot->average_wait = (total_cars_parked > 0) ? (total_wait_time / (double)total_cars_parked) : 0.0;
    snapshot->simulation_done = simulation_done;
    snapshot->car_count = configured_total_cars;
    snapshot->slot_count = parking_capacity;

    if (snapshot->car_count > PARKING_MAX_CARS) {
        snapshot->car_count = PARKING_MAX_CARS;
    }
    if (snapshot->slot_count > PARKING_MAX_CARS) {
        snapshot->slot_count = PARKING_MAX_CARS;
    }

    for (int i = 0; i < snapshot->car_count; i++) {
        snapshot->car_states[i] = car_states[i];
    }

    for (int i = 0; i < snapshot->slot_count; i++) {
        snapshot->slot_owners[i] = slot_owners[i];
    }
    pthread_mutex_unlock(&stats_mutex);
}

void parking_wait_until_done(void) {
    int done;

    while (1) {
        pthread_mutex_lock(&stats_mutex);
        done = simulation_done;
        pthread_mutex_unlock(&stats_mutex);
        if (done) {
            break;
        }
        sleep_ms(50);
    }

    join_all_threads_if_needed();
}

void parking_shutdown(void) {
    pthread_mutex_lock(&stats_mutex);
    if (simulation_running) {
        pthread_mutex_unlock(&stats_mutex);
        parking_wait_until_done();
    } else {
        pthread_mutex_unlock(&stats_mutex);
    }

    release_resources();
}
