#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

static char *ctime_better(char *buf, size_t bufsz, time_t t) {
   struct tm tm_now = {0};
   localtime_r(&t, &tm_now);
   strftime(buf, bufsz, "%Y-%m-%d %H:%M:%S", &tm_now);
   return buf;
}

int main(void) {
   int fds[2];

   if (pipe(fds) == -1) {
      perror("pipe");
      return EXIT_FAILURE;
   }

   pid_t pid = fork();
   if (pid == -1) {
      perror("fork");
      return EXIT_FAILURE;
   }

   if (pid == 0) { // child
      close(fds[1]);

      char buf[256] = {0};
      ssize_t n = read(fds[0], buf, sizeof(buf)/sizeof(*buf) - 1);
      if (n == -1) {
         perror("read");
         close(fds[0]);
         exit(EXIT_FAILURE);
      }
      buf[n] = 0;
      close(fds[0]);

      char child_time[64] = {0};
      ctime_better(child_time, sizeof(child_time)/sizeof(*buf), time(NULL));

      char out[512] = {0};
      int len = snprintf(out, sizeof(out)/sizeof(*out),
                        "[CHILD]               time=%s; PID=%d; PPID=%d\n"
                        "[CHILD] pipe message: %s\n",
                        child_time, getpid(), getppid(), buf);
      if (len > 0)
         write(STDOUT_FILENO, out, (size_t)len);

      exit(EXIT_SUCCESS);
   } else { // parent
      close(fds[0]);

      char parent_time[64] = {0};
      ctime_better(parent_time, sizeof(parent_time)/sizeof(*parent_time), time(NULL));

      char msg[256] = {0};
      int len = snprintf(msg, sizeof(msg)/sizeof(*msg), "time=%s; PID=%d", parent_time, getpid());

      sleep(8);

      ssize_t total = 0;
      while (total < len) {
         ssize_t w = write(fds[1], msg + total, (size_t)(len - total));
         if (w == -1) {
            if (errno == EINTR) continue;
            perror("write");
            close(fds[1]);
            return EXIT_FAILURE;
         }
         total += w;
      }
      close(fds[1]);

      int status = 0;
      if (waitpid(pid, &status, 0) == -1) {
         perror("waitpid");
         return EXIT_FAILURE;
      }
      if (WIFEXITED(status))
         printf("[PARENT] child exit with code=%d\n", WEXITSTATUS(status));
   }

   return EXIT_SUCCESS;
}