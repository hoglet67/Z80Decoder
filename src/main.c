#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <argp.h>
#include <string.h>

#include "em_z80.h"

// #define DUMP_COVERAGE

#define MAX_INSTR_LEN 5

#define DEPTH 4

#define RESET_THRESHOLD 1000

#define READ_BUFSIZE 8192

#define SAMPLE_BUFSIZE 8192

uint16_t buffer[READ_BUFSIZE];

uint16_t sample_buffer[SAMPLE_BUFSIZE];

// Whether to emulate each decoded instruction, to track additional state (registers and flags)
int do_emulate = 0;

// ====================================================================
// Argp processing
// ====================================================================

const char *cpu_names[] = {
   "default",
   "nmos_zilog",
   "nmos_nec",
   "cmos",
   0
};

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
   { "rst",            8, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for rst"},
   { "phi",            9, "BITNUM", OPTION_ARG_OPTIONAL, "The bit number for phi"},
   { "debug",        'd',  "LEVEL",                   0, "Sets debug level (0 1 or 2)"},
// Output options
   { "address",      'a',        0,                   0, "Show address of instruction."},
   { "hex",          'h',        0,                   0, "Show hex bytes of instruction."},
   { "instruction",  'i',        0,                   0, "Show instruction."},
   { "state",        's',  "LEVEL", OPTION_ARG_OPTIONAL, "Show register/flag state."},
   { "cycles",       'y',        0,                   0, "Show number of bus cycles."},
   { "cpu",          'c',    "CPU",                   0, "Enable cpu specific behaviour"},
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
   int idx_rst;
   int idx_phi;
   char *filename;
   int show_address;
   int show_hex;
   int show_instruction;
   int show_state;
   int show_cycles;
   int cpu;
   int debug;
} arguments;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
   int i;
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
         arguments->idx_rst = atoi(arg);
      } else {
         arguments->idx_rst = -1;
      }
      break;
   case   9:
      if (arg && strlen(arg) > 0) {
         arguments->idx_phi = atoi(arg);
      } else {
         arguments->idx_phi = -1;
      }
      break;
   case 'c':
      i = 0;
      while (cpu_names[i]) {
         if (strcasecmp(arg, cpu_names[i]) == 0) {
            arguments->cpu = i;
            return 0;
         }
         i++;
      }
      argp_error(state, "unsupported cpu type");
      break;
   case 'd':
      arguments->debug = atoi(arg);
      break;
   case 'a':
      arguments->show_address = 1;
      break;
   case 'h':
      arguments->show_hex = 1;
      break;
   case 'i':
      arguments->show_instruction = 1;
      break;
   case 's':
      if (arg && strlen(arg) > 0) {
         arguments->show_state = atoi(arg);
      } else {
         arguments->show_state = 1;
      }
      break;
   case 'y':
      arguments->show_cycles = 1;
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
   "INTACK"
};

typedef enum {
   ANN_NONE,
   ANN_WARN,
   ANN_INSTR,
   ANN_ROP1,
   ANN_WOP1,
   ANN_ROP2,
   ANN_WOP2
} AnnType;


typedef struct {
   Z80CycleType cycle;
   int data;
   int num_samples;
   int instr_cycles;
   int wait_cycles;
   int sample_index;
} Z80CycleSummaryType;


int prefix             = 0;
int opcode             = 0;
int arg_dis            = 0;
int arg_imm            = 0;
int arg_read           = 0;
int arg_write          = 0;
int failflag           = FAIL_NONE;
int instr_len          = 0;
InstrType *instruction = NULL;

static int instr_bytes[MAX_INSTR_LEN];
static AnnType ann_dasm     = ANN_NONE;
static const char *mnemonic = NULL;
static FormatType format    = TYPE_0;
static char *arg_reg        = NULL;

static Z80StateType state;

// Indicates the data bus value was not processed, and needs
// to be re-presented
#define BIT_UNPROCESSED 1

// Indicates the end of an instruction execution
#define BIT_INSTRUCTION 4

