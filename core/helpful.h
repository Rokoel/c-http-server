#ifndef HELPFUL_H
#define HELPFUL_H

#include "settings.h"    /* settings' constants */

#include <stdlib.h>    /* malloc, free */
#include <stdio.h>    /* struct FILE & related functions, printf */

/* A macro that prints to console and to logs with time and date. */
#define print(logs, args...) {\
  char *date_and_time = (char*)malloc(sizeof(char)*DATE_TIME_LEN + 1);\
  format_current_time_and_date(date_and_time, 1);\
  printf("[%s] in %s(): ", date_and_time, __func__);\
  printf(args);\
  fprintf(logs, "[%s] in %s(): ", date_and_time, __func__);\
  fprintf(logs, args);\
  free(date_and_time);\
}

int format_current_time_and_date(char* time_and_date_buffer, int should_return_both);

int choose_port(int argc, char **argv, FILE *logs);

char* c_read_file(const char * f_name, short * err, size_t * f_size, FILE *logs);

#endif