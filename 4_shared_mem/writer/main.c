#define _POSIX_C_SOURCE 200809L
#include "data_head.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

static struct data_payload *shm_data = NULL;
static        sem_t        *lock_sem = NULL;
static        sem_t        *data_sem = NULL;

static void cleanup_and_exit(int code) {
   if (shm_data) {
      sem_wait(lock_sem);
      shm_data->writer_pid = 0;
      sem_post(lock_sem);
   }
   if (data_sem) sem_close(data_sem);
   if (lock_sem) sem_close(lock_sem);
   if (shm_data) munmap(shm_data, sizeof(*shm_data));
   _exit(code);
}

static void on_signal(int sig) {
   (void)sig;
   char msg[] = "[WRITER] exit signal\n";
   write(STDOUT_FILENO, msg, sizeof(msg) - 1);
   cleanup_and_exit(EXIT_SUCCESS);
}

static int is_alive(int pid) {
   if (pid <= 0) return 0;
   if (kill(pid, 0) == 0) return 1;
   return errno == EPERM;
}

int main(void) {
   lock_sem = sem_open(SEM_LOCK_NAME, O_CREAT, 0666, 1);
   if (lock_sem == SEM_FAILED) {
      perror("sem_open lock");
      return EXIT_FAILURE;
   }
   data_sem = sem_open(SEM_DATA_NAME, O_CREAT, 0666, 0);
   if (data_sem == SEM_FAILED) {
      perror("sem_open data");
      sem_close(lock_sem);
      return EXIT_FAILURE;
   }

   int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
   if (fd == -1) {
      perror("shm_open");
      return EXIT_FAILURE;
   }
   if (ftruncate(fd, (off_t)sizeof(struct data_payload)) == -1) {
      perror("ftruncate");
      close(fd);
      return EXIT_FAILURE;
   }
   void *p = mmap(NULL, sizeof(struct data_payload), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (p == MAP_FAILED) {
      perror("mmap");
      close(fd);
      return EXIT_FAILURE;
   }
   close(fd);
   shm_data = p;

   sem_wait(lock_sem);

   int prev = shm_data->writer_pid;
   if (is_alive(prev)) {
      char buf[128] = {0};
      int len = snprintf(buf, sizeof(buf),"[WRITER] already running: PID=%d\n", prev);
      write(STDOUT_FILENO, buf, (size_t)len);
      sem_post(lock_sem);
      cleanup_and_exit(EXIT_FAILURE);
   }

   shm_data->writer_pid = (int)getpid();
   shm_data->msg_id = 0;
   memset(shm_data->time_str, 0, sizeof(shm_data->time_str));
   memset(shm_data->message,  0, sizeof(shm_data->message));

   sem_post(lock_sem);

   if (signal(SIGINT,  on_signal) == SIG_ERR) perror("signal SIGINT");
   if (signal(SIGTERM, on_signal) == SIG_ERR) perror("signal SIGTERM");

   char hello[128] = {0};
   int len = snprintf(hello, sizeof(hello), "[WRITER] launched PID=%d\n", getpid());
   write(STDOUT_FILENO, hello, (size_t)len);

   for (;;) {
      char time_buf[TIME_LEN] = {0};
      ctime_better(time_buf, sizeof(time_buf)/sizeof(*time_buf), time(NULL));

      char msg_buf[MSG_LEN] = {0};
      snprintf(msg_buf, sizeof(msg_buf)/sizeof(*msg_buf),
                        "time=%s; PID=%d",
                        time_buf, (int)getpid());

      sem_wait(lock_sem);
      shm_data->writer_pid = (int)getpid();
      shm_data->msg_id++;
      memcpy(shm_data->time_str, time_buf, sizeof(shm_data->time_str));
      memcpy(shm_data->message,  msg_buf,  sizeof(shm_data->message));
      sem_post(lock_sem);

      sem_post(data_sem);

      sleep(1);
   }

   cleanup_and_exit(EXIT_SUCCESS);
}