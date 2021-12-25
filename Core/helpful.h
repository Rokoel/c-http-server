#ifndef HELPFUL_H
#define HELPFUL_H

#include <stdlib.h>

int choose_port(int argc, char **argv);

char* c_read_file(const char * f_name, short * err, size_t * f_size);

#endif