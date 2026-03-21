#ifndef PARKING_LOT_H
#define PARKING_LOT_H

#define PARKING_MAX_CARS 64

typedef enum {
    CAR_NOT_ARRIVED = 0,
    CAR_WAITING = 1,
    CAR_PARKED = 2,
    CAR_DONE = 3
} CarState;

typedef struct {
    int total_cars;
    int capacity;
    int arrived;
    int waiting;
    int parked;
    int completed;
    int total_parked;
    double average_wait;
    int simulation_done;
    int car_count;
    int slot_count;
    int car_states[PARKING_MAX_CARS];
    int slot_owners[PARKING_MAX_CARS];
} ParkingSnapshot;

int parking_start(int capacity, int total_cars);
int parking_is_running(void);
void parking_get_snapshot(ParkingSnapshot *snapshot);
void parking_wait_until_done(void);
void parking_shutdown(void);

#endif
