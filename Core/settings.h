#ifndef SETTINGS_H
#define SETTINGS_H

#define MAXCONN 1000    /* number of maximum connections */
#define LOGS_LOCATION "../Logs/log-%s.txt"    /* string with specificators of format for locations of the log files */
#define DATE_TIME_LEN 128    /* length of string that contains date and time */
#define POSSIBLE_ROUTE_LEN 128
#define FILESTRUCTURE_LOCATION "../Filestructure/fs.txt"    /*  */
#define VIEWS_FOLDER "../Public/Views"
#define MAIN_PAGE_FOLDER_WITH_SLASH "main/"
#define ERROR_404_PAGE_FOLDER "err404"
#define ERROR_404_INDEX_HTML_ROUTE "../Public/Views/err404/index.html"
#define INDEX_HTML_FILENAME "index.html"
#define INDEX_HTML_FILENAME_WITH_SLASH "/index.html"

#define MAX_FILENAME_SIZE 64
#define MAX_CONTENT_TYPE_SIZE 64
#define MAX_HEADERS_SIZE 700
#define MAX_REQUESTED_ROUTE_SIZE 32
#define MAX_POSSIBLE_ROUTE_SIZE 34

#endif