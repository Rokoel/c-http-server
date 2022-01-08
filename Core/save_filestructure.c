/** @file save_filestructure.c
 *  @brief File that contains a function for saving the filestructure.
 * 
 *  This file contains the function save_filestructure(), which saves
 *  filestructure (i. e. all possible routes) in a file. Is a little bit
 *  time-consuming, so it shouldn't be used very frequently.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 *  @bug No known bugs.
**/

#include <stdio.h>    /* struct FILE & related functions */
#include <dirent.h>    /* struct dirent, DIR & related funtions */
#include <string.h>    /* strcmp, strcat */
#include <unistd.h>    /* sleep */

#include "settings.h"    /* settings' constants */


/** @brief Function that saves all possible routes into a file.
 * 
 *  This function is recursive. It saves all possible routes into the file,
 *  which is specified as "file" variable, separated by '\n' starting from the
 *  trunk directory.
 *  
 *  @param trunk String that contains path to a strating directory.
 *  @param file File to save the structure to.
 *  @return Returns void.
**/
void save_filestructure(char *trunk, FILE *file){

    /* Opens directory specified in "trunk" variable. */
    struct dirent *directory_entry;
    DIR *directory = opendir(trunk);

    /* If the directory can't be opened, it means that trunk is a path to a file. */
    if (!directory){
        /* So it saves the path and ends the recursion. */
        fprintf(file, "%s\n", trunk);
        return;
    }

    /* Here the process of finding all possible files and sub-directories of a trunk comes. */
    while ((directory_entry = readdir(directory)) != NULL){
        /* New file/sub-directory - new branch. */
        char branch[POSSIBLE_ROUTE_LEN] = "";
        /* If directory_entry isn't "." or ".." (these ones are also gotten by the readdir() function). */
        if( strcmp(directory_entry->d_name, ".") && strcmp(directory_entry->d_name, "..")){
            /* Copies trunk, adds current sub-directory/file. */
            strcat(branch, trunk);
            strcat(branch, "/");
            strcat(branch, directory_entry->d_name);
            /* Finds all sub-directories of new sub-directory. */
            save_filestructure(branch, file);
        }
    }
    /* Closes the directory after it is not needed. */
    closedir(directory);
}

void *update_filestructure(){
    while(1){
        /* Clear the file. */
        FILE *f = fopen(FILESTRUCTURE_LOCATION, "w");
        fclose(f);
        /* Save filestructure. */
        f = fopen(FILESTRUCTURE_LOCATION, "a");
        save_filestructure(VIEWS_FOLDER, f);
        fclose(f);
        /* Sleep for 2 hours. */
        sleep(7200);
    }
    return NULL;
}