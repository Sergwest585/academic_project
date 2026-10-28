
#ifndef DATA_HEAD_H
#define DATA_HEAD_H

#include <stddef.h>

#define NAMESPACE       "/sergk_shr4_"
#define SHM_NAME        NAMESPACE"shared_mem"
#define SEM_LOCK_NAME   NAMESPACE"shared_lock"
#define SEM_DATA_NAME   NAMESPACE"shared_data"

#define MSG_LEN         256
#define TIME_LEN        64

struct data_payload {
    unsigned long msg_id;
    char          time_str[TIME_LEN];
    int           writer_pid;
    char          message[MSG_LEN];
};

#endif // DATA_HEAD_H