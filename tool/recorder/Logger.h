#ifndef LOGGER_H
#define LOGGER_H

#include <cinttypes>

#define LOGLINE(X, ...) \ 
    printf("[%s:%d] " X "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif // LOGGER_H