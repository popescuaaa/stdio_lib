#ifndef UTILITY_H
#define UTILITY_H

#include <string.h>
#include <stdlib.h>
#include "constant_values.h"

int open_mode(const char *mode)
{
    if (strcmp(mode, "r") == 0) return READ_ONLY_MODE;
    if (strcmp(mode, "r+") == 0) return READ_WRITE_MODE;
    if (strcmp(mode, "w") == 0) return WRITE_CREATE_TRUNCATE_MODE;
    if (strcmp(mode, "w+") == 0) return READ_WRITE_CREATE_TRUNCATE_MODE;
    if (strcmp(mode, "a") == 0) return WRITE_ONLY_CREATE_APPEND_MODE;
    if (strcmp(mode, "a+") == 0) return READ_WRITE_CREATE_APPEND_MODE;

    return 0;
}


#endif /* UTILITY_H */