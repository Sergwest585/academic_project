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

#define and &&
#define or  ||
#define not !

char const* help_message = "                                              \n"
"mygrep - grep analog                                                     \n"
"                                                                         \n"
"USAGE:                                                                   \n"
"   mygrep [FLAGS...] pattern [files...]                                  \n"
"                                                                         \n"
"FLAGS:                                                                   \n"
"   -h --help          print help message                                 \n"
"      --verbose       enable debug logs                                  \n"
"   -v --version       print version                                      \n"
"   -i --ignore-case   case insensitive search                            \n"
"   -v --invert-match  select non-matching lines                          \n"
"   -n --line-number   show line numbers                                  \n"
"";

char const *version = "0.0.1";

enum { // get_opt cmd
   OPT_NONASCII = 127,
   OPT_VERBOSE
};

struct Opts {
   bool ignore_case;
   bool invert;
   bool line_number;
};

bool line_matches(const char *line, const char *pattern, bool ignore_case) {
   if (not ignore_case) {
      return strstr(line, pattern) != NULL;
   }

   size_t plen = strlen(pattern);
   if (plen == 0) return true;

   for (const char *p = line; *p; ++p) {
      size_t i = 0;
      // while (i < plen && p[i] &&
      //       tolower((unsigned char)p[i]) == tolower((unsigned char)pattern[i])) {
      //    ++i;
      // }
      if (i == plen) return true;
   }
   return false;
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

long grep_stream(int fd, const char *pattern, struct Opts opts) {
   char *line = NULL;
   size_t cap = 0;
   ssize_t len;
   long line_no = 0;
   long matches = 0;

   while ((len = _getline(&line, &cap, fd)) != -1) {
      ++line_no;

      bool had_nl = (len > 0 and line[len-1] == '\n');
      if (had_nl) line[len-1] = '\0';

      bool m = line_matches(line, pattern, opts.ignore_case);
      if (opts.invert) m = not m;

      if (m) {
         ++matches;
         if (opts.line_number) {
            printf("%ld:", line_no);
         }
         fputs(line, stdout);
         if (had_nl) putchar('\n');
      }
   }

   free(line);
   return matches;
}

int main(int argc, char** argv) {
   int opt;
   struct Opts opts = {0};

   struct option options[] = {
      {"help",         no_argument, 0, 'h'},
      {"ignore-case",  no_argument, 0, 'i'},
      {"invert-match", no_argument, 0, 'v'},
      {"line-number",  no_argument, 0, 'n'},
      {0,0,0,0}
   };

   while ((opt = getopt_long(argc, argv, "hivn", options, NULL)) != -1) {
      switch (opt) {
         case 'h':
            printf("%s", help_message);
            return 0;
         case 'i':
            opts.ignore_case = true;
            break;
         case 'v':
            opts.invert = true;
            break;
         case 'n':
            opts.line_number = true;
            break;
         default:
            fprintf(stderr, "%s", help_message);
            return 1;
      }
   }

   if (optind >= argc) {
      fprintf(stderr, "mygrep: missing pattern\n%s", help_message);
      return 2;
   }

   const char *pattern = argv[optind++];
   long total_matches = 0;

   if (optind >= argc) {
      total_matches = grep_stream(STDIN_FILENO, pattern, opts);
   } else {
      for (int i = optind; i < argc; ++i) {
         int fd = open(argv[i], O_RDONLY);
         if (not fd) {
            fprintf(stderr, "mygrep: %s: %s\n", argv[i], strerror(errno));
            return 2;
         }
         total_matches += grep_stream(fd, pattern, opts);
         close(fd);
      }
   }

   return total_matches > 0;
}