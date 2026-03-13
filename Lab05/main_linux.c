#include "log_processor.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_THREADS 4
#define MAX_THREADS 64

typedef struct ThreadTask {
    const char *filename;
    long start_offset;
    long end_offset;
    int ok;
    LogStats stats;
} ThreadTask;

static long get_file_size(const char *filename) {
    FILE *file = fopen(filename, "rb");
    long size = -1;

    if (file == NULL) {
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }

    size = ftell(file);
    fclose(file);
    return size;
}

static void print_report(const LogStats *stats, double seconds, int threads) {
    long most_count = 0;
    const char *most_url = get_most_visited_url(stats, &most_count);

    printf("Threads: %d\n", threads);
    printf("Processed Lines: %ld\n", stats->parsed_lines);
    printf("Total Unique IPs: %zu\n", get_unique_ip_count(stats));
    if (most_url != NULL) {
        printf("Most Visited URL: %s (%ld times)\n", most_url, most_count);
    } else {
        printf("Most Visited URL: (0 times)\n");
    }
    printf("HTTP Errors: %ld\n", stats->http_errors);
    printf("Elapsed Time: %.6f seconds\n", seconds);
}

static int parse_thread_count(const char *value) {
    long parsed = strtol(value, NULL, 10);

    if (parsed <= 0) {
        return DEFAULT_THREADS;
    }
    if (parsed > MAX_THREADS) {
        return MAX_THREADS;
    }
    return (int)parsed;
}

static void *worker_run(void *arg) {
    ThreadTask *task = (ThreadTask *)arg;
    if (task == NULL) {
        return NULL;
    }

    log_stats_init(&task->stats);
    if (task->stats.ip_counts.buckets == NULL || task->stats.url_counts.buckets == NULL) {
        task->ok = 0;
        return NULL;
    }

    task->ok = (process_log_chunk(task->filename, task->start_offset, task->end_offset, &task->stats) == 0);
    return NULL;
}

static int run_analysis(const char *filename, int threads, LogStats *out_stats, double *out_seconds) {
    ThreadTask *tasks = NULL;
    pthread_t *handles = NULL;
    long file_size = get_file_size(filename);
    long chunk_size = 0;
    int i = 0;
    struct timespec t0;
    struct timespec t1;

    if (filename == NULL || out_stats == NULL || out_seconds == NULL || threads <= 0) {
        return -1;
    }

    if (file_size <= 0) {
        return -1;
    }

    if (threads > file_size) {
        threads = (int)file_size;
        if (threads <= 0) {
            threads = 1;
        }
    }

    tasks = (ThreadTask *)calloc((size_t)threads, sizeof(ThreadTask));
    handles = (pthread_t *)calloc((size_t)threads, sizeof(pthread_t));
    if (tasks == NULL || handles == NULL) {
        free(tasks);
        free(handles);
        return -1;
    }

    chunk_size = file_size / threads;
    if (chunk_size <= 0) {
        chunk_size = file_size;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
        free(tasks);
        free(handles);
        return -1;
    }

    for (i = 0; i < threads; ++i) {
        tasks[i].filename = filename;
        tasks[i].start_offset = i * chunk_size;
        tasks[i].end_offset = (i == threads - 1) ? file_size : (i + 1) * chunk_size;
        tasks[i].ok = 0;

        if (pthread_create(&handles[i], NULL, worker_run, &tasks[i]) != 0) {
            for (int j = 0; j < i; ++j) {
                pthread_join(handles[j], NULL);
                log_stats_free(&tasks[j].stats);
            }
            free(tasks);
            free(handles);
            return -1;
        }
    }

    log_stats_init(out_stats);
    if (out_stats->ip_counts.buckets == NULL || out_stats->url_counts.buckets == NULL) {
        for (i = 0; i < threads; ++i) {
            pthread_join(handles[i], NULL);
            log_stats_free(&tasks[i].stats);
        }
        free(tasks);
        free(handles);
        return -1;
    }

    for (i = 0; i < threads; ++i) {
        pthread_join(handles[i], NULL);
        if (!tasks[i].ok || merge_log_stats(out_stats, &tasks[i].stats) != 0) {
            for (int j = 0; j < threads; ++j) {
                log_stats_free(&tasks[j].stats);
            }
            free(tasks);
            free(handles);
            log_stats_free(out_stats);
            return -1;
        }
        log_stats_free(&tasks[i].stats);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
        free(tasks);
        free(handles);
        log_stats_free(out_stats);
        return -1;
    }

    *out_seconds = (double)(t1.tv_sec - t0.tv_sec) +
                   (double)(t1.tv_nsec - t0.tv_nsec) / 1000000000.0;

    free(tasks);
    free(handles);
    return 0;
}

int main(int argc, char **argv) {
    const char *filename = "access.log";
    int threads = DEFAULT_THREADS;
    LogStats single_stats;
    LogStats multi_stats;
    double single_seconds = 0.0;
    double multi_seconds = 0.0;

    if (argc >= 2) {
        filename = argv[1];
    }
    if (argc >= 3) {
        threads = parse_thread_count(argv[2]);
    }

    printf("=== Multi-Threaded Web Log Analyzer ===\n");
    printf("Input File: %s\n", filename);
    printf("Requested Threads: %d\n\n", threads);

    if (run_analysis(filename, 1, &single_stats, &single_seconds) != 0) {
        fprintf(stderr, "Error: failed to process '%s' in single-thread mode. (%s)\n", filename, strerror(errno));
        return 1;
    }

    if (run_analysis(filename, threads, &multi_stats, &multi_seconds) != 0) {
        fprintf(stderr, "Error: failed to process '%s' in multi-thread mode. (%s)\n", filename, strerror(errno));
        log_stats_free(&single_stats);
        return 1;
    }

    printf("--- TEST: Single-Thread Baseline ---\n");
    print_report(&single_stats, single_seconds, 1);
    printf("\n--- TEST: Multi-Thread Result ---\n");
    print_report(&multi_stats, multi_seconds, threads);

    log_stats_free(&single_stats);
    log_stats_free(&multi_stats);
    return 0;
}
