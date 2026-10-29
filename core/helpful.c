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
**/

#include "helpful.h"

#include <stdio.h>    /* struct FILE & related functions, printf */
#include <stdlib.h>    /* atoi */
#include <time.h>    /* time, struct time_t & related functions */
#include <string.h>    /* strcpy */

#define FILE_OK 0    /* flag that shows that everything was ok when we read the file */
#define FILE_DOESNOT_EXIST 1    /* flag that shows that the file doesn't exist */
#define FILE_TOO_LARGE 2    /* flag that shows that the file is too large to be read */
#define FILE_READ_ERROR 3    /* flag that shows that an error occured while reading the file */


/** @brief Gets current time&date (or just date) to the string.
 * 
 *  This function gets formatted date and time or only formatted date
 *  (depending on the second parameter) into the first parameter.
 *  Calls time() function every time, may be inefficient.
 *  
 *  @param time_and_date_buffer String that will contain the result of function execution.
 *  @param should_return_both If equals 1, 1-st parameter will contain date and time, it will contain only date otherwise.
 *  @return Returns 1 on success, 0 otherwise.
**/
int format_current_time_and_date(char* time_and_date_buffer, int should_return_both){
  char buf[DATE_TIME_LEN] = {0};

  /* Gets current time. */
  time_t rawtime = time(NULL);
  if (rawtime == -1) {   
    printf("The time() function failed.\n");
    return 0;
  }
  /* Gets raw Unix time in seconds. */
  struct tm *ptm = localtime(&rawtime);
  if (ptm == NULL) {
    printf("The localtime() function failed.\n");
    return 0;
  }
  /* Formats and saves date and time in buffer. */
  if(should_return_both == 1){
    strftime(buf, DATE_TIME_LEN, "%d.%m.%y-%T", ptm);
  }
  /* Formats and saves only time in buffer. */
  else{
    strftime(buf, DATE_TIME_LEN, "%d.%m.%y", ptm);
  }
  /* Copies data from buffer to the first function argument. */
  strcpy(time_and_date_buffer, buf);
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
  /* If port isn't specified... */
  if(argc != 2){
    /* Uses default port 80. */
    print(logs, "Using default port 80.\n");
    return 80;
  }
  /* If port is specified... */
  else{
    /* Returns integer converted from the string as a port. */
    return atoi(argv[1]);
  }
}

/** @brief Reads from a file to a buffer.
 * 
 *  Gets file from file path specified in the f_name, tries to read it to a buffer.
 *  If succeeds, err will be FILE_OK. File can also be not found (err is FILE_DOESNOT_EXIST),
 *  too large (err is FILE_DOESNOT_EXIST) and an error may occur when reading file (err is FILE_READ_ERROR).
 *  
 *  @param f_name String that contains file path.
 *  @param err Will contain an error (or its absence) flag after function execution.
 *  @param f_size Will contain file size after function execution.
 *  @return Returns a pointer to a buffer that contains file contents.
**/
char* c_read_file(const char * f_name, short * err, size_t * f_size, FILE *logs){
  char * buffer;
  size_t length;
  /* Trys to open a file. */
  FILE * f = fopen(f_name, "rb");
  size_t read_length;
  
  /* If the file exists... */
  if(f){
    /* Find its length. */
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    /* If the file is too large... */
    if(length > 1073741824){
      /* Saves an error and returns NULL. */
      *err = FILE_TOO_LARGE;
      print(logs, "File is too large.\n");
      return NULL;
    }
    /* If the file isn't too large... */
    /* Allocates some memory for the buffer. */
    buffer = (char *)malloc(length + 1);
    /* If memory for the buffer was successfully allocated... */
    if(buffer){
      /* Reads file byte by byte. */
      read_length = fread(buffer, 1, length, f);
      /* If somehow size of what is read isn't equal to size of what is contained in the file... */
      if(length != read_length){
        /* Frees the buffer, saves an error and returns NULL. */
        free(buffer);
        *err = FILE_READ_ERROR;
        print(logs, "Error when reading file.\n");
        return NULL;
      }
    }
    /* Closes the file. */
    fclose(f);
    /* If none of the errors occured previousely, everything is ok. Saves that information. */
    *err = FILE_OK;
    print(logs, "Successfully read the file.\n");
    /* Closes the buffer for it to be complete. */
    buffer[length] = '\0';
    /* Saves file size. */
    *f_size = length;
    print(logs, "File's length is %lu bytes.\n", length);
  }
  /* If the file doesn't exist... */
  else{
    /* Saves an error and returns NULL. */
    *err = FILE_DOESNOT_EXIST;
    print(logs, "File doesn't exist.\n");
    return NULL;
  }
  /* Always returns buffer. */
  return buffer;
}