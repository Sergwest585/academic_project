#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define PRINT(MSG) do { \
   char msg[] = MSG; \
   write(STDOUT_FILENO, msg, sizeof(msg)/sizeof(*msg)); \
} while (0)
#define PRINTS(msg) do { \
   write(STDOUT_FILENO, msg, strlen(msg)); \
} while (0)

static void atexit_parent() {
   PRINT("[PARENT] atexit\n");
}
static void atexit_child() {
   PRINT("[CHILD] atexit\n");
}

static void on_sigint(int sig) {
   (void)sig;
   PRINT("SIGINT\n");
}

static void on_sigterm(int sig) {
   (void)sig;
   PRINT("SIGTERM\n");
   // char msg[64] = "SIGTERM (";
   // const char *info = strsignal(sig); // забыл...
   // if (strlen(info) < 50)
   //    strcat(msg, info);
   // strcat(msg, ")\n");
   // PRINTS(msg);
}

int main(void) {
   pid_t pid;

   if (signal(SIGINT, on_sigint) == SIG_ERR) {
      perror("signal(SIGINT)");
      exit(EXIT_FAILURE);
   }

   struct sigaction sa = {
      .sa_handler = on_sigterm
   };
   sigemptyset(&sa.sa_mask); // Posix-Паранойя

   if (sigaction(SIGTERM, &sa, NULL) == -1) {
      perror("sigaction(SIGTERM)");
      exit(EXIT_FAILURE);
   }

   pid = fork();

   if (pid == -1) {
      perror("fork");
      return EXIT_FAILURE;
   } else if (pid == 0) { // child
      atexit(atexit_child);

      printf("[CHILD] PID=%d, PPID=%d\n", getpid(), getppid());
      fflush(stdout);

      sleep(10);

      printf("[CHILD] exit with code=42\n");
      fflush(stdout);
      exit(42);
   } else { // parent
      atexit(atexit_parent);

      printf("[PARENT] PID=%d, PPID=%d, child PID=%d\n", getpid(), getppid(), pid);
      fflush(stdout);

      int status;
      pid_t w;
      do {
         w = waitpid(pid, &status, 0);
      } while (w == -1 && errno == EINTR);

      if (w == -1) {
         perror("waitpid");
         return EXIT_FAILURE;
      }

      if (WIFEXITED(status)) {
         printf("[PARENT] child (PID=%d) exit with code=%d\n", w, WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
         printf("[PARENT] child (PID=%d) was killed with sig=%d (%s)\n", w, WTERMSIG(status), strsignal(WTERMSIG(status)));
      } else {
         printf("[PARENT] child exit with status=0x%x\n", status);
      }
      fflush(stdout);

      return EXIT_SUCCESS;
   }
}