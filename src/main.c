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
// Z80 Bus State Machine
// ====================================================================


typedef enum {
   S_RESTART,
   S_IDLE,
   S_PRE1,
   S_PRE2,
   S_PREDIS,
   S_OPCODE,
   S_POSTDIS,
   S_IMM1,
   S_IMM2,
   S_ROP1,
   S_ROP2,
   S_WOP1,
   S_WOP2
} Z80StateType;

const char *state_names[] = {
   "RESTART",
   "IDLE",
   "PRE1",
   "PRE2",
   "PREDIS",
   "OPCODE",
   "POSTDIS",
   "IMM1",
   "IMM2",
   "ROP1",
   "ROP2",
   "WOP1",
   "WOP2"
};

typedef enum {
   C_NONE,
   C_FETCH,
   C_MEMRD,
   C_MEMWR,
   C_IORD,
   C_IOWR,
   C_INTACK
} Z80CycleType;

const char *cycle_names[] = {
   "NONE",
   "FETCH",
   "MEMRD",
   "MEMWR",
   "IORD",
   "IOWR",
   "INTACK",
};

typedef enum {
   ANN_NONE,
   ANN_WARN,
   ANN_INSTR,
   ANN_ROP,
   ANN_WOP
} AnnType;

static int instr_len = 0;
static AnnType ann_dasm;
static const char *mnemonic = NULL;
static FormatType format = TYPE_0;
static int arg_dis = 0;
static int arg_imm = 0;
static int arg_read = 0;
static int arg_write = 0;
static char *arg_reg = NULL;

static Z80StateType state;

