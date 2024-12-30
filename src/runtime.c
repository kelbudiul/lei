#include <stdlib.h>
#include <stdio.h>

// String to integer
int atoi(const char* str) {
    return strtol(str, NULL, 10);
}

// String to float
double atof(const char* str) {
    return strtod(str, NULL);
}

// Integer to string
char* itoa(int num, char* buffer, int base) {
    snprintf(buffer, 32, "%d", num);
    return buffer;
}

// Float to string
char* ftoa(double num, char* buffer, int precision) {
    snprintf(buffer, 32, "%.*f", precision, num);
    return buffer;
}
