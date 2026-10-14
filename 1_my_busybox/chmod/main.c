#include <stdio.h>    /* printf fprintf */
#include <stdlib.h>   /* */ 
#include <sys/stat.h> /* chmod */ 
#include <unistd.h>   /* */ 
#include <getopt.h>   /* getopt_long */ 
#include <stdbool.h> 
#include <assert.h> 
#define and &&
#define or  ||
#define not !

char const* help_message = "                                              \n"
"mychmod - chmod analog                                                   \n"
"                                                                         \n"
"USAGE:                                                                   \n"
"   mychmod [FLAGS...] [SELECTOR]<MODE...> <{files | directories}...>     \n"
"                                                                         \n"
"FLAGS:                                                                   \n"
"   -h --help        print help message                                   \n"
"      --verbose     enable debug logs                                    \n"
"   -v --version     print version                                        \n"
"                                                                         \n"
"SELECTOR:                                                                \n"
"   [ugoa]*          user/group/owner/all selector                        \n"
"                                                                         \n"
"MODE:                                                                    \n"
"   +/-/=r             read permision                                     \n"
"   +/-/=w             write permision                                    \n"
"   +/-/=x             execute permision                                  \n"
"                                                                         \n"
"   [0-7]{3}           read/write/execute for user,group,owner            \n"
"";

char const* version = "0.0.1";

enum { // get_opt cmd
   OPT_NONASCII = 127,
   OPT_VERBOSE
};

bool any_of(char chr, char const* const selector) {
   int pos = 0;
   while (selector[pos]) {
      if (chr == selector[pos]) return true;
      pos++;
   }
   return false;
}

int exec_new_mode(int old_mode, int new_mode, int group) {
   int selector = group*7;
   int invsel = ~selector&0777;
   return ((((-((new_mode>>10)&1))^(old_mode*((new_mode>>9)&1) | (~old_mode*((new_mode>>10)&1)) | new_mode))&selector) |
          (old_mode&invsel))
          &0777;
}

int parse_mode(char const* const arg, int* cursor, int group) {
   if (not any_of(arg[*cursor], "+-=")) {
      fprintf(stderr, "invalid mode\n");
      exit(1);
   }
   // mode = 1bit + 1bit + 3bit + 3bit + 3bit
   // and_flag + or_flag + group new modes
   int mode;
   int cmd_opt = arg[*cursor];
   int cmd     = 0; // 4r 2w 1x
   (*cursor)++;
   while(any_of(arg[*cursor], "rwx")) {
      cmd |=
         ((arg[*cursor]=='r')*4) +
         ((arg[*cursor]=='w')*2) +
         ((arg[*cursor]=='x')*1);
      (*cursor)++;
   }
   mode = ((cmd_opt=='-')<<10) + ((cmd_opt=='+')<<9) + ((group * cmd)&0777);
   return mode;
}

struct Op {
   int mode;
   int group;
};
struct Op_arr {
   struct Op* op;
   size_t size, cap;
};
struct Op_arr new_op_arr() {
   struct Op_arr op_arr;
   op_arr.size = 0;
   op_arr.cap = 4;
   op_arr.op = malloc(op_arr.cap*sizeof(*(op_arr.op)));
   if (not op_arr.op) {
      fprintf(stderr, "malloc failed\n");
      exit(2);
   }
   return op_arr;
}
void op_arr_add(struct Op_arr* arr, struct Op val) {
   arr->op[arr->size] = val;
   arr->size++;
   if (arr->size == arr->cap) {
      arr->cap <<= 1;
      arr->op = realloc(arr->op, arr->cap*sizeof(*(arr->op)));
      if (not arr->op) {
         fprintf(stderr, "realloc failed\n");
         exit(2);
      }
   }
}
void free_op_arr(struct Op_arr arr) {
   free(arr.op);
}
void parse_adv_mode(char const* arg, struct Op_arr* ops) {
   int cursor = 0;
   while (true) {
      while (arg[cursor] and arg[cursor] != ',') {
         int group = 0111;
         if (any_of(arg[cursor], "ugoa")) {
            group = 0;
            do {
               group |=
                  ((arg[cursor]=='a')*73) +
                  ((arg[cursor]=='u')<<6) +
                  ((arg[cursor]=='g')<<3) +
                  ((arg[cursor]=='o')<<0);
               cursor++;
            } while (any_of(arg[cursor], "ugoa"));
         }
         while (arg[cursor] and arg[cursor] != ',') {
            int val = parse_mode(arg, &cursor, group);
            op_arr_add(ops, (struct Op){val, group});
         }
      }
      if (not arg[cursor]) break;
      cursor++;
   }
}
void exec_chmod_on_file(char const* filename, struct Op_arr ops, bool verbose) {
   struct stat file_stat;
   if (stat(filename, &file_stat) == -1) {
      fprintf(stderr, "stat failed\n");
      return;
   }
   if (verbose) {
      printf("old mode for file: %s = %o\n", filename, file_stat.st_mode & 0777);
   }
   int mode = file_stat.st_mode & 0777;

   for (size_t i = 0; i < ops.size; ++i)
      mode = exec_new_mode(mode, ops.op[i].mode, ops.op[i].group) & 0777;
   if (verbose) {
      printf("new mode for file: %s = %o\n", filename, mode);
   }

   if (chmod(filename, mode) < 0) {
      fprintf(stderr, "chmod failed\n");
   }
}

int main(int argc, char** argv) {
   int opt;
   bool verbose = false;
   char const* extra_file = NULL;
   int false_opts = 0;
   
   struct option options[] = {
      {"help",    no_argument, 0, 'h'},
      {"verbose", no_argument, 0, OPT_VERBOSE},
      {"version", no_argument, 0, 'v'},
      {0,0,0,0}
   };

   struct Op_arr ops = new_op_arr();
   while((opt = getopt_long(argc, argv, "hvr:w:x:", options, NULL)) != -1) {
      switch (opt) {
         case 'v':
            printf("version: %s\n", version);
            return 0;
         case 'h':
            printf("%s", help_message);
            return 0;
         case OPT_VERBOSE:
            verbose = true;
            break;
         case 'r': // getopt moment :(
         case 'w': // and i also must parse -rw and -r file  :(
         case 'x':
            char const* arg = argv[optind-1];
            false_opts = 1;
            if (arg[0] == '-') {  // -rw
               parse_adv_mode(arg, &ops);
            } else {              // -r file
               false_opts = 2;
               parse_adv_mode(argv[optind-2], &ops);
               extra_file = arg;
            }
            break;
         default:
            fprintf(stderr, "%s", help_message);
            return 1;
      }
   }

   if (optind+1-false_opts >= argc) {
      fprintf(stderr, "not found files\n%s", help_message);
      free_op_arr(ops);
      return 1;
   }

   if (false_opts == 0) {
      char* mode_arg = argv[optind];

      if (mode_arg[0] >= '0' and mode_arg[0] <= '7' and mode_arg[1] and mode_arg[2]) {
         op_arr_add(&ops, (struct Op){
                  (mode_arg[0]-'0') << 6 |
                  (mode_arg[1]-'0') << 3 |
                  (mode_arg[2]-'0') << 0,
                  0777});
      } else {
         parse_adv_mode(mode_arg, &ops);
      }
   }

   if (extra_file) {
      exec_chmod_on_file(extra_file, ops, verbose);
   }
   for (int i = optind+1-false_opts; i < argc; ++i) {
      char const* filename = argv[i];
      exec_chmod_on_file(filename, ops, verbose);
   }

   if (verbose) {
      printf("chmod succses\n");
   }   

   free_op_arr(ops);
   return 0;
}
