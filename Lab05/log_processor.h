#ifndef LOG_PROCESSOR_H
#define LOG_PROCESSOR_H

#include <stddef.h>

typedef struct HashEntry {
    char *key;
    long count;
    struct HashEntry *next;
} HashEntry;

typedef struct HashMap {
    HashEntry **buckets;
    size_t bucket_count;
    size_t size;
} HashMap;

typedef struct LogStats {
    HashMap ip_counts;
    HashMap url_counts;
    long http_errors;
    long parsed_lines;
} LogStats;

int hash_map_init(HashMap *map, size_t bucket_count);
void hash_map_free(HashMap *map);
int hash_map_increment(HashMap *map, const char *key, long delta);
size_t hash_map_size(const HashMap *map);

void log_stats_init(LogStats *stats);
void log_stats_free(LogStats *stats);
int process_log_chunk(const char *filename, long start_offset, long end_offset, LogStats *out_stats);
int merge_log_stats(LogStats *dest, const LogStats *src);
size_t get_unique_ip_count(const LogStats *stats);
const char *get_most_visited_url(const LogStats *stats, long *count);

#endif
