#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>

#define and &&
#define not !

char const* help_message = "                                              \n"
"mycat - cat analog                                                       \n"
"                                                                         \n"
"USAGE:                                                                   \n"
"   mycat [FLAGS...] [files...]                                           \n"
"                                                                         \n"
"FLAGS:                                                                   \n"
"   -h --help             print help message                                   \n"
"      --verbose          enable debug logs                                    \n"
"   -v --version          print version                                        \n"
"   -n --number           number all output lines                              \n"
"   -b --number-nonblank  number nonempty output lines (overrides -n)     \n"
"   -E --show-ends        display $ at end of each line                        \n"
"";

char const* version = "0.0.1";

struct Opts {
   bool number;
   bool number_nonblank;
   bool show_ends;
};


bool check_direct_io(int fd) {
   int flags = fcntl(fd, F_GETFL);
   if (flags == -1) {
      perror("fcntl");
      return false;
   }
   return flags & O_DIRECT;
}
bool check_fs_type(int fd) {
   struct statfs fs_info;
   if (fstatfs(fd, &fs_info) == -1) {
      perror("fstatfs");
      return false;
   }

   // procfs 0x9fa0; sysfs 0x62656572
   return ((fs_info.f_type == 0x9fa0) || (fs_info.f_type == 0x62656572));
}



size_t splice_fds(int fd_in, int fd_out, size_t len) {
   int pipefd[2];
   
   if (pipe(pipefd) == -1) {
      return -1; 
   }

   size_t total_spliced = 0;

   while (total_spliced < len) {
      size_t to_transfer = len - total_spliced;

      size_t spliced_in = splice(fd_in, NULL, pipefd[1], NULL, to_transfer, SPLICE_F_MOVE | SPLICE_F_MORE);
      // if (spliced_in < 0) {
      //    if (errno == EINTR) continue;
      //    perror("splice to");
      //    close(pipefd[0]);
      //    close(pipefd[1]);
      //    return -1;
      // }
      if (spliced_in == 0) break;

      size_t pipe_backed = spliced_in;
      
      while (pipe_backed > 0) {
         size_t spliced_out = splice(pipefd[0], NULL, fd_out, NULL, pipe_backed, SPLICE_F_MOVE | SPLICE_F_MORE);
         // if (spliced_out < 0) {
         //    if (errno == EINTR) continue;
         //    perror("splice from");
         //    close(pipefd[0]);
         //    close(pipefd[1]);
         //    return -1;
         // }
         if (spliced_out == 0) {
            perror("splice from");
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
         }
         pipe_backed -= spliced_out;
         total_spliced += spliced_out;
      }
   }

   close(pipefd[0]);
   close(pipefd[1]);
   return total_spliced;
}

size_t _getline(char **lineptr, size_t *n, int fd) {
   if (lineptr == NULL || n == NULL) {
      errno = EINVAL;
      return -1;
   }

   if (*lineptr == NULL || *n == 0) {
      *n = 128;
      *lineptr = malloc(*n);
      if (*lineptr == NULL) {
         return -1;
      }
   }

   size_t pos = 0;
   while (1) {
      char c;
      ssize_t bytes_read = read(fd, &c, 1);
      if (bytes_read < 0) {
         if (errno == EINTR) continue;
         return -1;
      } else if (bytes_read == 0) {
         if (pos == 0) return -1;
         break;
      }

      if (pos + 1 >= *n) {
         *n *= 2;
         char *new_ptr = realloc(*lineptr, *n);
         if (new_ptr == NULL) {
            return -1;
         }
         *lineptr = new_ptr;
      }

      (*lineptr)[pos++] = c;

      if (c == '\n') break;
   }

   (*lineptr)[pos] = '\0';
   return pos;
}

void write_fds(int fd, struct Opts opts) {
   char *line = NULL;
   size_t len;
   size_t cap = 0;
   uint line_no = 0;

   while ((len = _getline(&line, &cap, fd)) != (size_t)-1) {
      bool is_empty = (len == 0) || (len == 1 and line[0] == '\n');
      bool need_number = opts.number || (opts.number_nonblank and not is_empty);

      if (need_number) {
         printf("%6u\t", ++line_no);
      }

      if (opts.show_ends) {
         for (size_t i = 0; i < len; ++i) {
            if (line[i] == '\n') {
               putchar('$');
               putchar('\n');
            } else {
               putchar(line[i]);
            }
         }
      } else {
         write(STDERR_FILENO, line, len);
      }
   }

   free(line);
}

int main(int argc, char** argv) {
   int opt;
   struct Opts opts = {0};

   struct option options[] = {
      {"help",              no_argument, 0, 'h'},
      {"number",            no_argument, 0, 'n'},
      {"number-nonblank",   no_argument, 0, 'b'},
      {"show-ends",         no_argument, 0, 'E'},
      {0,0,0,0}
   };

   while ((opt = getopt_long(argc, argv, "hnbE", options, NULL)) != -1) {
      switch (opt) {
         case 'h':
               printf("%s", help_message);
               return 0;
         case 'n':
               opts.number = true;
               break;
         case 'b':
               opts.number_nonblank = true;
               break;
         case 'E':
               opts.show_ends = true;
               break;
         default:
               fprintf(stderr, "%s", help_message);
               return 1;
      }
   }

   if (opts.number_nonblank) {
      opts.number = false;
   }

   if (optind >= argc) {
      if (not opts.number and not opts.number_nonblank and not opts.show_ends) splice_fds(STDIN_FILENO, STDOUT_FILENO, SIZE_MAX);
      else write_fds(STDIN_FILENO, opts);
   } else {
      for (int i = optind; i < argc; ++i) {
         int fd = open(argv[i], O_RDONLY);
         if (not fd) {
            perror("mycat open");
            return 2;
         }
         if (not opts.number and not opts.number_nonblank and not opts.show_ends) {
            splice_fds(STDIN_FILENO, STDOUT_FILENO, SIZE_MAX);
         } else {
            write_fds(fd, opts);
         }
         close(fd);
      }
   }

   return 0;
}