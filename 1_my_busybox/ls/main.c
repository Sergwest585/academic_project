#include <stdio.h>    /* printf fprintf */
#include <stdlib.h>   /* */ 
#include <sys/stat.h> /* chmod */ 
#include <getopt.h>   /* getopt_long */ 
#include <stdbool.h> 
#include <assert.h> 
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <math.h>
#include <string.h>
#define and &&
#define or  ||
#define not !

char const* help_message = "                                              \n"
"myls - ls analog                                                         \n"
"                                                                         \n"
"USAGE:                                                                   \n"
"   ls [FLAGS...] [{files | directories}...]                              \n"
"                                                                         \n"
"FLAGS:                                                                   \n"
"   -h --help        print help message                                   \n"
"      --verbose     enable debug logs                                    \n"
"   -v --version     print version                                        \n"
"   -l               use a long listing format                            \n"
"   -a --all         do not ignore entries starting with dot              \n"
"";

char const* version = "0.0.1";

enum { // get_opt cmd
   OPT_NONASCII = 127,
   OPT_VERBOSE
};

int format_size(size_t bytes, char *buf, size_t buf_len) {
   char const* suffixes = "BKMGTPEZY";
   int s = 0;
   
   while (bytes >= 1024*4 && s < 8) {
      bytes /= 1024;
      s++;
   }

   if (s == 0) {
      return snprintf(buf, buf_len, "%lu%c", bytes, suffixes[s]);
   } else {
      return snprintf(buf, buf_len, "%lu%c", bytes, suffixes[s]);
   }
}

int max(int a, int b) {return a>=b?a:b;}

struct linux_dirent64 {
   unsigned long long d_ino;    // Inode number
   long long          d_off;    // Offset to next structure
   unsigned short     d_reclen; // Length of this record
   unsigned char      d_type;   // Type of file
   char               d_name[]; // File name (null-terminated)
};

struct Opts {
   bool verbose;
   bool long_list;
   bool all;
};

struct File_info {
   unsigned char     d_type;
   struct stat       st;
   char              perms[11];
   long int          hard_link_count;
   char*             owner;
   char*             group;
   char              size[8];
   char              timebuf[20];
   char*             name;
};

int sort_by_name(const void *a, const void *b) {
   struct File_info arg1 = *(const struct File_info*)a;
   struct File_info arg2 = *(const struct File_info*)b;
   return strcmp(arg1.name, arg2.name);
}

