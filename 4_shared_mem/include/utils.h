
#ifndef UTILS_H
#define UTILS_H

#include <time.h>

static inline void ctime_better(char *buf, size_t bufsz, time_t t) {
   struct tm tm_now = {0};
   localtime_r(&t, &tm_now);
   strftime(buf, bufsz, "%Y-%m-%d %H:%M:%S", &tm_now);
   return;
}

#endif // UTILS_H
