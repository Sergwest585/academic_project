#define _GNU_SOURCE
#include "data_head.h"
#include "utils.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

static int is_alive(int pid) {
   if (pid <= 0) return 0;
   if (kill(pid, 0) == 0) return 1;
   return errno == EPERM;
}

int main(void) {
   key_t key = ftok(SHM_KEY_PATH, SHM_KEY_ID);
   if (key == -1) {
      char msg[] = "[READER] key not found\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      return EXIT_FAILURE;
   }

   int semid = semget(key, SEM_NUM, 0);
   if (semid == -1) {
      char msg[] = "[READER] semgetnot found\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      return EXIT_FAILURE;
   }
   int shmid = shmget(key, sizeof(struct data_payload), 0);
   if (shmid == -1) {
      char msg[] = "[READER] shmget not found\n";
      write(STDERR_FILENO, msg, sizeof(msg) - 1);
      return EXIT_FAILURE;
   }

   struct data_payload *shm = shmat(shmid, NULL, 0);
   if (shm == (void*)-1) {
      perror("shmat");
      return EXIT_FAILURE;
   }

   struct sembuf lock        = { .sem_num = SEM_MUTEX, .sem_op = -1 };
   struct sembuf unlock      = { .sem_num = SEM_MUTEX, .sem_op = +1 };
   struct sembuf wait_data   = { .sem_num = SEM_DATA,  .sem_op = -1 };

   char start[64] = {0};
   int len = snprintf(start, sizeof(start), "[READER PID=%-2d] launched\n", (int)getpid());
   write(STDOUT_FILENO, start, (size_t)len);

   unsigned long last_msg_id = 0;
   unsigned long last_pid = 0;

   for (;;) {
      struct timespec ts = {
         .tv_sec = 5,
      };
      if (semtimedop(semid, &wait_data, 1, &ts) == -1) {
         if (errno == EAGAIN) {
            if (!is_alive(last_pid)) {
               char bye[] = "[READER] writer exit\n";
               write(STDOUT_FILENO, bye, sizeof(bye));
               break;
            } else continue;
         } else if (errno == EINTR) {
            continue;
         } else {
            perror("semop wait data");
            break;
         }
      }

      if (semop(semid, &lock, 1) == -1) {
         if (errno == EINTR) continue;
         perror("semop lock");
         break;
      }

      last_pid = shm->writer_pid;
      unsigned long msg_id = shm->msg_id;
      int writer_alive = is_alive(shm->writer_pid);
      char incoming[MSG_LEN] = {0};
      memcpy(incoming, shm->message, sizeof(incoming));
      semop(semid, &unlock, 1);

      if (!writer_alive && msg_id == last_msg_id) {
         char bye[] = "[READER] writer exit\n";
         write(STDOUT_FILENO, bye, sizeof(bye));
         break;
      }
      if (msg_id == last_msg_id) {
         sleep(1);
         continue;
      }
      last_msg_id = msg_id;

      char reader_time[TIME_LEN] = {0};
      ctime_better(reader_time, sizeof(reader_time)/sizeof(*reader_time), time(NULL));

      char out[512] = {0};
      int len = snprintf(out, sizeof(out),
                        "[READER PID=%-2d]      time=%s\n"
                        "[READER PID=%-2d] msg: %s\n",
                        (int)getpid(), reader_time,
                        (int)getpid(), incoming);
      write(STDOUT_FILENO, out, (size_t)len);
   }

   shmdt(shm);
   return EXIT_SUCCESS;
}