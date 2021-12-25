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
#include <time.h>    /* time_t, s */

#define MAXCONN 100    /* number of maximum connections */
#define LOGS_LOCATION "../Logs/logs.txt"    /* location of the logs file */

Server serv;    /* main server */
FILE *logs;    /* logs file pointer */

int main(int argc, char **argv) {

  /* Opens logs file. */
  logs = fopen(LOGS_LOCATION, "a+");

  printf("%s: Server started on port %d\n", __func__);
  fprintf(logs, "%s: Server started on port %d\n", __func__);

  /* Sets up the server. */
  int port = setup(&serv, choose_port(argc, argv), MAXCONN, logs);
  printf("%s: Server started on port %d\n", __func__, port);
  fprintf(logs, "%s: Server started on port %d\n", __func__, port);
  printf("%s: Waiting for connetions...\n\n", __func__);
  fprintf(logs, "%s: Waiting for connetions...\n\n", __func__);
  
  fclose(logs);
  /* Server working loop. */
  while (1){
    work_with_socket(&(serv.server_file_descriptor));
  }
}