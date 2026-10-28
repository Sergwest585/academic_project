#define _POSIX_C_SOURCE 200809L
#include "data_head.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

int main(void) {
   int fd = shm_open(SHM_NAME, O_RDWR, 0);
   if (fd == -1) {
      char msg[] = "[READER] writer not found (no shm)\n";
      write(STDERR_FILENO, msg, sizeof(msg)/sizeof(*msg));
      return EXIT_FAILURE;
   }

   struct data_payload *data = mmap(NULL, sizeof(struct data_payload), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

   if (data == MAP_FAILED) {
      perror("mmap");
      close(fd);
      return EXIT_FAILURE;
   }
   close(fd);

   sem_t *lock = sem_open(SEM_LOCK_NAME, 0);
   if (lock == SEM_FAILED) {
      perror("sem_open lock");
      return EXIT_FAILURE;
   }
   sem_t *data_sem = sem_open(SEM_DATA_NAME, 0);
   if (data_sem == SEM_FAILED) {
      perror("sem_open data");
      return EXIT_FAILURE;
   }

   char start[128] = {0};
   int len = snprintf(start, sizeof(start), "[READER] launched PID=%d\n", (int)getpid());
   write(STDOUT_FILENO, start, (size_t)len);

   unsigned long last_msg_id = 0;

   for (;;) {
      struct timespec ts = {0};
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += 2;

      if (sem_timedwait(data_sem, &ts) == -1 && errno == ETIMEDOUT) {
         sem_wait(lock);
         int wp = data->writer_pid;
         sem_post(lock);
         if (wp == 0) {
            char bye[] = "[READER] writer is dead\n";
            write(STDOUT_FILENO, bye, sizeof(bye)/sizeof(*bye));
            break;
         }
         continue;
      }

      sem_wait(lock);
      if (data->msg_id == last_msg_id) {
         sem_post(lock);
         continue;
      }
      last_msg_id = data->msg_id;
      char incoming[MSG_LEN] = {0};
      memcpy(incoming, data->message, sizeof(incoming));
      sem_post(lock);

      char reader_time[TIME_LEN] = {0};
      ctime_better(reader_time, sizeof(reader_time)/sizeof(*reader_time), time(NULL));

      char out[512] = {0};
      int len = snprintf(out, sizeof(out),
                        "[READER PID=%d]                  time=%s\n"
                        "[READER PID=%d] msg from shared: %s\n",
                        (int)getpid(), reader_time,
                        (int)getpid(), incoming);
      write(STDOUT_FILENO, out, (size_t)len);
   }

   sem_close(data_sem);
   sem_close(lock);
   munmap(data, sizeof(*data));
   return EXIT_SUCCESS;
}