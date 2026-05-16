#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Memory Configuration */
#define NUM_FRAMES 100
#define PAGE_SIZE 256
#define MAX_VA 0xFFFFU
#define OFFSET_MASK 0xFFU
#define VPN_SHIFT 8
#define VPN_MASK 0xFFU
#define MAX_ITERATIONS 1000

/* ANSI Color Codes */
#define COLOR_RESET "\x1b[0m"
#define COLOR_FREE  "\x1b[32m"
#define COLOR_SYS   "\x1b[31m"
#define COLOR_P1    "\x1b[34m"
#define COLOR_P2    "\x1b[33m"

/* Frame state enumeration */
typedef enum {
    FRAME_FREE,
    FRAME_OCCUPIED
} FrameState;

/* Physical frame representation */
typedef struct {
    FrameState state;
    int owner;
} Frame;

/* Page Table Entry */
typedef struct {
    bool valid;
    int pfn;
} PTE;

/* Page Table representation */
typedef struct {
    PTE *entries;
    int num_pages;
    int pid;
} PageTable;

/* Translation result codes */
typedef enum {
    TRANS_OK,
    VA_OUT_OF_RANGE,
    VPN_OUT_OF_RANGE,
    PAGE_NOT_MAPPED
} TransResult;

#endif /* MAIN_H */
