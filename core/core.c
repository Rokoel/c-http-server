/** @file core.c
 *  @brief File that sets the server up and runs it.
 * 
 *  This file contains main() function. It also sets up server with
 *  setup() function and runs it with work_with_socket() function.
 *  It also defines maximum of connections to the server.
 * 
 *  @author Rokoel
**/

#include "helpful.h"    /* choose_port */
#include "server.h"    /* struct Server */
#include "socket.h"    /* work_with_socket */
#include "settings.h"    /* settings' constants */

#include <stdio.h>    /* fprintf, struct FILE */
#include <stdlib.h>
#include <signal.h>

/** @brief Entering point for the compiler.
 * 
 *  This function opens (or creates) today's logs file,
 *  then sets the server up on specified port and starts
 *  the main cycle.
 *  
 *  @param argc Number of program execution arguments.
 *  @param argv Array containing program execution arguments.
 *  @return Should not return.
**/
int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);

  Server serv = {0};
  char current_log_file_name[DATE_TIME_LEN + sizeof LOGS_LOCATION];
  char date_and_time[DATE_TIME_LEN];

  /* Gets today's date that will be used in naming current log file. */
  if (!format_current_time_date(date_and_time, 0)) {
    fprintf(stderr, "Error occurred while formatting date and time.\n");
    return 1;
  }
  /* Sets current_log_file to today's log filename. */
  if (snprintf(current_log_file_name, sizeof current_log_file_name,
               LOGS_LOCATION, date_and_time) >=
      (int)sizeof current_log_file_name) {
    fprintf(stderr, "Log filename is too long.\n");
    return 1;
  }

  /* Opens logs file. */
  FILE *logs = fopen(current_log_file_name, "a+");
  if (logs == NULL) {
    perror("fopen log file");
    return 1;
  }

  /* Sets up the server. */
  int port = setup(&serv, choose_port(argc, argv, logs), MAXCONN, logs);
  if (port < 0) {
    fclose(logs);
    return 1;
  }

  /* Prints information to the console and to the logs file. */
  print(logs, "Server started on port %d\n", port);
  print(logs, "Waiting for connetions...\n\n");

  /* The worker threads open the log file independently. */
  fclose(logs);

  /* Server working loop. */
  while (1) {
    accept_socket_and_open_thread(&serv.server_file_descriptor,
                                  current_log_file_name);
  }
}
