/** @file helpful.c
 *  @brief File that contains functions that are used for completing sub-tasks.
 * 
 *  This file contains format_current_time_date() function, which
 *  is used for getting date and time (this is used for logging system),
 *  choose_port() function, which is used for choosing port for the server before
 *  its setup, and c_read_file() function, that allows us to read strings from the file to the buffer.
 * 
 *  @author Rokoel
**/

#include "helpful.h"

#include <stdio.h>    /* struct FILE & related functions, printf */
#include <stdlib.h>    /* strtol */
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>    /* time, struct time_t & related functions */
#include <string.h>    /* strcpy */

#define FILE_OK 0    /* flag that shows that everything was ok when we read the file */
#define FILE_DOESNOT_EXIST 1    /* flag that shows that the file doesn't exist */
#define FILE_TOO_LARGE 2    /* flag that shows that the file is too large to be read */
#define FILE_READ_ERROR 3    /* flag that shows that an error occured while reading the file */

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_message(FILE *logs, const char *function_name, const char *format, ...)
{
  pthread_mutex_lock(&log_mutex);

  char date_and_time[DATE_TIME_LEN];
  if (!format_current_time_date(date_and_time, 1)) {
    strcpy(date_and_time, "unknown-time");
  }

  printf("[%s] in %s(): ", date_and_time, function_name);
  va_list arguments;
  va_start(arguments, format);
  vprintf(format, arguments);
  va_end(arguments);

  if (logs != NULL) {
    fprintf(logs, "[%s] in %s(): ", date_and_time, function_name);
    va_start(arguments, format);
    vfprintf(logs, format, arguments);
    va_end(arguments);
    fflush(logs);
  }

  pthread_mutex_unlock(&log_mutex);
}


/** @brief Gets current time&date (or just date) to the string.
 * 
 *  This function gets formatted date and time or only formatted date
 *  (depending on the second parameter) into the first parameter.
 *  Calls time() function every time, may be inefficient.
 *  
 *  @param time_and_date_str String that will contain the result of function execution.
 *  @param should_return_both If equals 1, 1-st parameter will contain date and time, it will contain only date otherwise.
 *  @return Returns 1 on success, 0 otherwise.
**/
int format_current_time_date(char* time_and_date_str, int should_return_both){
  if (time_and_date_str == NULL) {
    return 0;
  }

  char buf[DATE_TIME_LEN] = {0};

  /* Gets raw Unix time in seconds. */
  time_t rawtime;
  struct tm local_tm;
  if (time(&rawtime) == (time_t)-1) {
    return 0;
  }
  if (localtime_r(&rawtime, &local_tm) == NULL) {
      printf("The localtime_r() function failed.\n");
      return 0;
  }
  /* Formats and saves date and time in buffer. */
  const char *format = should_return_both == 1
      ? "%d.%m.%y-%T"
      : "%d.%m.%y";
  if (strftime(buf, sizeof buf, format, &local_tm) == 0) {
    return 0;
  }
  /* Copies data from buffer to the first function argument safely. */
  strncpy(time_and_date_str, buf, DATE_TIME_LEN - 1);
  time_and_date_str[DATE_TIME_LEN - 1] = '\0'; /* Ensure null-termination */
  return 1;
}

/** @brief Chooses port for the server.
 * 
 *  This function gets cmd arguments. If there are no
 *  arguments specified, uses default port 80 and gets
 *  port from cmd arguments otherwise.
 *  
 *  @param logs File pointer containing logs.
 *  @param argc Number of program execution arguments.
 *  @param argv Array containing program execution arguments.
 *  @return Returns port.
**/
int choose_port(int argc, char **argv, FILE *logs){
  if (argc != 2) {
    print(logs, "Using default port 8080.\n");
    return 8080;
  }

  char *end = NULL;
  errno = 0;
  long port = strtol(argv[1], &end, 10);
  if (errno != 0 || end == argv[1] || *end != '\0' ||
      port < 1 || port > 65535) {
    print(logs, "Invalid port '%s'; using default port 8080.\n", argv[1]);
    return 8080;
  }

  return (int)port;
}

/** @brief Reads from a file to a buffer.
 * 
 *  Gets file from file path specified in the f_name, tries to read it to a buffer.
 *  If succeeds, err will be FILE_OK. File can also be not found (err is FILE_DOESNOT_EXIST),
 *  too large (err is FILE_TOO_LARGE) and an error may occur when reading file (err is FILE_READ_ERROR).
 *  
 *  @param f_name String that contains file path.
 *  @param err Will contain an error (or its absence) flag after function execution.
 *  @param f_size Will contain file size after function execution.
 *  @return Returns a pointer to a buffer that contains file contents.
**/
char* c_read_file(const char * f_name, short * err, size_t * f_size, FILE *logs){
  if (err == NULL || f_size == NULL || f_name == NULL) {
    return NULL;
  }
  *f_size = 0;

  char *buffer = NULL;
  FILE *f = fopen(f_name, "rb");
  size_t read_length;
  
  /* If the file exists... */
  if(f){
    /* Find its length. */
    if (fseek(f, 0, SEEK_END) != 0) {
      *err = FILE_READ_ERROR;
      fclose(f);
      return NULL;
    }

    long end = ftell(f);
    if (end < 0 || fseek(f, 0, SEEK_SET) != 0) {
      *err = FILE_READ_ERROR;
      fclose(f);
      return NULL;
    }

    size_t length = (size_t)end;
    /* If the file is too large... */
    if(length > 1073741824){
      /* Saves an error and returns NULL. */
      *err = FILE_TOO_LARGE;
      print(logs, "File is too large.\n");
      fclose(f);
      return NULL;
    }
    /* If the file isn't too large... */
    /* Allocates some memory for the buffer. */
    buffer = (char *)malloc(length + 1);
    /* If memory for the buffer was not successfully allocated... */
    if(!buffer){
      *err = FILE_READ_ERROR;
      print(logs, "Memory allocation failed when reading file.\n");
      fclose(f);
      return NULL;
    }
    /* If memory for the buffer was successfully allocated... */
    /* Reads file byte by byte. */
    read_length = fread(buffer, 1, length, f);
    /* If somehow size of what is read isn't equal to size of what is contained in the file... */
    if (length != read_length || ferror(f)) {
      /* Frees the buffer, saves an error and returns NULL. */
      free(buffer);
      *err = FILE_READ_ERROR;
      print(logs, "Error when reading file.\n");
      fclose(f);
      return NULL;
    }
    /* Closes the file. */
    fclose(f);
    /* If none of the errors occured previousely, everything is ok. Saves that information. */
    *err = FILE_OK;
    print(logs, "Successfully read the file.\n");
    /* Terminates the buffer with a null byte. */
    buffer[length] = '\0';
    /* Saves file size. */
    *f_size = length;
    print(logs, "File's length is %zu bytes.\n", length);
    /* Always returns buffer if allocation and reading succeeded. */
    return buffer;
  }
  /* If the file doesn't exist... */
  else{
    /* Saves an error and returns NULL. */
    *err = FILE_DOESNOT_EXIST;
    print(logs, "Could not open file '%s'.\n", f_name);
    return NULL;
  }
}
