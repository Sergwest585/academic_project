#ifndef DATA_HEAD_H
#define DATA_HEAD_H

#include <stddef.h>

#define SHM_NAME        "/shared_mem"
#define SHM_LOCK_NAME   "/shared_lock"
#define SHM_DATA_NAME   "/shared_data"

#define MSG_LEN         256
#define TIME_LEN        64

struct data_payload {
    unsigned long id;
    char          time_str[TIME_LEN];
    int           writer_pid;
    char          message[MSG_LEN];
};

#endif // DATA_HEAD_H