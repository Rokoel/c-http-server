/** @file core.c
 *  @brief File that sets the server up and runs it.
 * 
 *  This file contains main() function. It also sets up server with
 *  setup() function and runs it with work_with_socket() function.
 *  It also defines maximum of connections to the server.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 *  @bug Server dies if a specific request is received.
 *  @todo Logging system.
 */

#include "helpful.h"    /* choose_port */
#include "server.h"    /* struct Server */
#include "socket.h"    /* work_with_socket */

#include <stdio.h>    /* fprintf, struct FILE */
#include <time.h>    /* time, struct time_t & related functions */

#define MAXCONN 100    /* number of maximum connections */

Server serv;    /* main server */
FILE *logs;    /* logs file pointer */

/** @brief Entering point for the compiler.
 * 
 *  This function opens (or creates) today's logs file,
 *  then sets the server up on specified port and starts
 *  the main cycle.
 *  
 *  @param argc Number of program execution arguments.
 *  @param argv Array containing program execution arguments.
 *  @return Should not return.
 */
int main(int argc, char **argv) {
  /* Allocates memory that is enough for assosiated data. */
  char *current_log_file = (char*)malloc(sizeof(char)*32 + 1), *date_and_time = (char*)malloc(sizeof(char)*DATE_TIME_LEN + 1);
  /* Gets today's date that will be used in naming current log file. */
  format_current_time_and_date(date_and_time, 0);
  /* Sets current_log_file to today's log filename. */
  sprintf(current_log_file, LOGS_LOCATION, date_and_time);
  free(date_and_time);
  /* Opens logs file. */
  logs = fopen(current_log_file, "a+");
  free(current_log_file);

  /* Sets up the server. */
  int port = setup(&serv, choose_port(logs, argc, argv), MAXCONN, logs);

  /* Prints information to the console and to the logs file. */
  print(logs, "Server started on port %d\n", port);
  print(logs, "Waiting for connetions...\n\n");

  /* Closes logs file. */
  fclose(logs);

  /* Server working loop. */
  while (1){
    accept_socket_and_open_thread(&(serv.server_file_descriptor), current_log_file);
  }
}