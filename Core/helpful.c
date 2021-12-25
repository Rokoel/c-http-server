#include "helpful.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_OK 0
#define FILE_DOESNOT_EXIST 1
#define FILE_TOO_LARGE 2
#define FILE_READ_ERROR 3

int choose_port(int argc, char **argv){
  if (argc != 2) {
    printf("CHOOSE_PORT: Using default port 80\n");    
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