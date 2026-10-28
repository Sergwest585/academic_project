
#ifndef DATA_HEAD_H
#define DATA_HEAD_H

#include <stddef.h>

#define SHM_KEY_PATH   "./shm_key_file"
#define SHM_KEY_ID     'A'

#define MSG_LEN         256
#define TIME_LEN        64


enum {
   SEM_MUTEX = 0,
   SEM_DATA  = 1,
   SEM_NUM   = 2
};

union semun {
   int              val;
   struct semid_ds *buf;
   unsigned short  *array;
};

struct data_payload {
   unsigned long msg_id;
   char          time_str[TIME_LEN];
   int           writer_pid;
   char          message[MSG_LEN];
};

#endif // DATA_HEAD_H