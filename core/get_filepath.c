/** @file get_filepath.c
 *  @brief Resolves request URLs to files under the views directory.
 */

#include "settings.h"
#include "helpful.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define INTERNAL_PATH_SIZE 4096

static int is_directory(const char *path)
{
    struct stat details;
    return stat(path, &details) == 0 && S_ISDIR(details.st_mode);
}

static int is_regular_file(const char *path)
{
    struct stat details;
    return stat(path, &details) == 0 && S_ISREG(details.st_mode);
}

static int set_path(char *destination, size_t destination_size,
                    const char *path)
{
    int written = snprintf(destination, destination_size, "%s", path);
    return written >= 0 && (size_t)written < destination_size;
}

static int has_extension(const char *path)
{
    const char *last_slash = strrchr(path, '/');
    const char *last_dot = strrchr(path, '.');
    return last_dot != NULL &&
           (last_slash == NULL || last_dot > last_slash);
}

static int has_dot_segment(const char *path)
{
    const char *segment = path;

    while (*segment != '\0') {
        const char *separator = strchr(segment, '/');
        size_t length = separator == NULL
            ? strlen(segment)
            : (size_t)(separator - segment);

        if ((length == 1 && strncmp(segment, ".", 1) == 0) ||
            (length == 2 && strncmp(segment, "..", 2) == 0)) {
            return 1;
        }

        if (separator == NULL) {
            break;
        }
        segment = separator + 1;
    }

    return 0;
}

static int fallback_path(char *filepath, size_t filepath_size, FILE *logs)
{
    if (!set_path(filepath, filepath_size, ERROR_404_INDEX_HTML_ROUTE)) {
        if (logs != NULL) {
            print(logs, "Fallback path does not fit in the destination buffer.\n");
        }
        return 500;
    }

    if (!is_regular_file(filepath)) {
        if (logs != NULL) {
            print(logs, "The 404 fallback file does not exist.\n");
        }
        return 500;
    }

    if (logs != NULL) {
        print(logs, "Filepath is '%s'.\n", filepath);
    }
    return 404;
}

/** @brief Resolves a URL to a file under VIEWS_FOLDER.
 *
 *  Explicit route directories are checked directly. Other paths are
 *  resolved under the main route. Existing files are found with stat(); no
 *  generated route list is needed. Missing paths may still resolve to an
 *  asset belonging to the 404 page.
 *
 *  @param url URL path supplied by the client.
 *  @param filepath Destination for the resolved filesystem path.
 *  @param filepath_size Size of filepath in bytes.
 *  @param logs File pointer containing logs.
 *  @return 200 for a found file, 404 for the fallback page, or 500 on error.
 */
int get_filepath(const char *url, char *filepath, size_t filepath_size, FILE *logs)
{
    char normalized[INTERNAL_PATH_SIZE];
    char candidate[INTERNAL_PATH_SIZE];

    if (url == NULL || filepath == NULL || filepath_size == 0) {
        if (logs != NULL) {
            print(logs, "Invalid arguments passed to get_filepath().\n");
        }
        return 500;
    }

    if (url[0] != '/') {
        if (logs != NULL) {
            print(logs, "Request URL must start with '/'.\n");
        }
        return fallback_path(filepath, filepath_size, logs);
    }

    /* Ignore a query string and fragment when mapping the URL to a file. */
    size_t url_length = strcspn(url, "?#");
    if (url_length == 0 || url_length >= sizeof normalized ||
        has_dot_segment(url)) {
        if (logs != NULL) {
            print(logs, "Request URL is invalid or unsafe.\n");
        }
        return fallback_path(filepath, filepath_size, logs);
    }

    memcpy(normalized, url, url_length);
    normalized[url_length] = '\0';
    if (strchr(normalized, '\\') != NULL) {
        if (logs != NULL) {
            print(logs, "Request URL contains an invalid path separator.\n");
        }
        return fallback_path(filepath, filepath_size, logs);
    }

    /* Directory-like URLs map to index.html, preserving the old behavior. */
    size_t normalized_length = strlen(normalized);
    if (normalized[normalized_length - 1] == '/') {
        if (snprintf(normalized + normalized_length,
                     sizeof normalized - normalized_length,
                     "%s", INDEX_HTML_FILENAME) >=
            (int)(sizeof normalized - normalized_length)) {
            return fallback_path(filepath, filepath_size, logs);
        }
    } else if (!has_extension(normalized)) {
        if (snprintf(normalized + normalized_length,
                     sizeof normalized - normalized_length,
                     "%s", INDEX_HTML_FILENAME_WITH_SLASH) >=
            (int)(sizeof normalized - normalized_length)) {
            return fallback_path(filepath, filepath_size, logs);
        }
    }

    /* Determine whether the first URL component names a route directory. */
    const char *first_component = normalized + 1;
    const char *first_separator = strchr(first_component, '/');
    size_t first_component_length = first_separator == NULL
        ? strlen(first_component)
        : (size_t)(first_separator - first_component);
    char route_directory[INTERNAL_PATH_SIZE];
    int route_written = snprintf(
        route_directory, sizeof route_directory, "%s/%.*s",
        VIEWS_FOLDER, (int)first_component_length, first_component
    );
    int explicit_route = route_written >= 0 &&
                         (size_t)route_written < sizeof route_directory &&
                         is_directory(route_directory);

    const char *route_prefix = explicit_route ? "" : "/main";
    int candidate_written = snprintf(
        candidate, sizeof candidate, "%s%s%s",
        VIEWS_FOLDER, route_prefix, normalized
    );
    if (candidate_written >= 0 &&
        (size_t)candidate_written < sizeof candidate &&
        is_regular_file(candidate)) {
        if (!set_path(filepath, filepath_size, candidate)) {
            if (logs != NULL) {
                print(logs, "Resolved path does not fit in the destination buffer.\n");
            }
            return 500;
        }
        if (logs != NULL) {
            print(logs, "Filepath is '%s'.\n", filepath);
        }
        return 200;
    }

    /* Try the requested path as an asset belonging to the 404 page. */
    candidate_written = snprintf(
        candidate, sizeof candidate, "%s/err404%s", VIEWS_FOLDER, normalized
    );
    if (candidate_written >= 0 &&
        (size_t)candidate_written < sizeof candidate &&
        is_regular_file(candidate)) {
        if (!set_path(filepath, filepath_size, candidate)) {
            return 500;
        }
        if (logs != NULL) {
            print(logs, "Filepath is '%s'.\n", filepath);
        }
        return 200;
    }

    return fallback_path(filepath, filepath_size, logs);
}
