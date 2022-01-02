#ifndef HELPFUL_H
#define HELPFUL_H

#include <stdlib.h>    /* struct FILE & related functions */
#include <stdio.h>    /* printf */

#define LOGS_LOCATION "../Logs/log-%s.txt"    /* string with specificators of format for locations of the log files */
#define DATE_TIME_LEN 256    /* length of string that contains date and time */

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

int choose_port(FILE *logs, int argc, char **argv);

char* c_read_file(const char * f_name, short * err, size_t * f_size);

#endif