int decode_instruction(Z80CycleSummaryType *cycle_q) {

   static int want_dis    = 0;
   static int want_imm    = 0;
   static int want_read   = 0;
   static int want_write  = 0;
   static int want_wr_be  = False;
   static int conditional = False;
   static char warning_buffer[80];

   int cycle = cycle_q->cycle;
   int data  = cycle_q->data;

   int ret = 0;

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
      opcode      = 0;
      prefix      = 0;
      instruction = NULL;
      state       = S_OPCODE;
      instr_len   = 0;
      // And fall through to S_OPCODE

   case S_OPCODE:
      // Check the cycle type...
      if (cycle != C_INTACK && cycle != ((prefix == 0xDDCB || prefix == 0xFDCB) ? C_MEMRD : C_FETCH)) {
         sprintf(warning_buffer, "Incorrect cycle type for prefix/opcode: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         break;
      }
      if (cycle == C_INTACK) {
         // Treat an INT interrupt as just another instruction
         prefix = 0;
         instr_len = 0;
         // The opcode represents the "vector" captured during the interrupt acknowlehge cycle
         opcode = data;
         instruction = &z80_interrupt_int;
      } else if (prefix == 0 &&
                 (cycle_q + 1)->cycle == C_MEMWR &&
                 (cycle_q + 2)->cycle == C_MEMWR &&
                 (cycle_q + 3)->cycle == C_FETCH &&
                 z80_get_pc() == (((cycle_q + 1)->data << 8) + (cycle_q + 2)->data) &&
                 (((cycle_q + 3)->data == 0x08) | // EX AF, AF'
                  ((cycle_q + 3)->data == 0xC3)) // JP
         ) {
         // Treat an NMI interrupt as just another instruction
         prefix = 0;
         instr_len = 0;
         opcode = 0;
         instruction = &z80_interrupt_nmi;
      } else if (z80_halted()) {
         // When halted, execute an NOP
         prefix = 0;
         instr_len = 0;
         opcode = 0;
         instruction = &table_by_prefix(0)[0];
      } else if ((prefix == 0) && (data == 0xCB || data == 0xED || data == 0xDD || data == 0xFD)) {
         // Process any first prefix byte
         prefix = data;
         instr_bytes[instr_len++] = data;
         // Increment the refresh address register for the first prefix byte
         z80_increment_r();
         break;
      } else if ((prefix == 0xDD || prefix == 0xFD) && (data == 0xCB)) {
         // Process any second prefix byte
         prefix = (prefix << 8) | data;
         instr_bytes[instr_len++] = data;
         // Increment the refresh address register for the second prefix byte
         z80_increment_r();
         // 0xDDCB or 0xFDCB is followed by a mandatory displacement
         state = S_PREDIS;
         break;
      } else {
         // Decode the prefix/opcode normally
         InstrType *table = table_by_prefix(prefix);
         arg_reg = reg_by_prefix(prefix);
         opcode = data;
         instr_bytes[instr_len++] = data;
         instruction = &table[opcode];
      }
      // Increment the refresh address register for the opcode, unless it's already been done
      if (prefix != 0xDDCB && prefix != 0xFDCB) {
         z80_increment_r();
      }
      // Undefined opcodes in blocks 0xDD and 0xFD act like the unprefixed opcode
      if ((prefix == 0xDD || prefix == 0xFD) && (instruction->want_dis < 0)) {
         InstrType *table = table_by_prefix(0);
         instruction = &table[opcode];
      }
      // If we get this far without hitting a break, we are ready to execute an instruction
      want_dis    = instruction->want_dis;
      want_imm    = instruction->want_imm;
      want_read   = instruction->want_read;
      want_write  = instruction->want_write;
      conditional = instruction->conditional;
      format      = instruction->format;
      mnemonic    = instruction->mnemonic;
      instruction->count++;
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
            ret |= BIT_INSTRUCTION;
         }
      }
      break;

   case S_PREDIS:
      if (cycle != C_MEMRD) {
         sprintf(warning_buffer, "Incorrect cycle type for pre-displacement: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         ret |= BIT_UNPROCESSED;
         break;
      }
      arg_dis = (char) data; // treat as signed
      instr_bytes[instr_len++] = data;
      state = S_OPCODE;
      break;

   case S_POSTDIS:
      if (cycle != C_MEMRD) {
         sprintf(warning_buffer, "Incorrect cycle type for post displacement: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         ret |= BIT_UNPROCESSED;
         break;
      }
      arg_dis = (char) data;
      instr_bytes[instr_len++] = data;
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
            ret |= BIT_INSTRUCTION;
         }
      }
      break;

   case S_IMM1:
      if (cycle != C_MEMRD) {
         sprintf(warning_buffer, "Incorrect cycle type for immediate1: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         ret |= BIT_UNPROCESSED;
         break;
      }
      arg_imm = data;
      instr_bytes[instr_len++] = data;
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
            ret |= BIT_INSTRUCTION;
         }
      }
      break;

   case S_IMM2:
      if (cycle != C_MEMRD) {
         sprintf(warning_buffer, "Incorrect cycle type for immediate2: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         ret |= BIT_UNPROCESSED;
         break;
      }
      arg_imm |= data << 8;
      instr_bytes[instr_len++] = data;
      ann_dasm = ANN_INSTR;
      if (want_read > 0) {
         state = S_ROP1;
      } else if (want_write > 0) {
         state = S_WOP1;
      } else {
         state = S_IDLE;
         ret |= BIT_INSTRUCTION;
      }
      break;

   case S_ROP1:
      // If an instruction is conditional (e.g. RET C) then
      // we might not see any memory accesses, and the next thing
      // will be the fetch of the next instruction
      if (conditional && (cycle == C_FETCH || cycle == C_INTACK)) {
         state = S_IDLE;
         ret |= BIT_INSTRUCTION | BIT_UNPROCESSED;
         break;
      }
      if (cycle != C_MEMRD && cycle != C_IORD) {
         sprintf(warning_buffer, "Incorrect cycle type for read op1: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         ret |= BIT_UNPROCESSED;
         break;
      }
      arg_read = data;
      if (want_read < 2) {
         ann_dasm = ANN_ROP1;
      }
      if (want_read > 1) {
         state = S_ROP2;
      } else if (want_write > 0) {
         state = S_WOP1;
      } else {
         state = S_IDLE;
         ret |= BIT_INSTRUCTION;
      }
      break;

   case S_ROP2:
      if (cycle != C_MEMRD && cycle != C_IORD) {
         sprintf(warning_buffer, "Incorrect cycle type for read op2: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         ret |= BIT_UNPROCESSED;
         break;
      }
      arg_read |= data << 8;
      ann_dasm = ANN_ROP2;
      if (want_write > 0) {
         state = S_WOP1;
      } else {
         state = S_IDLE;
         ret |= BIT_INSTRUCTION;
      }
      break;

   case S_WOP1:
      // If an instruction is conditional (e.g. CALL C) then
      // we might not see any memory accesses, and the next thing
      // will be the fetch of the next instruction
      if (conditional && (cycle == C_FETCH || cycle == C_INTACK)) {
         state = S_IDLE;
         ret |= BIT_INSTRUCTION | BIT_UNPROCESSED;
         break;
      }
      if (cycle != C_MEMWR && cycle != C_IOWR) {
         sprintf(warning_buffer, "Incorrect cycle type for write op1: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         ret |= BIT_UNPROCESSED;
         break;
      }
      arg_write = data;
      if (want_write > 1) {
         state = S_WOP2;
      } else {
         ann_dasm = ANN_WOP1;
         state = S_IDLE;
         ret |= BIT_INSTRUCTION;
      }
      break;

   case S_WOP2:
      if (cycle != C_MEMWR && cycle != C_IOWR) {
         sprintf(warning_buffer, "Incorrect cycle type for write op2: %s", cycle_names[cycle]);
         mnemonic = warning_buffer;
         ann_dasm = ANN_WARN;
         state = S_IDLE;
         ret |= BIT_UNPROCESSED;
         break;
      }
      if (want_wr_be) {
         arg_write = (arg_write << 8) | data;
      } else {
         arg_write |= data << 8;
      }
      ann_dasm = ANN_WOP2;
      // Hard-code a test for IM 2
      if (instruction == &z80_interrupt_int && z80_get_im() == 2) {
         want_write = 0;
         want_read = 2;
         state = S_ROP1;
      } else {
         state = S_IDLE;
         ret |= BIT_INSTRUCTION;
      }
      break;
   }

   return ret;
}

Z80CycleType get_cycle_type(uint16_t sample) {
   // Extract the control signals from the sample
   int m1   = (sample >> arguments.idx_m1  ) & 1;
   int rd   = (sample >> arguments.idx_rd  ) & 1;
   int wr   = (sample >> arguments.idx_wr  ) & 1;
   int mreq = (sample >> arguments.idx_mreq) & 1;
   int iorq = (sample >> arguments.idx_iorq) & 1;

   // Determine the cycle type
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
   return cycle;
}


void decode_cycle(Z80CycleSummaryType *cycle_q) {

   static int m_cycle = 0;
   static int instr_cycles = 0;
   static int wait_cycles = 0;
   int ret;
   int colon;
   char target[10];

   do {

      ret = decode_instruction(cycle_q);

      // Output the samples for this cycle, as long as they are processed
      if (!(ret & BIT_UNPROCESSED)) {

         instr_cycles += cycle_q->instr_cycles;
         wait_cycles += cycle_q->wait_cycles;

         if (arguments.debug > 0) {

            if (cycle_q->cycle == C_FETCH) {
               m_cycle = 1;
            } else {
               m_cycle++;
            }

            if (arguments.debug > 1) {
               int end = cycle_q->num_samples;
               for (int i = 0; i < end; i++) {
                  uint16_t sample = sample_buffer[(cycle_q->sample_index + i) & (SAMPLE_BUFSIZE - 1)];
                  Z80CycleType cycle = get_cycle_type(sample);
                  int m1   = (sample >> arguments.idx_m1  ) & 1;
                  int rd   = (sample >> arguments.idx_rd  ) & 1;
                  int wr   = (sample >> arguments.idx_wr  ) & 1;
                  int mreq = (sample >> arguments.idx_mreq) & 1;
                  int iorq = (sample >> arguments.idx_iorq) & 1;
                  int wait = (sample >> arguments.idx_wait) & 1;
                  int rst  = (sample >> arguments.idx_rst ) & 1;
                  int phi  = (sample >> arguments.idx_phi ) & 1;
                  int data = (sample >> arguments.idx_data) & 255;
                  printf("M%d %6s %d %d %d %d %d %d %d %d %02x ",
                         m_cycle, cycle_names[cycle],
                         m1, rd, wr, mreq, iorq, wait, rst, phi, data);
                  if (i < end - 1) {
                     printf("\n");
                  }
               }
            } else {
               printf("M%d %6s %02x %2d/%2d",
                      m_cycle, cycle_names[cycle_q->cycle],
                      cycle_q->data, cycle_q->instr_cycles, cycle_q->wait_cycles);
            }

            switch (ann_dasm) {
            case ANN_ROP1:
               printf(": Rd=%02X", arg_read);
               ann_dasm = ANN_NONE;
               break;
            case ANN_ROP2:
               printf(": Rd=%04X", arg_read);
               ann_dasm = ANN_NONE;
               break;
            case ANN_WOP1:
               printf(": Wr=%02X", arg_write);
               ann_dasm = ANN_NONE;
               break;
            case ANN_WOP2:
               printf(": Wr=%04X", arg_write);
               ann_dasm = ANN_NONE;
               break;
            default:
               break;
            }
            printf("\n");
         }
      }

      // Handle Warnings
      if (ann_dasm == ANN_WARN) {
         printf("WARNING: %s\n", mnemonic);
         ann_dasm = ANN_NONE;
      }

      if (ret & BIT_INSTRUCTION) {

         if (arguments.debug > 0) {
            printf("\n");
         }

         if (instr_cycles + wait_cycles > RESET_THRESHOLD) {
            z80_reset();
            printf("INFO: RESET inferred\n");
         }

         int count = 0;
         colon = 0;
         // We have everything available to process a complete instruction
         if (arguments.show_address) {
            if (z80_get_pc() >= 0) {
               printf("%04X", z80_get_pc());
            } else {
               printf("????");
            }
            colon = 1;
         }
         if (arguments.show_hex) {
            if (colon) {
               printf(" : ");
            }
            for (int i = 0; i < MAX_INSTR_LEN; i++) {
               if (i < instr_len) {
                  printf("%02X ", instr_bytes[i]);
               } else {
                  printf("   ");
               }
            }
            colon = 1;
         }
         if (arguments.show_instruction) {
            if (colon) {
               printf(" : ");
            }
            switch (format) {
            case TYPE_1:
               count = printf(mnemonic, arg_reg);
               break;
            case TYPE_2:
               count = printf(mnemonic, arg_reg, arg_reg);
               break;
            case TYPE_3:
               count = printf(mnemonic, arg_imm, arg_reg);
               break;
            case TYPE_4:
               count = printf(mnemonic, arg_reg, arg_imm);
               break;
            case TYPE_5:
               count = printf(mnemonic, arg_reg, arg_dis);
               break;
            case TYPE_6:
               count = printf(mnemonic, arg_reg, arg_dis, arg_imm);
               break;
            case TYPE_7:
               if (z80_get_pc() >= 0) {
                  sprintf(target, "%04Xh", (z80_get_pc() + instr_len + arg_dis) & 0xffff);
               } else {
                  sprintf(target, "$%+d", arg_dis + instr_len);
               }
               count = printf(mnemonic, target);
               break;
            case TYPE_8:
               count = printf(mnemonic, arg_imm);
               break;
            default:
               count = printf(mnemonic, 0);
               break;
            }
            // Pad the disassembled instruction
            if (arguments.show_cycles || arguments.show_state) {
               while (count++ < 20) {
                  printf(" ");
               }
            }
            colon = 1;
         }
         if (arguments.show_cycles) {
            if (colon) {
               printf(" : ");
            }
            printf("%2d/%2d", instr_cycles, wait_cycles);
            colon = 1;
         }
         if (do_emulate) {
            // Run the emulation
            failflag = FAIL_NONE;
            if (instruction && instruction->emulate) {
               instruction->emulate(instruction);
            }
         }
         if (arguments.show_state || failflag) {
            if (colon) {
               printf(" : ");
            }
            // Show the state after executing this instruction
            printf("%s", z80_get_state(arguments.show_state));
            if (failflag > FAIL_NONE) {
               if (failflag == FAIL_NOT_IMPLEMENTED) {
                  printf(" : not implemented");
               } else if (failflag == FAIL_IMPLEMENTATION_ERROR) {
                  printf(" : implementation error");
               } else {
                  printf(" : fail");
               }
            }
            colon = 1;
         }
         if (colon) {
            printf("\n");
            if (arguments.debug > 0) {
               printf("\n");
            }
         }

         // Reset the instruction variables
         instr_cycles = 0;
         wait_cycles = 0;
      }

   } while (ret & BIT_UNPROCESSED);

}

void lookahead_decode_cycle(Z80CycleSummaryType *cycle_summary) {
   static Z80CycleSummaryType cycle_q[DEPTH];
   static int fill = 0;
   memcpy(cycle_q + fill, cycle_summary, sizeof(Z80CycleSummaryType));
   if (fill < DEPTH - 1) {
      fill++;
   } else {
      decode_cycle(cycle_q);
      memmove(cycle_q, cycle_q + 1, sizeof(Z80CycleSummaryType) * (DEPTH - 1));
   }
}


void decode_sample(int sample) {
   static Z80CycleType prev_cycle    = C_NONE;
   static int sample_index           = 0;
   static int prev_data              = 0;
   static int prev_phi               = 0;
   static int prev_wait              = 0;
   static Z80CycleSummaryType cycle_summary;

   int wait = (sample >> arguments.idx_wait) & 1;
   int phi  = (sample >> arguments.idx_phi ) & 1;
   int data = (sample >> arguments.idx_data) & 255;

   // Determine the cycle type
   Z80CycleType cycle = get_cycle_type(sample);
   int cycle_start    = (cycle != prev_cycle && cycle != C_NONE);
   int cycle_end      = (cycle != prev_cycle && cycle == C_NONE);

   // Store the sample
   sample_buffer[sample_index] = sample;

   if (cycle != prev_cycle && cycle != C_NONE && prev_cycle != C_NONE) {
      printf("WARNING: unexpected transition from %s to %s\n",
             cycle_names[prev_cycle],
             cycle_names[cycle]);
   }

   // Increment cycles counts on the falling edge of Phi, where wait is accurate
   cycle_summary.num_samples++;
   if (arguments.idx_phi < 0 || (prev_phi && !phi)) {
      cycle_summary.instr_cycles++;
      if (prev_wait == 0) {
         cycle_summary.wait_cycles++;
      }
   }

   // At the end of a cycle, latch the cycle type and data
   if (cycle_end) {
      cycle_summary.cycle = prev_cycle;
      cycle_summary.data  = prev_data;
   }

   // At the beginning of the next cycle pass this on to the decoder, so the cycle count is correct
   if (cycle_start) {
      lookahead_decode_cycle(&cycle_summary);
      cycle_summary.num_samples = 0;
      cycle_summary.instr_cycles = 0;
      cycle_summary.wait_cycles  = 0;
      cycle_summary.sample_index = sample_index;
   }

   prev_cycle   = cycle;
   prev_wait    = wait;
   prev_phi     = phi;
   prev_data    = data;
   sample_index = (sample_index + 1) & (SAMPLE_BUFSIZE - 1);

}



// ====================================================================
// Input file processing and bus cycle extraction
// ====================================================================

void decode(FILE *stream) {

   int num;
   uint16_t sample;

   z80_init(arguments.cpu);

   while ((num = fread(buffer, sizeof(uint16_t), READ_BUFSIZE, stream)) > 0) {

      uint16_t *sampleptr = &buffer[0];

      while (num-- > 0) {

         sample = *sampleptr++;

         decode_sample(sample);

      }
   }

   // Flush the lookhead decoder with NOPs
   for (int i = 0; i < DEPTH - 1; i++) {
      Z80CycleSummaryType dummy;
      dummy.cycle = C_FETCH;
      dummy.data = 0;
      dummy.instr_cycles = 4;
      dummy.wait_cycles  = 0;
      dummy.sample_index = 0; // TOOD
      lookahead_decode_cycle(&dummy);
   }

}

// ====================================================================
// Main program entry point
// ====================================================================

int dump_counts(int prefix) {
   int total = 0;
   InstrType *instruction = table_by_prefix(prefix);
   for (int i = 0; i < 256; i++) {
      fprintf(stderr, "%02x %02x %10d %s\n",
             prefix, i, instruction[i].count, instruction[i].mnemonic);
      total += instruction[i].count;
   }
   return total;
}

int main(int argc, char *argv[]) {
   arguments.idx_data         =  0;
   arguments.idx_m1           =  8;
   arguments.idx_rd           =  9;
   arguments.idx_wr           = 10;
   arguments.idx_mreq         = 11;
   arguments.idx_iorq         = 12;
   arguments.idx_wait         = 13;
   arguments.idx_rst          = 14;
   arguments.idx_phi          = 15;
   arguments.filename         = NULL;
   arguments.show_address     = 0;
   arguments.show_hex         = 0;
   arguments.show_instruction = 0;
   arguments.show_state       = 0;
   arguments.show_cycles      = 0;
   arguments.cpu              = CPU_DEFAULT;
   arguments.debug            = 0;

   argp_parse(&argp, argc, argv, 0, 0, &arguments);

   if (arguments.show_address || arguments.show_state) {
      do_emulate = 1;
   }

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

#ifdef DUMP_COVERAGE
   int total = 0;
   total += dump_counts(0x00);
   total += dump_counts(0xCB);
   total += dump_counts(0xDD);
   total += dump_counts(0xED);
   total += dump_counts(0xFD);
   total += dump_counts(0xDDCB);
   total += dump_counts(0xFDCB);
   fprintf(stderr, "Total = %d\n", total);
#endif

   return 0;
}
