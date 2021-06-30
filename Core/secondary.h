#ifndef SECONDARY_H_INCLUDED
#define SECONDARY_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>




int choose_port(int argc, char **argv); /* gathers port from cli args or sets it to 80 if unspecified */

char* f_to_s (char * buffer, FILE * infile); /* transports file (or stream) info to string */

void error(char *msg); /* wrapper for perror, used for systemcalls */

void cerror(FILE *stream, char *cause, char *errno, char *shortmsg, char *longmsg); /* returns an error msg to the client */

int prepare(int argc, char **argv, int* portno, int* parentfd, int* optval, struct sockaddr_in* serveraddr, int maxconn); /* prepare for main loop (set server data, bind, listen) */


#endif