void list_dir(char const* filename, struct Opts opts) {
   int fd;
   long nread;
   char buf[1024];
   struct linux_dirent64* d;

   fd = open(filename, O_RDONLY);
   if (fd == -1) {
      perror("open");
      exit(2);
   }

   while (1) {
      nread = syscall(SYS_getdents64, fd, buf, sizeof(buf)/sizeof(*buf));
      if (nread == -1) {
         perror("getdents64");
         exit(2);
      }

      if (nread == 0) {
         break;
      }

      size_t farr_size = 0, farr_cap = 16;
      int max_hc = 0, max_owner = 0, max_group = 0, max_sz = 0;
      struct File_info* farr = malloc(farr_cap * sizeof(struct File_info));
      if (not farr) {
         perror("malloc");
         exit(2);
      }

      for (long bpos = 0; bpos < nread; bpos += d->d_reclen) {
         d = (struct linux_dirent64 *) (buf + bpos);
         struct stat st;
         if (not opts.all && d->d_name[0] == '.')
            continue;
         if (fstatat(fd, d->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
            perror("stat2");
            continue;
         }

         struct File_info finfo;
         finfo.d_type = d->d_type;
         finfo.st = st;

         finfo.perms[0] = S_ISDIR (st.st_mode) ? 'd' :
                          S_ISLNK (st.st_mode) ? 'l' :
                          S_ISCHR (st.st_mode) ? 'c' :
                          S_ISBLK (st.st_mode) ? 'b' :
                          S_ISFIFO(st.st_mode) ? 'p' :
                          S_ISSOCK(st.st_mode) ? 's' : '-';

         finfo.perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
         finfo.perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
         finfo.perms[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
         finfo.perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
         finfo.perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
         finfo.perms[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
         finfo.perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
         finfo.perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
         finfo.perms[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
         finfo.perms[10] = '\0'; // I hope there any better methods for this

         finfo.hard_link_count = st.st_nlink;
         max_hc = max(max_hc, log10(finfo.hard_link_count));

         struct passwd *pw = getpwuid(st.st_uid);
         finfo.owner = strdup(pw ? pw->pw_name : "unknown");
         size_t owner_length = strlen(finfo.owner);
         max_owner = max(max_owner, owner_length);

         struct group *gr = getgrgid(st.st_gid);
         finfo.group = strdup(gr ? gr->gr_name : "unknown");
         size_t group_length = strlen(finfo.group);
         max_group = max(max_group, group_length);

         max_sz = max(max_sz, format_size(st.st_size, finfo.size, sizeof(finfo.size)));
         
         struct tm *tm_info = localtime(&st.st_mtime);
         strftime(finfo.timebuf, sizeof(finfo.timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

         finfo.name = strdup(d->d_name);


         farr[farr_size] = finfo;
         farr_size++;
         if (farr_size == farr_cap) {
            farr_cap <<= 1;
            farr = realloc(farr, farr_cap * sizeof(struct File_info));
            if (not farr) {
               perror("realloc");
               exit(2);
            }
         }
      }

      qsort(farr, farr_size, sizeof(struct File_info), sort_by_name);

      for (size_t i = 0; i < farr_size; ++i) {
         if (opts.long_list) {
            printf("%s %*ld %*s %*s %*s %s ",
               farr[i].perms,
               max_hc, farr[i].hard_link_count,
               max_owner, farr[i].owner,
               max_group, farr[i].group,
               max_sz, farr[i].size,
               farr[i].timebuf
            );
         }
         if (farr[i].d_type == 4) {
            printf("\033[1;34m");
         } else if (farr[i].d_type == 8) {
            if (farr[i].st.st_mode & S_IXUSR) {
               printf("\033[1;32m");
            } else if (S_ISLNK(farr[i].st.st_mode)) {
               printf("\033[1;36m");
            }
         }
         printf("%s\033[0m\n", farr[i].name);
         free(farr[i].name);
         free(farr[i].owner);
         free(farr[i].group);
      }
      free(farr);
   }

   close(fd);
}

void list_file(char const* filename, struct Opts opts) {
   struct stat st;

   if (stat(filename, &st) != 0) {
      perror("stat");
      return;
   }

   if (S_ISDIR(st.st_mode)) {
      list_dir(filename, opts);
   }
}

int main(int argc, char** argv) {
   int opt;
   struct Opts opts = {0};
   
   struct option options[] = {
      {"help",    no_argument, 0, 'h'},
      {"verbose", no_argument, 0, OPT_VERBOSE},
      {"version", no_argument, 0, 'v'},
      {"all",     no_argument, 0, 'a'},
      {0,0,0,0}
   };

   while((opt = getopt_long(argc, argv, "hval", options, NULL)) != -1) {
      switch (opt) {
         case 'v':
            printf("version: %s\n", version);
            return 0;
         case 'h':
            printf("%s", help_message);
            return 0;
         case 'l':
            opts.long_list = true;
            break;
         case 'a':
            opts.all = true;
            break;
         case OPT_VERBOSE:
            opts.verbose = true;
            break;
         default:
            fprintf(stderr, "%s", help_message);
            return 1;
      }
   }

   if (optind >= argc) {
      list_file(".", opts);
   } else if (optind + 1 == argc) {
      list_file(argv[optind], opts);
   } else {
      for (int i = optind; i < argc; ++i) {
         printf("%s:\n", argv[i]);
         list_file(argv[i], opts);
         printf("\n");
      }
   }
   
   if (opts.verbose) {
      printf("ls succses\n");
   }   

   return 0;
}
