/** @file helpful.c
 *  @brief File that contains functions that are used for completing sub-tasks.
 * 
 *  This file contains format_current_time_and_date() function, which
 *  is used for getting date and time (this is used for logging system),
 *  choose_port() function, which is used for choosing port for the server before
 *  its setup, and c_read_file() function, that allows us to read strings from the file to the buffer.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 *  @bug No known bugs.
 */
#include "helpful.h"

#include <stdio.h>    /* printf */
#include <stdlib.h>    /* atoi, struct FILE & related functions */
#include <time.h>    /* time, struct time_t & related functions */
#include <string.h>    /* strcpy */

#define FILE_OK 0    /* flag that shows that everything was ok when we read the file */
#define FILE_DOESNOT_EXIST 1    /* flag that shows that the file doesn't exist */
#define FILE_TOO_LARGE 2    /* flag that shows that the file is too large to be read */
#define FILE_READ_ERROR 3    /* flag that shows that an error occured while reading the file */

int format_current_time_and_date(char* time_and_date_buffer, int should_return_both){
  char buf[DATE_TIME_LEN] = {0};
  time_t rawtime = time(NULL);

  if (rawtime == -1) {   
    printf("The time() function failed.\n");
    return 0;
  }
  struct tm *ptm = localtime(&rawtime);
  if (ptm == NULL) {
    printf("The localtime() function failed.\n");
    return 0;
  }
  if(should_return_both == 1){
    strftime(buf, DATE_TIME_LEN, "%d.%m.%y-%T", ptm);
  }
  else{
    strftime(buf, DATE_TIME_LEN, "%d.%m.%y", ptm);
  }
  strcpy(time_and_date_buffer, buf);
  return 1;
}

int choose_port(FILE *logs, int argc, char **argv){
  if (argc != 2) {
    print(logs, "Using default port 80.\n");
    return 80;
  }
  else{
    return atoi(argv[1]);
  }
}

char* c_read_file(const char * f_name, short * err, size_t * f_size) {
  char * buffer;
  size_t length;
  FILE * f = fopen(f_name, "rb");
  size_t read_length;
  
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (length > 1073741824) {
      *err = FILE_TOO_LARGE;
      return NULL;
    }
    buffer = (char *)malloc(length + 1);
    if (length) {
      read_length = fread(buffer, 1, length, f);
      if (length != read_length) {
        free(buffer);
        *err = FILE_READ_ERROR;
        return NULL;
      }
    }
    fclose(f);
    *err = FILE_OK;
    buffer[length] = '\0';
    *f_size = length;
  }
  else {
    *err = FILE_DOESNOT_EXIST;
    return NULL;
  }
  return buffer;
}