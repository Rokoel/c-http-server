#ifndef HELPFUL_H
#define HELPFUL_H

#include "settings.h"    /* settings' constants */

#include <stdio.h>

void log_message(FILE *logs, const char *function_name, const char *format, ...);

/* Prints to the console and, when available, to the log file. */
#define print(logs, ...) \
  log_message((logs), __func__, __VA_ARGS__)

int format_current_time_date(char* time_and_date_buffer, int should_return_both);

int choose_port(int argc, char **argv, FILE *logs);

char* c_read_file(const char * f_name, short * err, size_t * f_size, FILE *logs);

#endif
