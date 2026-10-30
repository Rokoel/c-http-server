#ifndef SETTINGS_H
#define SETTINGS_H

#define MAXCONN 1000    /* number of maximum connections */
#ifndef ROKOEL_LOGS_FOLDER
#define ROKOEL_LOGS_FOLDER "../logs"
#endif

#ifndef ROKOEL_VIEWS_FOLDER
#define ROKOEL_VIEWS_FOLDER "../public/views"
#endif

#define LOGS_LOCATION ROKOEL_LOGS_FOLDER "/log-%s.txt"
#define DATE_TIME_LEN 128    /* length of string that contains date and time */
#define VIEWS_FOLDER ROKOEL_VIEWS_FOLDER
#define ERROR_404_INDEX_HTML_ROUTE ROKOEL_VIEWS_FOLDER "/err404/index.html"
#define INDEX_HTML_FILENAME "index.html"
#define INDEX_HTML_FILENAME_WITH_SLASH "/index.html"

#define MAX_CONTENT_TYPE_SIZE 64
#define MAX_HEADERS_SIZE 700

#endif
