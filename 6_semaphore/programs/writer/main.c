#define _GNU_SOURCE
#include "data_head.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>

static int                  global_semid = -1;
static int                  global_shmid = -1;
static struct data_payload *shm_data = NULL;

static void cleanup_and_exit(int code) {
   if (shm_data) {
      struct sembuf op = { .sem_num = SEM_MUTEX, .sem_op = -1 };
      if (semop(global_semid, &op, 1) == 0) {
         shm_data->writer_pid = 0;
         op.sem_op = 1;
         semop(global_semid, &op, 1);
      }
      shmdt(shm_data);
   }
   _exit(code);
}

static void on_signal(int sig) {
   (void)sig;
   char msg[] = "[WRITER] on_signal\n";
   write(STDOUT_FILENO, msg, sizeof(msg));
   cleanup_and_exit(EXIT_SUCCESS);
}

static int is_alive(int pid) {
   if (pid <= 0) return 0;
   if (kill(pid, 0) == 0) return 1;
   return errno == EPERM;
}

int main(void) {
   if (access(SHM_KEY_PATH, F_OK) == -1) {
      int fd = open(SHM_KEY_PATH, O_CREAT | O_WRONLY, 0666);
      if (fd != -1) close(fd);
   }
   key_t key = ftok(SHM_KEY_PATH, SHM_KEY_ID);
   if (key == -1) {
      perror("ftok");
      return EXIT_FAILURE;
   }
   int new_set = 0;
   global_semid = semget(key, SEM_NUM, IPC_CREAT | IPC_EXCL | 0666);
   if (global_semid != -1) {
      new_set = 1;
   } else if (errno == EEXIST) {
      global_semid = semget(key, SEM_NUM, 0666);
      if (global_semid == -1) {
         perror("semget");
         return EXIT_FAILURE;
      }
   } else {
      perror("semget");
      return EXIT_FAILURE;
   }

   if (new_set) {
      union semun arg = {0};
      unsigned short vals[SEM_NUM] = { 1, 0 };
      arg.array = vals;
      if (semctl(global_semid, 0, SETALL, arg) == -1) {
         perror("semctl SETALL");
         return EXIT_FAILURE;
      }
   }

   global_shmid = shmget(key, sizeof(struct data_payload), IPC_CREAT | 0666);
   if (global_shmid == -1) {
      perror("shmget");
      return EXIT_FAILURE;
   }

   shm_data = shmat(global_shmid, NULL, 0);
   if (shm_data == (void*)-1) {
      perror("shmat");
      return EXIT_FAILURE;
   }

   struct sembuf lock   = { .sem_num = SEM_MUTEX, .sem_op = -1 };
   struct sembuf unlock = { .sem_num = SEM_MUTEX, .sem_op = +1 };
   struct sembuf post   = { .sem_num = SEM_DATA,  .sem_op = +1 };

   if (semop(global_semid, &lock, 1) == -1) {
      perror("semop lock");
      return EXIT_FAILURE;
   }

   int prev = shm_data->writer_pid;
   if (is_alive(prev)) {
      char buf[128] = {0};
      int len = snprintf(buf, sizeof(buf), "[WRITER] already launch: PID=%d\n", prev);
      write(STDOUT_FILENO, buf, (size_t)len);
      semop(global_semid, &unlock, 1);
      cleanup_and_exit(EXIT_FAILURE);
   }

   shm_data->writer_pid = (int)getpid();
   shm_data->msg_id = 0;
   memset(shm_data->time_str, 0, sizeof(shm_data->time_str));
   memset(shm_data->message,  0, sizeof(shm_data->message));
   semop(global_semid, &unlock, 1);

   if (signal(SIGINT,  on_signal) == SIG_ERR) perror("signal SIGINT");
   if (signal(SIGTERM, on_signal) == SIG_ERR) perror("signal SIGTERM");

   char hello[64] = {0};
   int len = snprintf(hello, sizeof(hello), "[WRITER] launch PID=%d\n", (int)getpid());
   write(STDOUT_FILENO, hello, (size_t)len);

   for (;;) {
      char time_buf[TIME_LEN] = {0};
      ctime_better(time_buf, sizeof(time_buf)/sizeof(*time_buf), time(NULL));

      char tmp[MSG_LEN] = {0};
      snprintf(tmp, sizeof(tmp),
               "time=%s; PID=%d",
               time_buf, (int)getpid());

      if (semop(global_semid, &lock, 1) == -1) {
         if (errno == EINTR) continue;
         perror("semop lock (loop)");
         break;
      }
      shm_data->writer_pid = (int)getpid();
      shm_data->msg_id++;
      memcpy(shm_data->time_str, time_buf, sizeof(shm_data->time_str));
      memcpy(shm_data->message, tmp, sizeof(shm_data->message));
      semop(global_semid, &unlock, 1);

      semop(global_semid, &post, 1);

      sleep(4);
   }

   cleanup_and_exit(EXIT_SUCCESS);
}