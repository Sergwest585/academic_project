#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define N_READERS      10
#define ARR_LEN        64
#define WRITES_TOTAL   20
#define WRITER_PERIOD  300000
#define READER_PERIOD  100000

static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

static char           global_arr[ARR_LEN] = {0};
static unsigned long  global_counter = 0;
static int            global_done = 0;

int _usleep(long usec) {
   struct timespec ts;
   ts.tv_sec = usec / 1000000;
   ts.tv_nsec = (usec % 1000000) * 1000;
   return nanosleep(&ts, NULL);
}

static void *writer_thread(void *arg) {
   (void)arg;
   char start[64] = {0};
   int len = snprintf(start, sizeof(start)/sizeof(*start), "[WRITER     tid=%lu] launch\n", (unsigned long)pthread_self());
   write(STDOUT_FILENO, start, (size_t)len);

   for (unsigned long i = 0; i < WRITES_TOTAL; ++i) {
      char tmp_arr[ARR_LEN] = {0};
      int len = snprintf(tmp_arr, sizeof(tmp_arr)/sizeof(*tmp_arr), "num: %lu was sended", i);
      if (len < 0) len = 0;
      tmp_arr[ARR_LEN - 1] = '\0';

      pthread_rwlock_wrlock(&rwlock);
      memcpy(global_arr, tmp_arr, sizeof(global_arr));
      global_counter = i;
      pthread_rwlock_unlock(&rwlock);

      _usleep(WRITER_PERIOD);
   }

   pthread_rwlock_wrlock(&rwlock);
   global_done = 1;
   pthread_rwlock_unlock(&rwlock);

   char bye[64] = {0};
   len = snprintf(bye, sizeof(bye), "[WRITER     tid=%lu] end\n", (unsigned long)pthread_self());
   write(STDOUT_FILENO, bye, (size_t)len);

   return NULL;
}

static void *reader_thread(void *arg) {
   unsigned long id = (unsigned long)arg;
   unsigned long last_seen = 0;

   char start[96] = {0};
   int len = snprintf(start, sizeof(start)/sizeof(*start), "[READER #%-2lu tid=%lu] started\n", id, (unsigned long)pthread_self());
   write(STDOUT_FILENO, start, (size_t)len);

   for (;;) {
      pthread_rwlock_rdlock(&rwlock);

      if (global_counter <= last_seen && global_done) {
         pthread_rwlock_unlock(&rwlock);
         break;
      }

      last_seen = global_counter;

      char local_arr[ARR_LEN] = {0};
      memcpy(local_arr, global_arr, sizeof(local_arr)/sizeof(*local_arr));
      unsigned long counter = global_counter;

      pthread_rwlock_unlock(&rwlock);

      char out[256] = {0};
      int len = snprintf(out, sizeof(out)/sizeof(*out), "[READER #%-2lu tid=%lu] counter=%lu; arr=\"%.*s\"\n",
                         id, (unsigned long)pthread_self(),
                         counter, ARR_LEN, local_arr);
      write(STDOUT_FILENO, out, (size_t)len);

      _usleep(READER_PERIOD);
   }

   char bye[96] = {0};
   len = snprintf(bye, sizeof(bye)/sizeof(*bye), "[READER #%-2lu tid=%lu] exit\n", id, (unsigned long)pthread_self());
   write(STDOUT_FILENO, bye, (size_t)len);

   return NULL;
}

int main(void) {
   pthread_t readers[N_READERS] = {0};
   pthread_t writer = {0};

   if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
      perror("pthread_create writer");
      return EXIT_FAILURE;
   }

   for (unsigned long i = 0; i < N_READERS; ++i) {
      if (pthread_create(&readers[i], NULL, reader_thread, (void *)(i + 1)) != 0) {
         char msg[64] = {0};
         int len = snprintf(msg, sizeof(msg)/sizeof(*msg), "pthread_create reader #%lu", i + 1);
         msg[len] = '\0';
         perror(msg);
         return EXIT_FAILURE;
      }
   }

   pthread_join(writer, NULL);
   for (int i = 0; i < N_READERS; ++i) pthread_join(readers[i], NULL);

   return EXIT_SUCCESS;
}