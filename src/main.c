#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <argp.h>
#include <string.h>

#include "z80tables.h"

#define BUFSIZE 8192

uint16_t buffer[BUFSIZE];


// ====================================================================
// Argp processing
// ====================================================================

const char *argp_program_version = "decodez80 0.1";

const char *argp_program_bug_address = "<dave@hoglet.com>";

static char doc[] = "\n\
Decoder for Z80 logic analyzer capture files.\n\
\n\
";

static char args_doc[] = "[FILENAME]";

static struct argp_option options[] = {
   { "data",           1, "BITNUM",                   0, "The start bit number for data"},
   { "m1",             2, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for m1"},
   { "rd",             3, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for rd"},
   { "wr",             4, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for wr"},
   { "mreq",           5, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for mreq"},
   { "iorq",           6, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for iorq"},
   { "wait",           7, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for wait"},
   { 0 }
};

struct arguments {
   int idx_data;
   int idx_m1;
   int idx_rd;
   int idx_wr;
   int idx_mreq;
   int idx_iorq;
   int idx_wait;
   char *filename;
} arguments;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
   struct arguments *arguments = state->input;

   switch (key) {
   case   1:
      arguments->idx_data = atoi(arg);
   case   2:
      if (arg && strlen(arg) > 0) {
         arguments->idx_m1 = atoi(arg);
      } else {
         arguments->idx_m1 = -1;
      }
      break;
   case   3:
      if (arg && strlen(arg) > 0) {
         arguments->idx_rd = atoi(arg);
      } else {
         arguments->idx_rd = -1;
      }
      break;
   case   4:
      if (arg && strlen(arg) > 0) {
         arguments->idx_wr = atoi(arg);
      } else {
         arguments->idx_wr = -1;
      }
      break;
   case   5:
      if (arg && strlen(arg) > 0) {
         arguments->idx_mreq = atoi(arg);
      } else {
         arguments->idx_mreq = -1;
      }
      break;
   case   6:
      if (arg && strlen(arg) > 0) {
         arguments->idx_iorq = atoi(arg);
      } else {
         arguments->idx_iorq = -1;
      }
      break;
   case   7:
      if (arg && strlen(arg) > 0) {
         arguments->idx_wait = atoi(arg);
      } else {
         arguments->idx_wait = -1;
      }
      break;
   case ARGP_KEY_ARG:
      arguments->filename = arg;
      break;
   case ARGP_KEY_END:
      if (state->arg_num > 1) {
         argp_error(state, "multiple capture file arguments");
      }
      break;
   default:
      return ARGP_ERR_UNKNOWN;
   }
   return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };


// ====================================================================
// Input file processing and bus cycle extraction
// ====================================================================

void decode(FILE *stream) {

   // Pin mappings into the 16 bit words
   int idx_data  = arguments.idx_data;
   int idx_m1    = arguments.idx_m1  ;
   int idx_rd    = arguments.idx_rd  ;
   int idx_wr    = arguments.idx_wr  ;
   int idx_mreq  = arguments.idx_mreq;
   int idx_iorq  = arguments.idx_iorq;
   int idx_wait  = arguments.idx_wait;

   int num;

   // The previous sample of the 16-bit capture (async sampling only)
   uint16_t sample       = -1;

   int data;
   int pin_m1;
   int pin_rd;
   int pin_wr;
   int pin_mreq;
   int pin_iorq;
   int pin_wait;

   unsigned int prefix = 0;

   while ((num = fread(buffer, sizeof(uint16_t), BUFSIZE, stream)) > 0) {

      uint16_t *sampleptr = &buffer[0];

      while (num-- > 0) {

         sample       = *sampleptr++;

         data = (sample >> idx_data) & 255;
         pin_m1   = (sample >> idx_m1  ) & 1;
         pin_rd   = (sample >> idx_rd  ) & 1;
         pin_wr   = (sample >> idx_wr  ) & 1;
         pin_mreq = (sample >> idx_mreq) & 1;
         pin_iorq = (sample >> idx_iorq) & 1;
         pin_wait = (sample >> idx_wait) & 1;





         InstrType* table = NULL;

         if (pin_wait == 0) {
            continue;
         }

         if (pin_mreq == 0) {
            if (pin_rd == 0) {
               if (pin_m1 == 0) {
                  printf("fetch   ");

                  if (prefix == 0) {
                     if (data == 0xcb || data == 0xdd || data == 0xed || data == 0xfd) {
                        prefix = data;
                     } else  {
                        table = main_instructions;
                     }
                  } else if (prefix == 0xcb) {
                     table = bit_instructions;
                     prefix = 0;
                  } else if (prefix == 0xdd) {
                     if (data == 0xcb) {
                        prefix = (prefix << 8) | data;
                     } else {
                        table = index_instructions;
                        prefix = 0;
                     }
                  } else if (prefix == 0xed) {
                     table = extended_instructions;
                     prefix = 0;
                  } else if (prefix == 0xfd) {
                     if (data == 0xcb) {
                        prefix = (prefix << 8) | data;
                     } else {
                        table = index_instructions;
                        prefix = 0;
                     }
                  } else if (prefix == 0xddcb) {
                     table = index_bit_instructions;
                     prefix = 0;
                  } else if (prefix == 0xfdcb) {
                     table = index_bit_instructions;
                     prefix = 0;
                  } else {
                     printf("*** illegal prefix: %x ***\n", prefix);
                     prefix = 0;
                  }
               } else {
                  printf("memrd   ");
               }
            } else if (pin_wr == 0) {
               printf("memwr   ");
            } else {
               printf("mem??   ");
            }
         } else if (pin_iorq == 0) {
            if (pin_m1 == 0) {
               printf("intack  ");
            } else if (pin_rd == 0) {
               printf("iord    ");
            } else if (pin_wr == 0) {
               printf("iowr    ");
            } else {
               printf("io??    ");
            }
         } else {
            printf("        ");
         }

         printf("%d %d %d %d %d %d %02x",
                pin_m1, pin_rd, pin_wr, pin_mreq, pin_iorq, pin_wait, data);

         if (table) {
            printf(" : %s\n", table[data].fmt);
            table = NULL;
         } else {
            printf("\n");
         }
      }
   }
}


// ====================================================================
// Main program entry point
// ====================================================================

int main(int argc, char *argv[]) {
   arguments.idx_data         =  0;
   arguments.idx_m1          =  8;
   arguments.idx_rd         =  9;
   arguments.idx_wr          = 10;
   arguments.idx_mreq         = 11;
   arguments.idx_iorq          = 12;
   arguments.idx_wait          = 13;
   arguments.filename         = NULL;

   argp_parse(&argp, argc, argv, 0, 0, &arguments);

   FILE *stream;
   if (!arguments.filename || !strcmp(arguments.filename, "-")) {
      stream = stdin;
   } else {
      stream = fopen(arguments.filename, "r");
      if (stream == NULL) {
         perror("failed to open capture file");
         return 2;
      }
   }
   decode(stream);
   fclose(stream);

   return 0;
}
