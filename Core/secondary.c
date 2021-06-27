#include "secondary.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int choose_port(int argc, char **argv){
    if (argc != 2) {
        printf("Using default port 80\n");    
        return 80;
    }
    else{
        return atoi(argv[1]);
    }
}

char* f_to_s (char const * buffer, FILE * infile){
    /* declare a file pointer */
    long numbytes;
    /* Get the number of bytes */
    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);
    /* reset the file position indicator to 
    the beginning of the file */
    fseek(infile, 0L, SEEK_SET);
    /* copy all the text into the buffer */
    fread(buffer, sizeof(char), numbytes, infile);
    return buffer;
}