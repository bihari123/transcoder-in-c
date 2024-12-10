// monitor.h
#ifndef MONITOR_H
#define MONITOR_H

#include "types.h"

void print_stats(TranscoderContext *ctx);
void *monitor_thread_func(void *arg);

#endif // MONITOR_H
