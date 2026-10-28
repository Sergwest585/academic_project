#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stddef.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>

#define N_READERS       1
#define ARR_LEN         32
#define WRITER_PERIOD_S 1

#define NAMESPACE       "/sergk_sem6_"
#define SEM_DATA_NAME   NAMESPACE"shared_data"

static char   global_arr[ARR_LEN] = {0};

static sem_t *data_sem = NULL;

static void on_signal(int sig) {
   (void)sig;
   sem_close(data_sem);
   sem_unlink(SEM_DATA_NAME);
   _exit(EXIT_SUCCESS);
}

static void *writer_thread(void *arg) {
   (void)arg;
   char start[] = "[WRITER] launched\n";
   write(STDOUT_FILENO, start, sizeof(start)/sizeof(*start));

   unsigned long i = 0;
   for (;;) {
      char msg_buf[ARR_LEN] = {0};
      snprintf(msg_buf, sizeof(msg_buf)/sizeof(*msg_buf), "%lu", ++i);

      memcpy(global_arr, msg_buf, sizeof(global_arr));
      sem_post(data_sem);

      sleep(WRITER_PERIOD_S);
      sem_wait(data_sem);
   }

   return NULL;
}

static void *reader_thread(void *arg) {
   (void)arg;
   char start[] = "[READER] launched\n";
   write(STDOUT_FILENO, start, sizeof(start)/sizeof(*start));

   for (;;) {
      char incoming[ARR_LEN] = {0};

      sem_wait(data_sem);
      memcpy(incoming, global_arr, sizeof(incoming));
      sem_post(data_sem);

      char out[512] = {0};
      int len = snprintf(out, sizeof(out),
                        "[READER] TID=%lu; msg from writer: %s\n",
                        (unsigned long)pthread_self(), incoming);
      write(STDOUT_FILENO, out, (size_t)len);

      sleep(1);
   }

   return NULL;
}

int main(void) {
   sem_unlink(SEM_DATA_NAME);
   data_sem = sem_open(SEM_DATA_NAME, O_CREAT, 0666, 0);

   if (signal(SIGINT,  on_signal) == SIG_ERR) perror("signal SIGINT");
   if (signal(SIGTERM, on_signal) == SIG_ERR) perror("signal SIGTERM");

   if (data_sem == SEM_FAILED) {
      perror("sem_open");
      sem_close(data_sem);
      sem_unlink(SEM_DATA_NAME);
      return EXIT_FAILURE;
   }

   pthread_t readers[N_READERS] = {0};
   pthread_t writer = {0};

   if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
      perror("pthread_create writer");
      sem_close(data_sem);
      sem_unlink(SEM_DATA_NAME);
      return EXIT_FAILURE;
   }

   for (unsigned long i = 0; i < N_READERS; ++i) {
      if (pthread_create(&readers[i], NULL, reader_thread, (void *)(i + 1)) != 0) {
         perror("pthread_create reader");
         sem_close(data_sem);
         sem_unlink(SEM_DATA_NAME);
         return EXIT_FAILURE;
      }
   }

   pthread_join(writer, NULL);
   for (int i = 0; i < N_READERS; ++i) pthread_join(readers[i], NULL);

   sem_close(data_sem);
   sem_unlink(SEM_DATA_NAME);

   return EXIT_SUCCESS;
}