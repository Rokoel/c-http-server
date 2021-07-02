/* 
 * tiny.c - a minimal HTTP server that serves static and
 *          dynamic content with the GET method. Neither 
 *          robust, secure, nor modular. Use for instructional
 *          purposes only.
 *          Dave O'Hallaron, Carnegie Mellon
 *
 *          usage: tiny <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <ctype.h>

#include "secondary.h"

#define BUFSIZE 1024
#define MAXERRS 16
#define MAXCONN 5

extern char **environ; /* the environment */
int i = 0; /* unconnected index for fors */


int main(int argc, char **argv) {

  /* variables for connection management */
  int parentfd;          /* parent socket */
  int childfd;           /* child socket */
  int portno;            /* port to listen on */
  int clientlen;         /* byte size of client's address */
  struct hostent *hostp; /* client host info */
  char *hostaddrp;       /* dotted decimal host addr string */
  int optval;            /* flag value for setsockopt */
  //typedef struct sockaddr_in S_ADDR;
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */

  /* variables for connection I/O */
  FILE *stream;          /* stream version of childfd */
  char buf[BUFSIZE];     /* message buffer */
  char method[BUFSIZE];  /* request method */
  char uri[BUFSIZE];     /* request uri */
  char version[BUFSIZE]; /* request method */
  char filename[BUFSIZE];/* path derived from uri */
  char filetype[BUFSIZE];/* path derived from uri */
  char cgiargs[BUFSIZE]; /* cgi argument list */
  char *p;               /* temporary pointer */
  int is_static;         /* static request? */
  struct stat sbuf;      /* file status */
  int fd;                /* static content filedes */
  int pid;               /* process id from fork */
  int wait_status;       /* status from wait */
  char request_data[BUFSIZE]; /* headers and data in one */


  /*=========================PREPARE-FOR-MAIN-LOOP====================================*/
  prepare(argc, argv, &portno, &parentfd, &optval, &serveraddr, MAXCONN);
  /*==================================================================================*/

  /*==================================================================================*/
  /* 
   * main loop: wait for a connection request, parse HTTP,
   * serve requested content, close connection.
   */

  printf("Server ready!\n");
  printf("Waiting for connetions...\n\n");
  clientlen = sizeof(clientaddr);
  while (1) {

    /* wait for a connection request */
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (childfd < 0) 
      error("ERROR on accept");
    
    /* determine who sent the message */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    
    /* open the child socket descriptor as a stream */
    if ((stream = fdopen(childfd, "r+")) == NULL)
      error("ERROR on fdopen");

    /* get the HTTP request line */
    fgets(buf, BUFSIZE, stream);
    printf("%s", buf);
    sscanf(buf, "%s %s %s\n", method, uri, version);


    /* read (and ignore) the HTTP headers. May be used to work with post requests */

    printf("REQUEST: \n%s\n", f_to_s(request_data, stream));
    printf("\n");


    /* parse the uri [crufty] */ /* GET routing */
    if (!strstr(uri, "cgi-bin")) { /* static content */
      is_static = 1;
      strcpy(cgiargs, "");
      //strcpy(filename, "..");
      //strcat(filename, uri);
      if (uri[strlen(uri)-1] == '/')
	      strcpy(filename, "../Public/Views/index.html");
      else
        if (uri[strlen(uri)-1] == 'o')
          strcpy(filename, "../Public/Views/icon.ico");
        else
          if (uri[strlen(uri)-1] == 's'){
            if (uri[strlen(uri)-2] == 's')
              strcpy(filename, "../Public/Views/css/styles.css");
            if (uri[strlen(uri)-2] == 'j')
              strcpy(filename, "../Public/Views/js/scripts.js");
          }
    }
    else { /* dynamic content */
      is_static = 0;
      p = index(uri, '?');
      if (p) {
	      strcpy(cgiargs, p+1);
	      *p = '\0';
      }
      else {
	      strcpy(cgiargs, "");
      }
      strcpy(filename, ".");
      strcat(filename, uri);
    }

    /* make sure the file exists */
    if (stat(filename, &sbuf) < 0) {
      cerror(stream, filename, "404", "Not found", 
	     "Tiny couldn't find this file");
      fclose(stream);
      close(childfd);
      continue;
    }

    /* serve static content */
    if (is_static) {
      if (strstr(filename, ".html"))
	      strcpy(filetype, "text/html");
      else if (strstr(filename, ".gif"))
	      strcpy(filetype, "image/gif");
      else if (strstr(filename, ".jpg"))
	      strcpy(filetype, "image/jpg");
      else if (strstr(filename, ".css"))
	      strcpy(filetype, "text/css");
      else if (strstr(filename, ".js"))
	      strcpy(filetype, "text/javascript");
      else if (strstr(filename, ".ico"))
	      strcpy(filetype, "image/x-icon");
      else 
	      strcpy(filetype, "text/plain");

      /* print response header */
      fprintf(stream, "HTTP/1.1 200 OK\n");
      fprintf(stream, "Server: Tiny Web Server\n");
      fprintf(stream, "Content-length: %d\n", (int)sbuf.st_size);
      fprintf(stream, "Content-type: %s\n", filetype);
      fprintf(stream, "\r\n"); 
      fflush(stream);

      /* Use mmap to return arbitrary-sized response body */
      fd = open(filename, O_RDONLY);
      p = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      fwrite(p, 1, sbuf.st_size, stream);
      munmap(p, sbuf.st_size);
    }

    /* serve dynamic content */
    else {
      /* make sure file is a regular executable file */
      if (!(S_IFREG & sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)) {
	      cerror(stream, filename, "403", "Forbidden", 
	      "You are not allow to access this item");
	      fclose(stream);
	      close(childfd);
	      continue;
      }

      /* a real server would set other CGI environ vars as well*/
      setenv("QUERY_STRING", cgiargs, 1); 

      /* print first part of response header */
      sprintf(buf, "HTTP/1.1 200 OK\n");
      write(childfd, buf, strlen(buf));
      sprintf(buf, "Server: Tiny Web Server\n");
      write(childfd, buf, strlen(buf));

      /* create and run the child CGI process so that all child
         output to stdout and stderr goes back to the client via the
         childfd socket descriptor */
      pid = fork();
      if (pid < 0) {
	      perror("ERROR in fork");
	      exit(1);
      }
      else if (pid > 0) { /* parent process */
	      wait(&wait_status);
      }
      else { /* child  process*/
	      close(0); /* close stdin */
	      dup2(childfd, 1); /* map socket to stdout */
	      dup2(childfd, 2); /* map socket to stderr */
	      if (execve(filename, NULL, environ) < 0) {
	        perror("ERROR in execve");
	      }
      }
    }

    /* clean up */
    fclose(stream);
    close(childfd);

    /* clear request_data */
    for(i = 0; i < sizeof(request_data)/sizeof(request_data[0]); i++){
      request_data[i] = 0;
    }
  }
}