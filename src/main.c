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
   { "nmi",            8, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for nmi"},
   { "phi",            9, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for phi"},
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
   int idx_nmi;
   int idx_phi;
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
   case   8:
      if (arg && strlen(arg) > 0) {
         arguments->idx_nmi = atoi(arg);
      } else {
         arguments->idx_nmi = -1;
      }
      break;
   case   9:
      if (arg && strlen(arg) > 0) {
         arguments->idx_phi = atoi(arg);
      } else {
         arguments->idx_phi = -1;
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
   S_IDLE,
   S_OPCODE,
   S_PREDIS,
   S_POSTDIS,
   S_IMM1,
   S_IMM2,
   S_ROP1,
   S_ROP2,
   S_WOP1,
   S_WOP2
} Z80StateType;

const char *state_names[] = {
   "IDLE",
   "OPCODE",
   "PREDIS",
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

static int instr_len        = 0;
static AnnType ann_dasm     = ANN_NONE;
static const char *mnemonic = NULL;
static FormatType format    = TYPE_0;
static int arg_dis          = 0;
static int arg_imm          = 0;
static int arg_read         = 0;
static int arg_write        = 0;
static char *arg_reg        = NULL;

static Z80StateType state;

static int nmi_pending = 0;

void decode_state(Z80CycleType cycle, int data) {

   static int want_dis    = 0;
   static int want_imm    = 0;
   static int want_read   = 0;
   static int want_write  = 0;
   static int want_wr_be  = False;
   static int conditional = False;
   static int op_prefix   = 0;

   InstrType *table = NULL;
   InstrType *instruction = NULL;

   instr_len += 1;

   switch (state) {

   case S_IDLE:
      want_dis    = 0;
      want_imm    = 0;
      want_read   = 0;
      want_write  = 0;
      want_wr_be  = False;
      conditional = False;
      arg_dis     = 0;
      arg_imm     = 0;
      arg_read    = 0;
      arg_write   = 0;
      arg_reg     = "";
      mnemonic    = "";
      format      = TYPE_0;
      op_prefix   = 0;
      instr_len   = 1;
      state       = S_OPCODE;
      // And fall through to S_OPCODE

   case S_OPCODE:
      // Handle interrupt acknowledge
      if (op_prefix == 0) {
         if (nmi_pending ) {
            printf(" (NMI taken)");
            nmi_pending = 0;
            // NMI always followed by two writes
            want_write = 2;
            state = S_WOP1;
            return;
         }
         if (cycle == C_INTACK) {
            // INT always followed by two writes
            want_write = 2;
            state = S_WOP1;
            return;
         }
      }
      // Check the cycle type...
      if (cycle != ((op_prefix == 0xDDCB || op_prefix == 0xFDCB) ? C_MEMRD : C_FETCH)) {
         mnemonic = "Incorrect cycle type for prefix/opcode";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      if (op_prefix == 0) {
         // Process any first prefix byte
         if (data == 0xCB || data == 0xED || data == 0xDD || data == 0xFD) {
            op_prefix = data;
            return;
         }
      } else if (data == 0xCB && (op_prefix == 0xDD || op_prefix == 0xFD)) {
         // Process any second prefix byte, and then then the pre-displacement
         op_prefix = (op_prefix << 8) | data;
         state = S_PREDIS;
         return;
      }
      // Otherwise, continue to decode the opcode
      table = table_by_prefix(op_prefix);
      arg_reg = reg_by_prefix(op_prefix);
      op_prefix = 0;
      instruction = &table[data];
      if (instruction->want_dis < 0) {
         mnemonic = "Invalid instruction";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      want_dis    = instruction->want_dis;
      want_imm    = instruction->want_imm;
      want_read   = instruction->want_read;
      want_write  = instruction->want_write;
      conditional = instruction->conditional;
      format      = instruction->format;
      mnemonic    = instruction->mnemonic;
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
         if (want_read > 0) {
            state = S_ROP1;
         } else if (want_write > 0) {
            state = S_WOP1;
         } else {
            state = S_IDLE;
         }
      }
      break;

   case S_PREDIS:
      if (cycle != C_MEMRD) {
         mnemonic = "Incorrect cycle type for pre-displacement";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      arg_dis = (char) data; // treat as signed
      state = S_OPCODE;
      break;

   case S_POSTDIS:
      if (cycle != C_MEMRD) {
         mnemonic = "Incorrect cycle type for post displacement";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      arg_dis = (char) data;
      if (want_imm > 0) {
         state = S_IMM1;
      } else {
         ann_dasm = ANN_INSTR;
         if (want_read > 0) {
            state = S_ROP1;
         } else if (want_write > 0) {
            state = S_WOP1;
         } else {
            state = S_IDLE;
         }
      }
      break;

   case S_IMM1:
      if (cycle != C_MEMRD) {
         mnemonic = "Incorrect cycle type for immediate1";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      arg_imm = data;
      if (want_imm > 1) {
         state = S_IMM2;
      } else {
         ann_dasm = ANN_INSTR;
         if (want_read > 0) {
            state = S_ROP1;
         } else if (want_write > 0) {
            state = S_WOP1;
         } else {
            state = S_IDLE;
         }
      }
      break;

   case S_IMM2:
      if (cycle != C_MEMRD) {
         mnemonic = "Incorrect cycle type for immediate2";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      arg_imm |= data << 8;
      ann_dasm = ANN_INSTR;
      if (want_read > 0) {
         state = S_ROP1;
      } else if (want_write > 0) {
         state = S_WOP1;
      } else {
         state = S_IDLE;
      }
      break;

   case S_ROP1:
      // If an instruction is conditional (e.g. RET C) then
      // we might not see any memory accesses, and the next thing
      // will be the fetch of the next instruction
      if (conditional && (cycle == C_FETCH || cycle == C_INTACK)) {
         state = S_IDLE;
         // Recusively call ourselves to handle this case cleanly
         decode_state(cycle, data);
         return;
      }
      if (cycle != C_MEMRD && cycle != C_IORD) {
         mnemonic = "Incorrect cycle type for read op1";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      arg_read = data;
      if (want_read < 2) {
         mnemonic = "Rd: %02X";
         ann_dasm = ANN_ROP;
      }
      if (want_read > 1) {
         state = S_ROP2;
      } else if (want_write > 0) {
         state = S_WOP1;
      } else {
         state = S_IDLE;
      }
      break;

   case S_ROP2:
      if (cycle != C_MEMRD && cycle != C_IORD) {
         mnemonic = "Incorrect cycle type for read op2";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      arg_read |= data << 8;
      mnemonic = "Rd: %04X";
      ann_dasm = ANN_ROP;
      if (want_write > 0) {
         state = S_WOP1;
      } else {
         state = S_IDLE;
      }
      break;

   case S_WOP1:
      // If an instruction is conditional (e.g. CALL C) then
      // we might not see any memory accesses, and the next thing
      // will be the fetch of the next instruction
      if (conditional && (cycle == C_FETCH || cycle == C_INTACK)) {
         state = S_IDLE;
         // Recusively call ourselves to handle this case cleanly
         decode_state(cycle, data);
         return;
      }
      if (cycle != C_MEMWR && cycle != C_IOWR) {
         mnemonic = "Incorrect cycle type for write op1";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      arg_write = data;
      if (want_write > 1) {
         state = S_WOP2;
      } else {
         mnemonic = "Wr: %02X";
         ann_dasm = ANN_WOP;
         state = S_IDLE;
      }
      break;

   case S_WOP2:
      if (cycle != C_MEMWR && cycle != C_IOWR) {
         mnemonic = "Incorrect cycle type for write op2";
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         return;
      }
      if (want_wr_be) {
         arg_write = (arg_write << 8) | data;
      } else {
         arg_write |= data << 8;
      }
      mnemonic = "Wr: %04X";
      ann_dasm = ANN_WOP;
      state = S_IDLE;
      break;
   }
}

void decode_cycle_begin() {
   // for now do nothing
}

void decode_cycle_end(Z80CycleType cycle, int nmi, int data) {
   decode_state(cycle, data);
   switch (ann_dasm) {
   case ANN_INSTR:
      printf(" : %d : ", instr_len);
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
      printf(" : WARNING: ");
      printf(mnemonic, 0);
      break;
   default:
      break;
   }
   ann_dasm = ANN_NONE;
}

void decode_cycle_trans() {
   printf("Illegal transition between control states\n");
}

void decode_cycle(int m1, int rd, int wr, int mreq, int iorq, int wait, int phi, int nmi, int data) {
   static Z80CycleType prev_cycle = C_NONE;
   static int prev_data = 0;
   static int prev_m1 = 0;
   static int prev_rd = 0;
   static int prev_wr = 0;
   static int prev_mreq = 0;
   static int prev_iorq = 0;
   static int prev_wait = 0;
   static int prev_nmi = 0;
   static int prev_phi = 0;

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

   printf("%6s %10s %d %d %d %d %d %d %d %d %02x",
          cycle_names[prev_cycle],
          state_names[state],
          prev_m1,
          prev_rd,
          prev_wr,
          prev_mreq,
          prev_iorq,
          prev_wait,
          prev_nmi,
          prev_phi,
          prev_data);

   if (cycle != prev_cycle) {
      if (prev_cycle == C_NONE) {
         printf(" . ");
         decode_cycle_begin();
      } else if (cycle == C_NONE) {
         printf(" * ");
         decode_cycle_end(prev_cycle, prev_nmi, prev_data);
      } else {
         printf(" . ");
         decode_cycle_trans();
      }
   } else {
      printf(" . ");
   }

   // Look for a falling edge of NMI
   if (nmi == 0 && prev_nmi == 1) {
      nmi_pending = 1;
      printf("(NMI pending)");
   }

   printf("\n");

   prev_cycle = cycle;
   prev_m1    = m1;
   prev_rd    = rd;
   prev_wr    = wr;
   prev_mreq  = mreq;
   prev_iorq  = iorq;
   prev_wait  = wait;
   prev_nmi   = nmi;
   prev_phi   = phi;
   prev_data  = data;


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
   int idx_nmi   = arguments.idx_nmi;
   int idx_phi   = arguments.idx_phi ;

   int num;

   // The previous sample of the 16-bit capture (async sampling only)
   uint16_t sample       = -1;

   int m1;
   int rd;
   int wr;
   int mreq;
   int iorq;
   int wait;
   int nmi;
   int phi;
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
         nmi  = (sample >> idx_nmi ) & 1;
         phi  = (sample >> idx_phi ) & 1;
         data = (sample >> idx_data) & 255;

         decode_cycle(m1, rd, wr, mreq, iorq, wait, nmi, phi, data);

      }
   }
}


// ====================================================================
// Main program entry point
// ====================================================================

int main(int argc, char *argv[]) {
   arguments.idx_data         =  0;
   arguments.idx_m1           =  8;
   arguments.idx_rd           =  9;
   arguments.idx_wr           = 10;
   arguments.idx_mreq         = 11;
   arguments.idx_iorq         = 12;
   arguments.idx_wait         = 13;
   arguments.idx_nmi          = 14;
   arguments.idx_phi          = 15;
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
