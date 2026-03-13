#include "log_processor.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BUCKETS 4096
#define MAX_LOG_LINE 2048

static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c = 0;

    while ((c = (unsigned char)*str++) != 0) {
        hash = ((hash << 5) + hash) + (unsigned long)c;
    }

    return hash;
}

static char *duplicate_string(const char *src) {
    size_t len = strlen(src);
    char *copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, src, len + 1);
    return copy;
}

int hash_map_init(HashMap *map, size_t bucket_count) {
    if (map == NULL || bucket_count == 0) {
        return -1;
    }

    map->buckets = (HashEntry **)calloc(bucket_count, sizeof(HashEntry *));
    if (map->buckets == NULL) {
        return -1;
    }

    map->bucket_count = bucket_count;
    map->size = 0;
    return 0;
}

void hash_map_free(HashMap *map) {
    size_t i = 0;

    if (map == NULL || map->buckets == NULL) {
        return;
    }

    for (i = 0; i < map->bucket_count; ++i) {
        HashEntry *entry = map->buckets[i];
        while (entry != NULL) {
            HashEntry *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }

    free(map->buckets);
    map->buckets = NULL;
    map->bucket_count = 0;
    map->size = 0;
}

int hash_map_increment(HashMap *map, const char *key, long delta) {
    unsigned long hash = 0;
    size_t index = 0;
    HashEntry *entry = NULL;

    if (map == NULL || map->buckets == NULL || key == NULL) {
        return -1;
    }

    hash = hash_djb2(key);
    index = (size_t)(hash % map->bucket_count);

    entry = map->buckets[index];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            entry->count += delta;
            return 0;
        }
        entry = entry->next;
    }

    entry = (HashEntry *)malloc(sizeof(HashEntry));
    if (entry == NULL) {
        return -1;
    }

    entry->key = duplicate_string(key);
    if (entry->key == NULL) {
        free(entry);
        return -1;
    }

    entry->count = delta;
    entry->next = map->buckets[index];
    map->buckets[index] = entry;
    map->size += 1;

    return 0;
}

size_t hash_map_size(const HashMap *map) {
    if (map == NULL) {
        return 0;
    }
    return map->size;
}

void log_stats_init(LogStats *stats) {
    if (stats == NULL) {
        return;
    }

    if (hash_map_init(&stats->ip_counts, DEFAULT_BUCKETS) != 0) {
        stats->ip_counts.buckets = NULL;
        stats->ip_counts.bucket_count = 0;
        stats->ip_counts.size = 0;
    }

    if (hash_map_init(&stats->url_counts, DEFAULT_BUCKETS) != 0) {
        stats->url_counts.buckets = NULL;
        stats->url_counts.bucket_count = 0;
        stats->url_counts.size = 0;
    }

    stats->http_errors = 0;
    stats->parsed_lines = 0;
}

void log_stats_free(LogStats *stats) {
    if (stats == NULL) {
        return;
    }

    hash_map_free(&stats->ip_counts);
    hash_map_free(&stats->url_counts);
    stats->http_errors = 0;
    stats->parsed_lines = 0;
}

static int map_is_ready(const HashMap *map) {
    return map != NULL && map->buckets != NULL && map->bucket_count > 0;
}

static int parse_log_line(const char *line, char *ip_out, size_t ip_size, char *url_out, size_t url_size, int *status_out) {
    const char *quote_start = NULL;
    const char *quote_end = NULL;
    const char *method_start = NULL;
    const char *url_start = NULL;
    const char *url_end = NULL;
    const char *status_start = NULL;
    const char *space_after_ip = NULL;
    size_t ip_len = 0;
    size_t url_len = 0;

    if (line == NULL || ip_out == NULL || url_out == NULL || status_out == NULL) {
        return -1;
    }

    space_after_ip = strchr(line, ' ');
    if (space_after_ip == NULL) {
        return -1;
    }

    ip_len = (size_t)(space_after_ip - line);
    if (ip_len == 0 || ip_len >= ip_size) {
        return -1;
    }

    memcpy(ip_out, line, ip_len);
    ip_out[ip_len] = '\0';

    quote_start = strchr(space_after_ip, '"');
    if (quote_start == NULL) {
        return -1;
    }

    quote_end = strchr(quote_start + 1, '"');
    if (quote_end == NULL) {
        return -1;
    }

    method_start = quote_start + 1;
    while (*method_start != '\0' && isspace((unsigned char)*method_start)) {
        method_start++;
    }

    url_start = strchr(method_start, ' ');
    if (url_start == NULL || url_start >= quote_end) {
        return -1;
    }

    url_start++;
    while (url_start < quote_end && isspace((unsigned char)*url_start)) {
        url_start++;
    }

    url_end = url_start;
    while (url_end < quote_end && !isspace((unsigned char)*url_end)) {
        url_end++;
    }

    url_len = (size_t)(url_end - url_start);
    if (url_len == 0 || url_len >= url_size) {
        return -1;
    }

    memcpy(url_out, url_start, url_len);
    url_out[url_len] = '\0';

    status_start = quote_end + 1;
    while (*status_start != '\0' && isspace((unsigned char)*status_start)) {
        status_start++;
    }

    if (*status_start == '\0') {
        return -1;
    }

    *status_out = atoi(status_start);
    return 0;
}