void decode_state(Z80CycleType cycle, Z80CycleType prev_cycle, int bus_data, int pend_data) {

   static int want_dis = 0;
   static int want_imm = 0;
   static int want_read = 0;
   static int want_write = 0;
   static int want_wr_be = False;
   static int op_repeat = False;
   static int op_prefix = 0;

   InstrType *table = NULL;
   InstrType *instruction = NULL;

   //printf("    cycle: %s\n", cycle_names[cycle]);
   //printf("  p_cycle: %s\n", cycle_names[prev_cycle]);
   //printf("     data: %08X\n", bus_data);
   //printf("   p_data: %08X\n", pend_data);

   switch (state) {

   case S_RESTART:
      state = S_IDLE;
      break;

   case S_IDLE:
      if (prev_cycle == C_FETCH) {
         want_dis   = 0;
         want_imm   = 0;
         want_read  = 0;
         want_write = 0;
         want_wr_be = False;
         op_repeat  = False;
         arg_dis    = 0;
         arg_imm    = 0;
         arg_read   = 0;
         arg_write  = 0;
         arg_reg    = "";
         mnemonic   = "";
         format     = TYPE_0;
         op_prefix  = 0;
         instr_len  = 0;
         if (bus_data == 0xCB || bus_data == 0xED || bus_data == 0xDD || bus_data == 0xFD) {
            state = S_PRE1;
         } else {
            state = S_OPCODE;
         }
      }
      break;

   case S_PRE1:
      if (prev_cycle != C_FETCH) {
         mnemonic = "Prefix not followed by fetch";
         ann_dasm = ANN_WARN;
         state = S_RESTART;
      } else {
         op_prefix = pend_data;
         if (op_prefix == 0xDD || op_prefix == 0xFD) {
            if (bus_data == 0xCB) {
               state = S_PRE2;
               break;
            }
            if (bus_data == 0xDD || bus_data == 0xED || bus_data == 0xFD) {
               state = S_PRE1;
               break;
            }
         }
         state = S_OPCODE;
      }
      break;

   case S_PRE2:
      if (prev_cycle != C_MEMRD) {
         mnemonic = "Missing displacement";
         ann_dasm = ANN_WARN;
         state = S_RESTART;
      } else {
         op_prefix = (op_prefix << 8) | pend_data;
         state = S_PREDIS;
      }
      break;

   case S_PREDIS:
      if (prev_cycle != C_MEMRD) {
         mnemonic = "Missing opcode";
         ann_dasm = ANN_WARN;
         state = S_RESTART;
      } else {
         arg_dis = (char) pend_data; // tread as signed
         state = S_OPCODE;
      }
      break;

   case S_OPCODE:
      table = table_by_prefix(op_prefix);
      arg_reg = reg_by_prefix(op_prefix);
      op_prefix = 0;
      instruction = &table[pend_data];
      if (instruction->want_dis < 0) {
         mnemonic = "Invalid instruction";
         ann_dasm = ANN_WARN;
         state = S_RESTART;
      } else {
         want_dis   = instruction->want_dis;
         want_imm   = instruction->want_imm;
         want_read  = instruction->want_read;
         want_write = instruction->want_write;
         op_repeat  = instruction->op_repeat;
         format     = instruction->format;
         mnemonic   = instruction->mnemonic;
         if (want_write < 0) {
            want_wr_be = True;
            want_write = -want_write;
         } else {
            want_wr_be = False;
         }
         if (want_dis > 0) {
            state = S_POSTDIS;
         } else if (want_imm > 0) {
            state = S_IMM1;
         } else {
            ann_dasm = ANN_INSTR;
            if (want_read > 0 && (prev_cycle == C_MEMRD || prev_cycle == C_IORD)) {
               state = S_ROP1;
            } else if (want_write > 0 && (prev_cycle == C_MEMWR || prev_cycle == C_IOWR)) {
               state = S_WOP1;
            } else {
               state = S_RESTART;
            }
         }
      }
      break;

   case S_POSTDIS:
      arg_dis = (char) pend_data;
      if (want_imm > 0) {
         state = S_IMM1;
      } else {
         ann_dasm = ANN_INSTR;
         if (want_read > 0 && (prev_cycle == C_MEMRD || prev_cycle == C_IORD)) {
            state = S_ROP1;
         } else if (want_write > 0 && (prev_cycle == C_MEMWR || prev_cycle == C_IOWR)) {
            state = S_WOP1;
         } else {
            state = S_RESTART;
         }
      }
      break;

   case S_IMM1:
      arg_imm = pend_data;
      if (want_imm > 1) {
         state = S_IMM2;
      } else {
         ann_dasm = ANN_INSTR;
         if (want_read > 0 && (prev_cycle == C_MEMRD || prev_cycle == C_IORD)) {
            state = S_ROP1;
         } else if (want_write > 0 && (prev_cycle == C_MEMWR || prev_cycle == C_IOWR)) {
            state = S_WOP1;
         } else {
            state = S_RESTART;
         }
      }
      break;

   case S_IMM2:
      arg_imm |= pend_data << 8;
      ann_dasm = ANN_INSTR;
      if (want_read > 0 && (prev_cycle == C_MEMRD || prev_cycle == C_IORD)) {
         state = S_ROP1;
      } else if (want_write > 0 && (prev_cycle == C_MEMWR || prev_cycle == C_IOWR)) {
         state = S_WOP1;
      } else {
         state = S_RESTART;
      }
      break;

   case S_ROP1:
      arg_read = pend_data;
      if (want_read < 2) {
         mnemonic = "Rd: %02X";
         ann_dasm = ANN_ROP;
      }
      if (want_read > 1) {
         state = S_ROP2;
      } else if (want_write > 0) {
         state = S_WOP1;
      } else if (op_repeat && (prev_cycle == C_MEMRD || prev_cycle == C_IORD)) {
         // Repeating ops seem unnecessary
         state = S_ROP1;
      } else {
         state = S_RESTART;
      }
      break;

   case S_ROP2:
      arg_read |= pend_data << 8;
      mnemonic = "Rd: %04X";
      ann_dasm = ANN_ROP;
      if (want_write > 0 && (prev_cycle == C_MEMWR || prev_cycle == C_IOWR)) {
         state = S_WOP1;
      } else {
         state = S_RESTART;
      }
      break;

   case S_WOP1:
      arg_write = pend_data;
      //if (want_read > 1) {
      //   state = S_ROP2;
      //} else
      if (want_write > 1) {
         state = S_WOP2;
      } else {
         mnemonic = "Wr: %02X";
         ann_dasm = ANN_WOP;
         if (want_read > 0 && op_repeat && (prev_cycle == C_MEMRD || prev_cycle == C_IORD)) {
            // Repeating ops seem unnecessary
            state = S_ROP1;
         } else {
            state = S_RESTART;
         }
      }
      break;

   case S_WOP2:
      if (want_wr_be) {
         arg_write = (arg_write << 8) | pend_data;
      } else {
         arg_write |= pend_data << 8;
      };
      mnemonic = "Wr: %04X";
      ann_dasm = ANN_WOP;
      state = S_RESTART;
      break;
      }
}


