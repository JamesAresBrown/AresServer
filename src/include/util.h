/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_UTIL_H
#define MYSERVER_UTIL_H

#include <cstdio>
#include <cstdlib>

inline void ErrorIf(bool condition, const char *errmsg) {
    if (condition) {
        perror(errmsg);
        exit(EXIT_FAILURE);
    }
}

#endif //MYSERVER_UTIL_H