static int seek_to_next_line(FILE *file) {
    int ch = 0;

    if (file == NULL) {
        return -1;
    }

    do {
        ch = fgetc(file);
        if (ch == EOF) {
            return 1;
        }
    } while (ch != '\n');

    return 0;
}

int process_log_chunk(const char *filename, long start_offset, long end_offset, LogStats *out_stats) {
    FILE *file = NULL;
    char line[MAX_LOG_LINE];
    int status = 0;

    if (filename == NULL || out_stats == NULL || start_offset < 0 || end_offset < start_offset) {
        return -1;
    }

    if (!map_is_ready(&out_stats->ip_counts) || !map_is_ready(&out_stats->url_counts)) {
        return -1;
    }

    file = fopen(filename, "rb");
    if (file == NULL) {
        return -1;
    }

    if (fseek(file, start_offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    if (start_offset > 0) {
        int seek_result = seek_to_next_line(file);
        if (seek_result == 1) {
            fclose(file);
            return 0;
        }
        if (seek_result != 0) {
            fclose(file);
            return -1;
        }
    }

    while (1) {
        long line_start = ftell(file);
        char ip[64];
        char url[512];
        int http_code = 0;

        if (line_start < 0 || line_start >= end_offset) {
            break;
        }

        if (fgets(line, sizeof(line), file) == NULL) {
            break;
        }

        if (parse_log_line(line, ip, sizeof(ip), url, sizeof(url), &http_code) != 0) {
            continue;
        }

        status = hash_map_increment(&out_stats->ip_counts, ip, 1);
        if (status != 0) {
            fclose(file);
            return -1;
        }

        status = hash_map_increment(&out_stats->url_counts, url, 1);
        if (status != 0) {
            fclose(file);
            return -1;
        }

        if (http_code >= 400 && http_code <= 599) {
            out_stats->http_errors += 1;
        }

        out_stats->parsed_lines += 1;
    }

    fclose(file);
    return 0;
}

int merge_log_stats(LogStats *dest, const LogStats *src) {
    size_t i = 0;

    if (dest == NULL || src == NULL) {
        return -1;
    }

    if (!map_is_ready(&dest->ip_counts) || !map_is_ready(&dest->url_counts) ||
        !map_is_ready(&src->ip_counts) || !map_is_ready(&src->url_counts)) {
        return -1;
    }

    for (i = 0; i < src->ip_counts.bucket_count; ++i) {
        HashEntry *entry = src->ip_counts.buckets[i];
        while (entry != NULL) {
            if (hash_map_increment(&dest->ip_counts, entry->key, entry->count) != 0) {
                return -1;
            }
            entry = entry->next;
        }
    }

    for (i = 0; i < src->url_counts.bucket_count; ++i) {
        HashEntry *entry = src->url_counts.buckets[i];
        while (entry != NULL) {
            if (hash_map_increment(&dest->url_counts, entry->key, entry->count) != 0) {
                return -1;
            }
            entry = entry->next;
        }
    }

    dest->http_errors += src->http_errors;
    dest->parsed_lines += src->parsed_lines;
    return 0;
}

size_t get_unique_ip_count(const LogStats *stats) {
    if (stats == NULL) {
        return 0;
    }
    return hash_map_size(&stats->ip_counts);
}

const char *get_most_visited_url(const LogStats *stats, long *count) {
    size_t i = 0;
    const char *best_url = NULL;
    long best_count = 0;

    if (count != NULL) {
        *count = 0;
    }

    if (stats == NULL || !map_is_ready(&stats->url_counts)) {
        return NULL;
    }

    for (i = 0; i < stats->url_counts.bucket_count; ++i) {
        HashEntry *entry = stats->url_counts.buckets[i];
        while (entry != NULL) {
            if (entry->count > best_count) {
                best_count = entry->count;
                best_url = entry->key;
            }
            entry = entry->next;
        }
    }

    if (count != NULL) {
        *count = best_count;
    }

    return best_url;
}
