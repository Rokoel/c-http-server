/** @file get_filepath.c
 *  @brief File that finds the correct filepath for the file to be returned to the user.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 *  @bug No known bugs.
**/

#include "settings.h"    /* settings' constants */
#include "helpful.h"    /* print macro */

#include <stdlib.h>
#include <string.h>    /* strcpy, strcat, strstr, strlen, strcmp */
#include <stdio.h>    /* struct FILE & related functions */
#include <dirent.h>

/** @brief Function that prepends one string to another.
 * 
 *  This function prepends one string to another. It assumes that
 *  string that another string is being prepended to has enough memory
 *  allocated. And that's alright because we use it the way that we are
 *  sure that enough memory is allocated.
 *  
 *  @param where_to_prepend String to be prepended to.
 *  @param what_to_prepend String that is being prepended.
 *  @return Returns void.
**/
void prepend_string(char* where_to_prepend, const char* what_to_prepend){
    size_t len = strlen(what_to_prepend);
    memmove(where_to_prepend + len, where_to_prepend, strlen(where_to_prepend) + 1);
    memcpy(where_to_prepend, what_to_prepend, len);
}

/** @brief Function that gets filepath from the url.
 * 
 *  This function gets filepath from the url and returns
 *  response code according to the filepath and initial url.
 *  It doesn't change the url.
 *  The function works in 4 steps:
 *  1. If the file isn't specified, function supposes that index.html is requested.
 *  2. It figures out which one of several routes it is. And if finds none, it
 *  supposes that it is either main or err404.
 *  3. Checks if url is one of possible urls (which are saved in fs.txt). If it is, then
 *  filepath is the correct one and it returns 200.
 *  4. If it isn't one of them, it tries to know, if it is the file requested from err404
 *  page (for example, styles.css) or not. If it is, then the correct filepath is saved.
 *  Err404's index.html filepath is saved otherwise. 404 is returned either way.
 * 
 *  @param url String with url to get filepath from.
 *  @param filepath Pointer to get filepath to.
 *  @param logs File pointer containing logs.
 *  @return Returns response code.
**/
int get_filepath(char *url, char *filepath, FILE *logs){
    /* Initializes filepath. It now contains "url". */
    strcpy(filepath, url);

    /* If the requested file isn't specified, so the request doesn't contain a dot... */
    if( !strstr(url, ".") ){
        /* There are two possibilities: */
        /* If file ends with "/"... */
        if( url[strlen(url) - 1] == '/' ){
            /* Adds only "index.html". */
            strcat(filepath, INDEX_HTML_FILENAME);
        }
        /* If file doesn't end with "/"... */
        else {
            /* Adds "/" AND "index.html". */
            strcat(filepath, INDEX_HTML_FILENAME_WITH_SLASH);
        }
    }

    /* Url can start with /[route_name] */
    struct dirent *directory_entry;
    /* Tries to open Public/Views directory. */
    DIR *directory = opendir(VIEWS_FOLDER);
    /* If it couldn't be opened, returns 500. */
    if(directory == NULL){
        print(logs, "Could not open current directory.\n");
        return 500;
    }

    int is_one_of_routes = 0;
    /* Gets all possible routes' names and tries to find them in the beginning of the url. */
    while ((directory_entry = readdir(directory)) != NULL){
        if( strcmp(directory_entry->d_name, ".") && strcmp(directory_entry->d_name, "..")){
            char possible_route[POSSIBLE_ROUTE_LEN] = "/";
            strcat(possible_route, directory_entry->d_name);
            /* If it is one of the routes... */
            if(strncmp(possible_route, filepath, strlen(possible_route)) == 0){
                /* It is enough and we can leave the loop. */
                is_one_of_routes = 1;
                break;
            }
        }
    }
    /* Doesn't forget to close opened directory. */
    closedir(directory);

    /* If the main page is requested... */
    if(is_one_of_routes == 0){
        /* The main page doesn't usually specify "main" route. So it simply starts with /[folder_inside_main...]. */
        /* So in order to be able to get file from the filepath, adding "/main" is needed. */
        prepend_string(filepath, "/main");
    }
    
    ////////////////////!!!!!!!!!!!!!!!! DOES PREPENDING CAUSE MEMORY LEAKS??????????/////////////


    /* Prepends "../Public/Views" for getting file from filepath in the future. */
    prepend_string(filepath, VIEWS_FOLDER);

    /* Supposes that there is no file found. */
    int response_code = 404;
    /* Opens file that contains all possible routes. */
    FILE *fs = fopen(FILESTRUCTURE_LOCATION, "r");
    /* If the file cannot be opened... */
    if(!fs){
        /* Marks it as Internal Server Error and returns associated code. */
        print(logs, "Couldn't open fs.txt.\n");
        return 500;
    }

    /* Initializes possible route. */
    char possible_route[POSSIBLE_ROUTE_LEN] = "";
    /* Possible routes are saved on each line of the previousely opened file (fs). */
    while (fgets(possible_route, POSSIBLE_ROUTE_LEN, fs) != NULL){
        /* As fgets gets string WITH "\n" in the end, removes "\n" from the end of the line. */
        int size = strlen(possible_route);
        possible_route[size-1] = '\0';
        /* If the url is actually one of possible routes... */
        if( !strcmp(filepath, possible_route) ){
            print(logs, "Filepath is '%s'.\n", filepath);
            return 200;
        }
    }

    /* If it isn't one of possible urls... */
    print(logs, "It's 404 Not Found.\n");
    /* Starts from the beginning of the file. */
    fseek(fs, 0, SEEK_SET);
    while (fgets(possible_route, POSSIBLE_ROUTE_LEN, fs) != NULL){
        /* As fgets gets string WITH "\n" in the end, removes "\n" from the end of the line. */
        int size = strlen(possible_route);
        possible_route[size-1] = '\0';
        /* If the url that was gotten from the file is err404 url... */
        if( strstr(possible_route, ERROR_404_PAGE_FOLDER) ){
            char buffer[POSSIBLE_ROUTE_LEN];
            /* Tries to understand if the request is complete nonsense or just err404 page requested data files by */
            /* trying to find similarities in filepath and possible err404 routes (example: tries to find /css/styles.css in the request). */
            strcpy(buffer, possible_route + strlen(VIEWS_FOLDER) + strlen(ERROR_404_PAGE_FOLDER) + 1);
            /* If it isn't nonsense... */
            if(strstr(filepath, buffer)){
                /* Saves requested data's filepath. */
                strcpy(filepath, possible_route);
                print(logs, "Filepath is '%s'.\n", filepath);
                /* And returns 200 because file was actually found. */
                return 200;
            }
        }
    }

    /* If the request is complete nonsense, sets file to be err404's index.html. */
    strcpy(filepath, ERROR_404_INDEX_HTML_ROUTE);
    print(logs, "Filepath is '%s'.\n", filepath);

    /* Response code is obviously 404 in this case. */
    return response_code;
}