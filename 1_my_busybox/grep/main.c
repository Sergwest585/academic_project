#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

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

long grep_stream(FILE *fp, const char *pattern, struct Opts opts) {
   char *line = NULL;
   size_t cap = 0;
   ssize_t len;
   long line_no = 0;
   long matches = 0;

   while ((len = getline(&line, &cap, fp)) != -1) {
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
      total_matches = grep_stream(stdin, pattern, opts);
   } else {
      for (int i = optind; i < argc; ++i) {
         FILE *fp = fopen(argv[i], "r");
         if (not fp) {
               fprintf(stderr, "mygrep: %s: %s\n", argv[i], strerror(errno));
               return 2;
         }
         total_matches += grep_stream(fp, pattern, opts);
         fclose(fp);
      }
   }

   return total_matches > 0 ? 0 : 1;
}