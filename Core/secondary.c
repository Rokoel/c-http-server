#include "secondary.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>



int choose_port(int argc, char **argv){
    if (argc != 2) {
        printf("Using default port 80\n");    
        return 80;
    }
    else{
        return atoi(argv[1]);
    }
}

char* f_to_s (char * buffer, FILE * infile){
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

void error(char *msg) {
  perror(msg);
  exit(1);
}

void cerror(FILE *stream, char *cause, char *errno, char *shortmsg, char *longmsg) {
  fprintf(stream, "HTTP/1.1 %s %s\n", errno, shortmsg);
  fprintf(stream, "Content-type: text/html\n");
  fprintf(stream, "\n");
  fprintf(stream, "<html><title>Tiny Error</title>");
  fprintf(stream, "<body bgcolor=""ffffff"">\n");
  fprintf(stream, "%s: %s\n", errno, shortmsg);
  fprintf(stream, "<p>%s: %s\n", longmsg, cause);
  fprintf(stream, "<hr><em>The Tiny Web server</em>\n");
}



int prepare(int argc, char **argv, int* portno, int* parentfd, int* optval, struct sockaddr_in* serveraddr, int maxconn){
  /* choose a port */
  *portno = choose_port(argc, argv);
  /* open socket descriptor */
  *parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (*parentfd < 0){
    error("FATAL ERROR opening socket");
    return 0; /* we don't get here */
  }
  /* allows us to restart server immediately */
  *optval = 1;
  setsockopt(*parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&(*optval), sizeof(int));

  /* bind port to socket */
  bzero((char *) serveraddr, sizeof(*serveraddr)); /* clear server address */
  /* set server's data */
  (*serveraddr).sin_family = AF_INET;
  (*serveraddr).sin_addr.s_addr = htonl(INADDR_ANY);
  (*serveraddr).sin_port = htons((unsigned short)(*portno));

  /* bind */
  if (bind(*parentfd, (struct sockaddr *) serveraddr, sizeof(*serveraddr)) < 0){
    error("FATAL ERROR: binding failed");
    return 0; /* we don't get here */
  }

  /* get us ready to accept connection requests, i.e. listen */
  if (listen(*parentfd, maxconn) < 0){ /* allow maxconn requests (defined in core.c) to queue up */ 
    error("FATAL ERROR: listening failed");
    return 0; /* we don't get here */
  }
  return 1;
}