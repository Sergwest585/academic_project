#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

static const char FIFO_PATH[] = "./fifo_file";

static char *ctime_better(char *buf, size_t bufsz, time_t t) {
   struct tm tm_now = {0};
   localtime_r(&t, &tm_now);
   strftime(buf, bufsz, "%Y-%m-%d %H:%M:%S", &tm_now);
   return buf;
}

static int run_writer(void) {
   char parent_time[64] = {0};
   ctime_better(parent_time, sizeof(parent_time)/sizeof(*parent_time), time(NULL));

   char msg[256] = {0};
   int len = snprintf(msg, sizeof(msg)/sizeof(*msg), "time=%s; PID=%d", parent_time, getpid());

   if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
      perror("mkfifo");
      return EXIT_FAILURE;
   }
   
   int fd = open(FIFO_PATH, O_WRONLY);
   if (fd == -1) {
      perror("open writer");
      return EXIT_FAILURE;
   }

   sleep(8);

   ssize_t total = 0;
   while (total < len) {
      ssize_t w = write(fd, msg + total, (size_t)(len - total));
      if (w == -1) {
         if (errno == EINTR) continue;
         perror("write FIFO");
         close(fd);
         return EXIT_FAILURE;
      }
      total += w;
   }

   close(fd);
   printf("[WRITER] write to FIFO: %s\n", msg);
   return EXIT_SUCCESS;
}

static int run_reader(void) {
   int fd = open(FIFO_PATH, O_RDONLY);
   if (fd == -1) {
      perror("open reader");
      return EXIT_FAILURE;
   }

   char buf[256] = {0};
   ssize_t n = read(fd, buf, sizeof(buf) - 1);
   if (n == -1) {
      perror("read");
      close(fd);
      return EXIT_FAILURE;
   }
   buf[n] = '\0';
   close(fd);

   char child_time[64] = {0};
   ctime_better(child_time, sizeof(child_time)/sizeof(*child_time), time(NULL));

   printf("[READER]           time=%s; PID=%d\n", child_time, getpid());
   printf("[READER] FIFO msg: %s\n", buf);

   if (unlink(FIFO_PATH) == -1 && errno != ENOENT) {
      perror("unlink");
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
   if (argc != 2) {
      fprintf(stderr, "usage: %s {writer|reader}\n", argv[0]);
      return EXIT_FAILURE;
   }

   if (strcmp(argv[1], "writer") == 0) return run_writer();
   if (strcmp(argv[1], "reader") == 0) return run_reader();

   fprintf(stderr, "unknown cmd: %s\n", argv[1]);
   return EXIT_FAILURE;
}