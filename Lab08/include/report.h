#ifndef REPORT_H
#define REPORT_H

#include "process.h"

void print_dataset(const ProcessSet *set);
void print_stats(const ProcessSet *set);
void print_gantt(const char *algorithm_name, const int timeline[], int length);

#endif
