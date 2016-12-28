#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

int clean_stdin(void) {
    while (getchar()!='\n');
    return 1;
}

bool convertStringToInt(char * string, int * result) {
    char * end = NULL;
    errno = 0;
    long temp = strtol(string, &end, 10);

    if (*end == '\0' && errno != ERANGE && temp >= INT_MIN && temp <= INT_MAX) {
        *result = (int)temp;
        return true;
    }
    return false;
}

bool convertStringToDouble(char * string, double * result) {
    char * end = NULL;
    errno = 0;
    double temp = strtod(string, &end);

    if (*end == '\0' && errno != ERANGE) {
        *result = temp;
        return true;
    }
    return false;
}