void decode_cycle_begin() {
   // for now do nothing
}

void decode_cycle_end(Z80CycleType prev_cycle, Z80CycleType cycle, int pend_data, int bus_data) {
   instr_len += 1;
   if (cycle == C_INTACK) {
      printf("*** INTACK ***\n");
   }
   decode_state(cycle, prev_cycle, bus_data, pend_data);
   switch (ann_dasm) {
   case ANN_INSTR:
      printf(" : ");
      switch (format) {
      case TYPE_1:
         printf(mnemonic, arg_reg);
         break;
      case TYPE_2:
         printf(mnemonic, arg_reg, arg_reg);
         break;
      case TYPE_3:
         printf(mnemonic, arg_imm, arg_reg);
         break;
      case TYPE_4:
         printf(mnemonic, arg_reg, arg_imm);
         break;
      case TYPE_5:
         printf(mnemonic, arg_reg, arg_dis);
         break;
      case TYPE_6:
         printf(mnemonic, arg_reg, arg_dis, arg_imm);
         break;
      case TYPE_7:
         printf(mnemonic, arg_dis + instr_len);
         break;
      case TYPE_8:
         printf(mnemonic, arg_imm);
         break;
      default:
         printf(mnemonic, 0);
         break;
      }
      break;
   case ANN_ROP:
      printf(" : ");
      printf(mnemonic, arg_read);
      break;
   case ANN_WOP:
      printf(" : ");
      printf(mnemonic, arg_write);
      break;
   case ANN_WARN:
      printf(" : ");
      printf(mnemonic, 0);
      break;
   default:
      break;
   }
   ann_dasm = ANN_NONE;
   // This seems a rather complex way of jumping two states
   if (state == S_RESTART) {
      state = S_IDLE;
      decode_state(cycle, prev_cycle, bus_data, pend_data);
   }
}

void decode_cycle_trans() {
   printf("Illegal transition between control states\n");
}

void decode_cycle(int m1, int rd, int wr, int mreq, int iorq, int data) {
   static Z80CycleType prev_cycle = C_NONE;
   static int bus_data = 0;
   static int pend_data = 0;
   Z80CycleType cycle = C_NONE;
   if (mreq == 0) {
      if (rd == 0) {
         if (m1 == 0) {
            cycle = C_FETCH;
         } else {
            cycle = C_MEMRD;
         }
      } else if (wr == 0) {
         cycle = C_MEMWR;
      }
   } else if (iorq == 0) {
      if (m1 == 0) {
         cycle = C_INTACK;
      } else if (rd == 0) {
         cycle = C_IORD;
      } else if (wr == 0) {
         cycle = C_IOWR;
      }
   }

   printf("%6s %10s %d %d %d %d %d %02x", cycle_names[cycle], state_names[state], m1, rd, wr, mreq, iorq, data);

   if (cycle != C_NONE) {
      bus_data = data;
   }
   if (cycle != prev_cycle) {
      if (prev_cycle == C_NONE) {
         decode_cycle_begin();
      } else if (cycle == C_NONE) {
         //printf("    before: %s\n", state_names[state]);
         decode_cycle_end(prev_cycle, cycle, pend_data, bus_data);
         pend_data = bus_data;
         //printf("     after: %s\n", state_names[state]);
      } else {
         decode_cycle_trans();
      }
   }
   printf("\n");



   prev_cycle = cycle;
}





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

   int m1;
   int rd;
   int wr;
   int mreq;
   int iorq;
   int wait;
   int data;

   while ((num = fread(buffer, sizeof(uint16_t), BUFSIZE, stream)) > 0) {

      uint16_t *sampleptr = &buffer[0];

      while (num-- > 0) {

         sample       = *sampleptr++;

         m1   = (sample >> idx_m1  ) & 1;
         rd   = (sample >> idx_rd  ) & 1;
         wr   = (sample >> idx_wr  ) & 1;
         mreq = (sample >> idx_mreq) & 1;
         iorq = (sample >> idx_iorq) & 1;
         wait = (sample >> idx_wait) & 1;
         data = (sample >> idx_data) & 255;

         decode_cycle(m1, rd, wr, mreq, iorq, data);

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
