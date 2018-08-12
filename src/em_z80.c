//
// This file is based on part of the libsigrokdecode project.
//
// Copyright (C) 2014 Daniel Elstner <daniel.kitta@gmail.com>
//

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "em_z80.h"

#define UNDEFINED1 {0, 0, 0, 0, False, TYPE_0, "???", op_nop}

#define UNDEFINED2 {-1, -1, -1, -1, False, TYPE_0, "???", NULL}

// #define DEBUG_SCF_CCF

#ifdef DEBUG_SCF_CCF
int tmp_a;
int tmp_f5;
int tmp_f3;
int tmp_q;
int tmp_op;
#endif

// The following global variables are available, from the instruction decoder
extern int prefix;
extern int opcode;
extern int arg_dis;
extern int arg_imm;
extern int arg_read;
extern int arg_write;
extern int instr_len;
extern int failflag;

// ===================================================================
// Emulation registers
// ==================================================================

// The CPU type
static int cpu;

// Program counter
static int reg_pc;

// Stack pointer
static int reg_sp;

// Normal Flags
static int flag_s;
static int flag_z;
static int flag_f5;
static int flag_h;
static int flag_f3;
static int flag_pv;
static int flag_n;
static int flag_c;

// Alternative Flags
static int alt_flag_s;
static int alt_flag_z;
static int alt_flag_f5;
static int alt_flag_h;
static int alt_flag_f3;
static int alt_flag_pv;
static int alt_flag_n;
static int alt_flag_c;

// Normal registers
static int reg_a;
static int reg_b;
static int reg_c;
static int reg_d;
static int reg_e;
static int reg_h;
static int reg_l;

// Alternative registers
static int alt_reg_a;
static int alt_reg_b;
static int alt_reg_c;
static int alt_reg_d;
static int alt_reg_e;
static int alt_reg_h;
static int alt_reg_l;

// Index registers
static int reg_ixl;
static int reg_ixh;
static int reg_iyl;
static int reg_iyh;

// Other
static int reg_ir;
static int reg_iff1;
static int reg_iff2;
static int reg_im;
static int reg_i;
static int reg_r;
static int reg_memptr;
static int reg_q;
static int halted;

#define IM_MODE_0    0
#define IM_MODE_1    1
#define IM_MODE_2    2

// Pointers to the registers,

#define ID_MEMORY 6

#define ID_RR_BC 0
#define ID_RR_DE 1
#define ID_RR_HL 2
#define ID_RR_SP 3 // For "pair1" functions
#define ID_RR_AF 3 // For "pair2" functions
#define ID_RR_IX 4
#define ID_RR_IY 5

#define ID_R_B    0
#define ID_R_C    1
#define ID_R_D    2
#define ID_R_E    3
#define ID_R_H    4
#define ID_R_L    5
#define ID_R_ARG  6
#define ID_R_A    7
#define ID_R_IXH  8
#define ID_R_IXL  9
#define ID_R_IYH 10
#define ID_R_IYL 11

int *reg_ptr[] = {
   &reg_b,
   &reg_c,
   &reg_d,
   &reg_e,
   &reg_h,
   &reg_l,
   &arg_read,
   &reg_a,
   &reg_ixh,
   &reg_ixl,
   &reg_iyh,
   &reg_iyl
};

// ===================================================================
// Emulation output
// ===================================================================

static char buffer[1024];

static const char default_state[] = "A=?? F=???????? BC=???? DE=???? HL=???? IX=???? IY=???? SP=????";
static const char full_state[]    = "A=?? F=???????? BC=???? DE=???? HL=???? IX=???? IY=???? SP=???? : IR=???? IF1=? IF2=? IM=? MZ=????";

#define OFFSET_A    2
#define OFFSET_F    7
#define OFFSET_B   19
#define OFFSET_C   21
#define OFFSET_D   27
#define OFFSET_E   29
#define OFFSET_H   35
#define OFFSET_L   37
#define OFFSET_IX  43
#define OFFSET_IY  51
#define OFFSET_SP  59

#define OFFSET_IR  69
#define OFFSET_IF1 78
#define OFFSET_IF2 84
#define OFFSET_IM  89
#define OFFSET_MZ  94

static void write_flag(char *buffer, int flag, int value) {
   *buffer = value ? flag : ' ';
}

static void write_hex1(char *buffer, int value) {
   *buffer = value + (value < 10 ? '0' : 'A' - 10);
}

static void write_hex2(char *buffer, int value) {
   write_hex1(buffer++, (value >> 4) & 15);
   write_hex1(buffer++, (value >> 0) & 15);
}

static void write_hex4(char *buffer, int value) {
   write_hex1(buffer++, (value >> 12) & 15);
   write_hex1(buffer++, (value >> 8) & 15);
   write_hex1(buffer++, (value >> 4) & 15);
   write_hex1(buffer++, (value >> 0) & 15);
}

char *z80_get_state(int verbosity) {
   strcpy(buffer, verbosity > 1 ? full_state : default_state);
   if (reg_a >= 0) {
      write_hex2(buffer + OFFSET_A, reg_a);
   }
   if (flag_s >= 0) {
      write_flag(buffer + OFFSET_F + 0, 'S', flag_s);
   }
   if (flag_z >= 0) {
      write_flag(buffer + OFFSET_F + 1, 'Z', flag_z);
   }
   if (flag_f5 >= 0) {
      write_hex1(buffer + OFFSET_F + 2, flag_f5);
   }
   if (flag_h >= 0) {
      write_flag(buffer + OFFSET_F + 3, 'H', flag_h);
   }
   if (flag_f3 >= 0) {
      write_hex1(buffer + OFFSET_F + 4, flag_f3);
   }
   if (flag_pv >= 0) {
      write_flag(buffer + OFFSET_F + 5, 'V', flag_pv);
   }
   if (flag_n >= 0) {
      write_flag(buffer + OFFSET_F + 6, 'N', flag_n);
   }
   if (flag_c >= 0) {
      write_flag(buffer + OFFSET_F + 7, 'C', flag_c);
   }
   if (reg_b >= 0) {
      write_hex2(buffer + OFFSET_B, reg_b);
   }
   if (reg_c >= 0) {
      write_hex2(buffer + OFFSET_C, reg_c);
   }
   if (reg_d >= 0) {
      write_hex2(buffer + OFFSET_D, reg_d);
   }
   if (reg_e >= 0) {
      write_hex2(buffer + OFFSET_E, reg_e);
   }
   if (reg_h >= 0) {
      write_hex2(buffer + OFFSET_H, reg_h);
   }
   if (reg_l >= 0) {
      write_hex2(buffer + OFFSET_L, reg_l);
   }
   if (reg_ixh >= 0) {
      write_hex2(buffer + OFFSET_IX, reg_ixh);
   }
   if (reg_ixl >= 0) {
      write_hex2(buffer + OFFSET_IX + 2, reg_ixl);
   }
   if (reg_iyh >= 0) {
      write_hex2(buffer + OFFSET_IY, reg_iyh);
   }
   if (reg_iyl >= 0) {
      write_hex2(buffer + OFFSET_IY + 2, reg_iyl);
   }
   if (reg_sp >= 0) {
      write_hex4(buffer + OFFSET_SP, reg_sp);
   }
   if (verbosity > 1) {
      if (reg_i >= 0) {
         write_hex2(buffer + OFFSET_IR, reg_i);
      }
      if (reg_r >= 0) {
         write_hex2(buffer + OFFSET_IR + 2, reg_r);
      }
      if (reg_iff1 >= 0) {
         write_hex1(buffer + OFFSET_IF1, reg_iff1);
      }
      if (reg_iff2 >= 0) {
         write_hex1(buffer + OFFSET_IF2, reg_iff2);
      }
      if (reg_im >= 0) {
         write_hex1(buffer + OFFSET_IM, reg_im);
      }
      if (reg_memptr >= 0) {
         write_hex4(buffer + OFFSET_MZ, reg_memptr);
      }
   }
   return buffer;
}

int z80_get_pc() {
   return reg_pc;
}

// ===================================================================
// Emulation reset / interrupt
// ===================================================================

void z80_init(int cpu_type) {
   cpu = cpu_type;
   // Defined on reset
   reg_pc      = -1;
   reg_sp      = -1;
   reg_a       = -1;
   flag_s      = -1;
   flag_z      = -1;
   flag_f5     = -1;
   flag_h      = -1;
   flag_f3     = -1;
   flag_pv     = -1;
   flag_n      = -1;
   flag_c      = -1;
   reg_ir      = -1;
   reg_iff1    = -1;
   reg_iff2    = -1;
   reg_im      = -1;
   reg_i       = -1;
   reg_r       = -1;
   // Undefined on reset
   reg_b       = -1;
   reg_c       = -1;
   reg_d       = -1;
   reg_e       = -1;
   reg_h       = -1;
   reg_l       = -1;
   alt_reg_a   = -1;
   alt_flag_s  = -1;
   alt_flag_z  = -1;
   alt_flag_f5 = -1;
   alt_flag_h  = -1;
   alt_flag_f3 = -1;
   alt_flag_pv = -1;
   alt_flag_n  = -1;
   alt_flag_c  = -1;
   alt_reg_b   = -1;
   alt_reg_c   = -1;
   alt_reg_d   = -1;
   alt_reg_e   = -1;
   alt_reg_h   = -1;
   alt_reg_l   = -1;
   reg_ixh     = -1;
   reg_ixl     = -1;
   reg_iyh     = -1;
   reg_iyl     = -1;
   reg_memptr  = -1;
   reg_q       = -1;
   halted      =  0;
}

void z80_reset() {
   // Defined on reset
   reg_pc      = 0;
   reg_sp      = 0xFFFF;
   reg_a       = 0xFF;
   flag_s      = 1;
   flag_z      = 1;
   flag_f5     = 1;
   flag_h      = 1;
   flag_f3     = 1;
   flag_pv     = 1;
   flag_n      = 1;
   flag_c      = 1;
   reg_ir      = 0;
   reg_iff1    = 0;
   reg_iff2    = 0;
   reg_im      = 0;
   reg_i       = 0;
   reg_r       = 0;
   // Undefined on reset
   reg_b       = -1;
   reg_c       = -1;
   reg_d       = -1;
   reg_e       = -1;
   reg_h       = -1;
   reg_l       = -1;
   alt_reg_a   = -1;
   alt_flag_s  = -1;
   alt_flag_z  = -1;
   alt_flag_f5 = -1;
   alt_flag_h  = -1;
   alt_flag_f3 = -1;
   alt_flag_pv = -1;
   alt_flag_n  = -1;
   alt_flag_c  = -1;
   alt_reg_b   = -1;
   alt_reg_c   = -1;
   alt_reg_d   = -1;
   alt_reg_e   = -1;
   alt_reg_h   = -1;
   alt_reg_l   = -1;
   reg_ixh     = -1;
   reg_ixl     = -1;
   reg_iyh     = -1;
   reg_iyl     = -1;
   reg_memptr  = -1;
   reg_q       = -1;
   halted      =  0;
}

void z80_increment_r() {
   if (reg_r >= 0) {
      reg_r = (reg_r & 0x80) | ((reg_r + 1) & 0x7f);
   }
}

// ===================================================================
// Emulation helper
// ===================================================================

static int get_r_id(int id) {
   // If the prefix is 0xDD, references to h/l are replaced by ixh/ixl
   if (prefix == 0xdd) {
      if (id == ID_R_H) {
         id = ID_R_IXH;
      }
      if (id == ID_R_L) {
         id = ID_R_IXL;
      }
   }
   // If the prefix is 0xFD, references to h/l are replaces bd iyh/iyl
   if (prefix == 0xfd) {
      if (id == ID_R_H) {
         id = ID_R_IYH;
      }
      if (id == ID_R_L) {
         id = ID_R_IYL;
      }
   }
   return id;
}

static int get_rr_id() {
   // Returns:
   // Prefix = 0xDD   => IX
   // Prefix = 0xFD   => IY
   // Opcode[5:4] = 0 => BC
   // Opcode[5:4] = 1 => DE
   // Opcode[5:4] = 2 => SP
   // Opcode[5:4] = 3 => HL
   int id = (opcode >> 4) & 3;
   if (id == ID_RR_HL) {
      // If the prefix is 0xDD, references to hl are replaced by ix
      if (prefix == 0xdd) {
         id = ID_RR_IX;
      }
      // If the prefix is 0xFD, references to hl are replaced by iy
      if (prefix == 0xfd) {
         id = ID_RR_IY;
      }
   }
   return id;
}

static int get_hl_or_idx_id() {
   // Prefix = 0xDD   => IX
   // Prefix = 0xFD   => IY
   // Otherwise       => HL
   return (prefix == 0xdd) ? ID_RR_IX : (prefix == 0xfd) ? ID_RR_IY : ID_RR_HL;
}

static const unsigned char partab[256] = {
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
};

static void set_sign_zero(int result) {
   flag_s  = (result >> 7) & 1;
   flag_z  = ((result & 0xff) == 0);
   flag_f5 = (result >> 5) & 1;
   flag_f3 = (result >> 3) & 1;
}

static void set_sign_zero_16(int result) {
   flag_s  = (result >> 15) & 1;
   flag_z  = ((result & 0xffff) == 0);
   flag_f5 = (result >> 13) & 1;
   flag_f3 = (result >> 11) & 1;
}

static void set_sign_zero2(int result, int operand) {
   flag_s  = (result >> 7) & 1;
   flag_z  = ((result & 0xff) == 0);
   flag_f5 = (operand >> 5) & 1;
   flag_f3 = (operand >> 3) & 1;
}

static void set_sign_zero_undefined() {
   flag_s  = -1;
   flag_z  = -1;
   flag_f5 = -1;
   flag_f3 = -1;
}

static void set_flags_undefined() {
   flag_s  = -1;
   flag_z  = -1;
   flag_f5 = -1;
   flag_h  = -1;
   flag_f3 = -1;
   flag_pv = -1;
   flag_n  = -1;
   flag_c  = -1;
}

static int read_reg_pair_helper(int id, int type) {
   // Cases 4 and 5 are used for 0xDD and 0xFD prefixed operations
   switch(id) {
   case 0:
      if (reg_b >= 0 && reg_c >= 0) {
         return (reg_b << 8) | reg_c;
      }
      break;
   case 1:
      if (reg_d >= 0 && reg_e >= 0) {
         return (reg_d << 8) | reg_e;
      }
      break;
   case 2:
      if (reg_h >= 0 && reg_l >= 0) {
         return (reg_h << 8) | reg_l;
      }
      break;
   case 3:
      if (type == 1) {
         return reg_sp;
      } else {
         if (reg_a >= 0 && flag_s >= 0 && flag_z >= 0 && flag_f5 >= 0 && flag_h >= 0 && flag_f3 >= 0 && flag_pv >= 0 && flag_n >= 0 && flag_c >= 0) {
            return (reg_a << 8) | (flag_s << 7) | (flag_z << 6) | (flag_f5 << 5) | (flag_h << 4) | (flag_f3 << 3) | (flag_pv << 2) | (flag_n << 1) | flag_c;
         }
      }
      break;
   case 4:
      if (reg_ixh >= 0 && reg_ixl >= 0) {
         return (reg_ixh << 8) | reg_ixl;
      }
      break;
   case 5:
      if (reg_iyh >= 0 && reg_iyl >= 0) {
         return (reg_iyh << 8) | reg_iyl;
      }
      break;
   }
   return -1;
}

static void write_reg_pair_helper(int id, int value, int type) {
   // Cases 4 and 5 are used for 0xDD and 0xFD prefixed operations
   switch(id) {
   case 0:
      if (value >= 0) {
         reg_b = (value >> 8) & 0xff;
         reg_c = value & 0xff;
      } else {
         reg_b = -1;
         reg_c = -1;
      }
      break;
   case 1:
      if (value >= 0) {
         reg_d = (value >> 8) & 0xff;
         reg_e = value & 0xff;
      } else {
         reg_d = -1;
         reg_e = -1;
      }
      break;
   case 2:
      if (value >= 0) {
         reg_h = (value >> 8) & 0xff;
         reg_l = value & 0xff;
      } else {
         reg_h = -1;
         reg_l = -1;
      }
      break;
   case 3:
      if (type == 1) {
         reg_sp = value;
      } else {
         if (value >= 0) {
            reg_a   = (value >> 8) & 0xff;
            flag_s  = (value >> 7) & 1;
            flag_z  = (value >> 6) & 1;
            flag_f5 = (value >> 5) & 1;
            flag_h  = (value >> 4) & 1;
            flag_f3 = (value >> 3) & 1;
            flag_pv = (value >> 2) & 1;
            flag_n  = (value >> 1) & 1;
            flag_c  = value        & 1;
         } else {
            reg_a = -1;
            set_flags_undefined();
         }
      }
      break;
   case 4:
      if (value >= 0) {
         reg_ixh = (value >> 8) & 0xff;
         reg_ixl = value & 0xff;
      } else {
         reg_ixh = -1;
         reg_ixl = -1;
      }
      break;
   case 5:
      if (value >= 0) {
         reg_iyh = (value >> 8) & 0xff;
         reg_iyl = value & 0xff;
      } else {
         reg_iyh = -1;
         reg_iyl = -1;
      }
      break;
   }
}

static int read_reg_pair1(int id) {
   return read_reg_pair_helper(id, 1);
}

static int read_reg_pair2(int id) {
   return read_reg_pair_helper(id, 2);
}

static void write_reg_pair1(int id, int value) {
   write_reg_pair_helper(id, value, 1);
}

static void write_reg_pair2(int id, int value) {
   write_reg_pair_helper(id, value, 2);
}

static void swap(int *reg1, int *reg2) {
   int tmp;
   tmp = *reg1;
   *reg1 = *reg2;
   *reg2 = tmp;
}

static void update_pc() {
   if (reg_pc >= 0) {
      reg_pc = (reg_pc + instr_len) & 0xffff;
   }
}

static void update_memptr(int addr) {
   reg_memptr = addr;
}

static void update_memptr_inc(int addr) {
   if (addr >= 0) {
      reg_memptr = (addr + 1) & 0xffff;
   } else {
      reg_memptr = -1;
   }
}

static void update_memptr_dec(int addr) {
   if (addr >= 0) {
      reg_memptr = (addr - 1) & 0xffff;
   } else {
      reg_memptr = -1;
   }
}

static void update_memptr_inc_split(int hi, int lo) {
   if (lo >= 0 && hi >= 0) {
      reg_memptr = (hi << 8) | ((lo + 1) & 0xff);
   } else {
      reg_memptr = -1;
   }
}

static void update_memptr_idx_disp() {
   if (prefix == 0xdd || prefix == 0xfd || prefix == 0xddcb || prefix == 0xfdcb) {
      int idx = read_reg_pair1((prefix == 0xfd || prefix == 0xfdcb) ? ID_RR_IY : ID_RR_IX);
      if (idx >= 0) {
         reg_memptr = (idx + arg_dis) & 0xffff;
      } else {
         reg_memptr = -1;
      }
   } else {
      failflag = FAIL_IMPLEMENTATION_ERROR;
   }
}

static inline void flags_updated() {
   reg_q = 1;
}

static inline void flags_not_updated() {
   reg_q = 0;
}

// ===================================================================
// Emulated instructions - HALT/NOP/INT/NMI
// ===================================================================

int z80_halted() {
   return halted;
}

static void op_halt(InstrType *instr) {
   update_pc();
   halted = 1;
   // Update undocumented Q register
   flags_not_updated();
}

static void op_nop(InstrType *instr) {
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_interrupt_nmi(InstrType *instr) {
   // Clear halted
   halted = 0;
   if (reg_pc >= 0 && reg_pc != arg_write) {
      failflag = FAIL_ERROR;
   }
   reg_pc = 0x0066;
   reg_iff1 = 0;
   if (reg_sp >= 0) {
      reg_sp = (reg_sp - 2) & 0xffff;
   }
   // Update undocumented memptr register
   update_memptr(reg_pc);
   // Update undocumented Q register
   flags_not_updated();
}

static void op_interrupt_int(InstrType *instr) {
   // Clear halted
   halted = 0;
   // Disable interrupts
   reg_iff1 = 0;
   reg_iff2 = 0;
   // Determine the interrupt mode, but fall back to IM1 if reg_im is undefined
   // (this may well be the case if the capture started after the system booted)
   // TODO: this fallback should be a command line option
   int mode = (reg_im >= 0) ? reg_im : IM_MODE_1;
   if (mode == 0 && ((opcode & 0xC7) != 0xC7)) {
      // In interrput mode 0 we only implement the case where the opcode is RST
      failflag = FAIL_NOT_IMPLEMENTED;
      reg_pc = -1;
      reg_sp = -1;
   } else {
      // Validate the addess of the interrupted instruction
      if (reg_pc >= 0 && reg_pc != arg_write) {
         failflag = FAIL_ERROR;
      }
      // That address is pushed onto the stack
      if (reg_sp >= 0) {
         reg_sp = (reg_sp - 2) & 0xffff;
      }
      switch (mode) {
      case IM_MODE_0:
         // In interrupt mode 0 the vector is executed as if it were a single-byte opcode
         reg_pc = opcode & 0x38;
         break;
      case IM_MODE_1:
         // In interrupt mode 1, the vector is ignored and an RST 38 is performed
         reg_pc = 0x0038;
         break;
      case IM_MODE_2:
         // In interrupt mode 2, the new PC is read from a vector table
         // TODO: we need to extend the instruction decoder to deal with
         // OPCODE-WOP1-WOP2 followed by optional ROP1-ROP2
         // reg_pc = arg_read;
         reg_pc = -1;
      }
   }
   // Update undocumented memptr register
   update_memptr(reg_pc);
   // Update undocumented Q register
   flags_not_updated();
}

// ===================================================================
// Emulated instructions - Push/Pop
// ===================================================================

static void op_push(InstrType *instr) {
   int reg_id = get_rr_id();
   int reg = read_reg_pair2(reg_id);
#ifdef DEBUG_SCF_CCF
   // 0xF5 = PUSH AF
   if (tmp_op >= 0 && opcode == 0xf5 && flag_f5 >= 0 && flag_f3 >= 0) {
      printf("\n");
      printf("%s old: %d %d; a=", (tmp_op ? "SCF" : "CCF"), tmp_f5, tmp_f3);
      for (int i = 7; i >= 0; i--) {
         printf("%d", (tmp_a >> i) & 1);
      }
      printf("; q=%d", tmp_q);
      printf("; exp: %d %d", flag_f5, flag_f3);
      printf("; act: %d %d", (arg_write >> 5) & 1, (arg_write >> 3) & 1);
      if ((((arg_write >> 5) & 1) == flag_f5) && (((arg_write >> 3) & 1) == flag_f3)) {
         printf( " xxx pass\n");
      } else {
         printf( " xxx fail\n");

      }
      tmp_op = -1;
   }
#endif
   if (reg >= 0 && reg != arg_write) {
      failflag = FAIL_ERROR;
   }
   write_reg_pair2(reg_id, arg_write);
   if (reg_sp >= 0) {
      reg_sp = (reg_sp - 2) & 0xffff;
   }
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_pop(InstrType *instr) {
   int reg_id = get_rr_id();
   write_reg_pair2(reg_id, arg_read);
   if (reg_sp >= 0) {
      reg_sp = (reg_sp + 2) & 0xffff;
   }
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

// ===================================================================
// Emulated instructions - Control Flow
// ===================================================================

static int test_cc(int cc) {
   // default to unknown
   int taken = -1;
   switch (cc) {
   case 0:
      // NZ
      if (flag_z >= 0) {
         taken = !flag_z;
      }
      break;
   case 1:
      // Z
      if (flag_z >= 0) {
         taken = flag_z;
      }
      break;
   case 2:
      // NC
      if (flag_c >= 0) {
         taken = !flag_c;
      }
      break;
   case 3:
      // C
      if (flag_c >= 0) {
         taken = flag_c;
      }
      break;
   case 4:
      // PO
      if (flag_pv >= 0) {
         taken = !flag_pv;
      }
      break;
   case 5:
      // PE
      if (flag_pv >= 0) {
         taken = flag_pv;
      }
      break;
   case 6:
      // P
      if (flag_s >= 0) {
         taken = !flag_s;
      }
      break;
   case 7:
      // M
      if (flag_s >= 0) {
         taken = flag_s;
      }
      break;
   }
   return taken;
}

static void op_call(InstrType *instr) {
   update_pc();
   // The stacked PC is the next instuction
   if (reg_pc >= 0 && reg_pc != arg_write) {
      failflag = FAIL_ERROR;
   }
   reg_pc = arg_imm;
   if (reg_sp >= 0) {
      reg_sp = (reg_sp - 2) & 0xffff;
   }
   // Update undocumented memptr register
   update_memptr(arg_imm);
   // Update undocumented Q register
   flags_not_updated();
}

static void op_call_cond(InstrType *instr) {
   int cc = (opcode >> 3) & 7;
   int taken = test_cc(cc);
   if (taken >= 0) {
      if (taken) {
         op_call(instr);
      } else {
         update_pc();
      }
   } else {
      reg_pc = -1;
      reg_sp = -1;
   }
   // Update undocumented memptr register
   update_memptr(arg_imm);
   // Update undocumented Q register
   flags_not_updated();
}

static void op_ret(InstrType *instr) {
   reg_pc = arg_read;
   if (reg_sp >= 0) {
      reg_sp = (reg_sp + 2) & 0xffff;
   }
   // Update undocumented memptr register
   update_memptr(reg_pc);
   // Update undocumented Q register
   flags_not_updated();
}

static void op_retn(InstrType *instr) {
   op_ret(instr);
   reg_iff1 = reg_iff2;
   // Also used for reti, as there is no difference from an emulation perspective
   // Update undocumented Q register
   flags_not_updated();
}

static void op_ret_cond(InstrType *instr) {
   int cc = (opcode >> 3) & 7;
   int taken = test_cc(cc);
   if (taken >= 0) {
      if (taken) {
         op_ret(instr);
      } else {
         update_pc();
      }
   } else {
      reg_pc = -1;
      reg_sp = -1;
      update_memptr(-1);
   }
   // Update undocumented Q register
   flags_not_updated();
}

static void op_jr(InstrType *instr) {
   int cc = (opcode >> 3) & 7;
   int taken = cc < 4 ? 1 : test_cc(cc - 4);
   // TODO: could infer more state from number of cycles
   if (taken >= 0 && reg_pc >= 0) {
      update_pc();
      if (taken) {
         reg_pc = (reg_pc + arg_dis) & 0xffff;
         // Update undocumented memptr register
         update_memptr(reg_pc);
      }
   } else {
      reg_pc = -1;
      // Update undocumented memptr register
      update_memptr(-1);
   }
   // Update undocumented Q register
   flags_not_updated();
}

static void op_jp(InstrType *instr) {
   reg_pc = arg_imm;
   // Update undocumented memptr register
   update_memptr(arg_imm);
   // Update undocumented Q register
   flags_not_updated();
}

static void op_jp_hl(InstrType *instr) {
   int rr_id = get_hl_or_idx_id();
   reg_pc = read_reg_pair1(rr_id);
   // Note: undocumented memptr does not change in this case
   // Update undocumented Q register
   flags_not_updated();
}

static void op_jp_cond(InstrType *instr) {
   int cc = (opcode >> 3) & 7;
   int taken = test_cc(cc);
   // TODO: could infer more state from number of cycles
   if (taken >= 0) {
      if (taken) {
         reg_pc = arg_imm;
      } else {
         update_pc();
      }
   } else {
      reg_pc = -1;
   }
   // Update undocumented memptr register
   update_memptr(arg_imm);
   // Update undocumented Q register
   flags_not_updated();
}

static void op_djnz(InstrType *instr) {
   int taken = -1;
   if (reg_b >= 0) {
      reg_b = (reg_b - 1) & 0xff;
      taken = (reg_b != 0);
   }
   if (taken >= 0 && reg_pc >= 0) {
      update_pc();
      if (taken) {
         reg_pc = (reg_pc + arg_dis) & 0xffff;
         // Update undocumented memptr register
         update_memptr(reg_pc);
      }
   } else {
      reg_pc = -1;
      // Update undocumented memptr register
      update_memptr(-1);
   }
   // Update undocumented Q register
   flags_not_updated();
}

static void op_rst(InstrType *instr) {
   // The stacked PC is the next instuction
   update_pc();
   if (reg_pc >= 0 && reg_pc != arg_write) {
      failflag = FAIL_ERROR;
   }
   reg_pc = opcode & 0x38;
   if (reg_sp >= 0) {
      reg_sp = (reg_sp - 2) & 0xffff;
   }
   // Update undocumented memptr register
   update_memptr(reg_pc);
   // Update undocumented Q register
   flags_not_updated();
}

// ===================================================================
// Emulated instructions - ALU
// ===================================================================

static void op_alu(InstrType *instr) {
   int type    = (opcode >> 6) & 3;
   int alu_op  = (opcode >> 3) & 7;
   int operand;
   int r_id = get_r_id(opcode & 7);
   if (type == 2) {
      // alu[y] r[z]
      operand = *reg_ptr[r_id];
   } else if (type == 3 && r_id == ID_MEMORY) {
      // alu[y] n
      operand = arg_imm;
   } else {
      printf("opcode table error for %02x\n", opcode);
      return;
   }
   int cbits;
   int result;
   int cin = flag_c;
   switch (alu_op) {
   case 0:
      // ADD
      cin = 0;
   case 1:
      // ADC
      if (reg_a >= 0 && operand >= 0 && cin >= 0) {
         result  = reg_a + operand + cin;
         set_sign_zero(result);
         cbits   = reg_a ^ operand ^ result;
         flag_c  = (cbits >> 8) & 1;
         flag_h  = (cbits >> 4) & 1;
         flag_pv = ((cbits >> 8) ^ (cbits >> 7)) & 1;
         reg_a   = result & 0xff;
      } else {
         reg_a   = -1;
         set_flags_undefined();
      }
      flag_n = 0;
      break;
   case 2:
      // SUB
      cin = 0;
   case 3:
      // SBC
      if (reg_a >= 0 && operand >= 0 && cin >= 0) {
         result  = reg_a - operand - cin;
         set_sign_zero(result);
         cbits   = reg_a ^ operand ^ result;
         flag_c  = (cbits >> 8) & 1;
         flag_h  = (cbits >> 4) & 1;
         flag_pv = ((cbits >> 8) ^ (cbits >> 7)) & 1;
         reg_a   = result & 0xff;
      } else {
         reg_a   = -1;
         set_flags_undefined();
      }
      flag_n = 1;
      break;
   case 4:
      // AND
      if (reg_a >= 0 && operand >= 0) {
         reg_a &= operand;
         set_sign_zero(reg_a);
         flag_pv = partab[reg_a];
      } else {
         reg_a = -1;
         set_flags_undefined();
         flag_pv = -1;
      }
      flag_c = 0;
      flag_n = 0;
      flag_h = 1;
      break;
   case 5:
      // XOR
      if (reg_a >= 0 && operand >= 0) {
         reg_a ^= operand;
         set_sign_zero(reg_a);
         flag_pv = partab[reg_a];
      } else {
         reg_a = -1;
         set_flags_undefined();
         flag_pv = -1;
      }
      flag_c = 0;
      flag_n = 0;
      flag_h = 0;
      break;
   case 6:
      // OR
      if (reg_a >= 0 && operand >= 0) {
         reg_a |= operand;
         set_sign_zero(reg_a);
         flag_pv = partab[reg_a];
      } else {
         reg_a = -1;
         set_flags_undefined();
      }
      flag_c = 0;
      flag_n = 0;
      flag_h = 0;
      break;
   case 7:
      // CP
      if (reg_a >= 0 && operand >= 0) {
         result  = reg_a - operand;
         set_sign_zero2(result, operand);
         cbits   = reg_a ^ operand ^ result;
         flag_c  = (cbits >> 8) & 1;
         flag_h  = (cbits >> 4) & 1;
         flag_pv = ((cbits >> 8) ^ (cbits >> 7)) & 1;
      } else {
         reg_a   = -1;
         set_flags_undefined();
      }
      flag_n = 1;
      break;
   }
   update_pc();
   // Update undocumented memptr register if (ix+disp) addressing used
   if ((prefix == 0xdd || prefix == 0xfd) && r_id == ID_MEMORY) {
      update_memptr_idx_disp();
   }
   // Update undocumented Q register
   flags_updated();
}

static void op_neg(InstrType *instr) {
   int result;
   int cbits;
   if (reg_a >= 0) {
      result  = -reg_a;
      set_sign_zero(result);
      cbits   = reg_a ^ result;
      flag_c  = (cbits >> 8) & 1;
      flag_h  = (cbits >> 4) & 1;
      flag_pv = ((cbits >> 8) ^ (cbits >> 7)) & 1;
      reg_a   = result & 0xff;
   } else {
      set_flags_undefined();
   }
   flag_n = 1;
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void op_adc_hl_rr(InstrType *instr) {
   int reg_id = get_rr_id();
   // This only appears in the ED block, hence uses just hl as the destination
   int dst_id = ID_RR_HL;
   int op1 = read_reg_pair1(dst_id);
   int op2 = read_reg_pair1(reg_id);
   if (op1 < 0 || op2 < 0 || flag_c < 0) {
      write_reg_pair1(dst_id, -1);
      set_flags_undefined();
   } else {
      int result = op1 + op2 + flag_c;
      int cbits = result ^ op1 ^ op2;
      set_sign_zero_16(result);
      flag_c = (cbits >> 16) & 1;
      flag_h = (cbits >> 12) & 1;
      flag_pv = ((cbits >> 16) ^ (cbits >> 15)) & 1;
      write_reg_pair1(dst_id, result & 0xffff);
   }
   flag_n = 0;
   // Update undocumented memptr register
   update_memptr_inc(op1);
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void op_sbc_hl_rr(InstrType *instr) {
   int reg_id = get_rr_id();
   // This only appears in the ED block, hence uses just hl as the destination
   int dst_id = ID_RR_HL;
   int op1 = read_reg_pair1(dst_id);
   int op2 = read_reg_pair1(reg_id);
   if (op1 < 0 || op2 < 0 || flag_c < 0) {
      write_reg_pair1(dst_id, -1);
      set_flags_undefined();
   } else {
      int result = op1 - op2 - flag_c;
      int cbits = result ^ op1 ^ op2;
      set_sign_zero_16(result);
      flag_c = (cbits >> 16) & 1;
      flag_h = (cbits >> 12) & 1;
      flag_pv = ((cbits >> 16) ^ (cbits >> 15)) & 1;
      write_reg_pair1(dst_id, result & 0xffff);
   }
   flag_n = 1;
   // Update undocumented memptr register
   update_memptr_inc(op1);
   update_pc();
   // Update undocumented Q register
   flags_updated();
}


static void op_add_hl_rr(InstrType *instr) {
   int reg_id = get_rr_id();
   // This appears in the unprefixed and DD/FD blocks, so the destination can be hl or ix/iy
   int dst_id = get_hl_or_idx_id();
   int op1 = read_reg_pair1(dst_id);
   int op2 = read_reg_pair1(reg_id);
   if (op1 < 0 || op2 < 0) {
      write_reg_pair1(dst_id, -1);
      set_flags_undefined();
   } else {
      int result = op1 + op2;
      int cbits = result ^ op1 ^ op2;
      flag_c = (cbits >> 16) & 1;
      flag_h = (cbits >> 12) & 1;
      flag_f5 = (result >> 13) & 1;
      flag_f3 = (result >> 11) & 1;
      write_reg_pair1(dst_id, result & 0xffff);
   }
   flag_n = 0;
   // Update undocumented memptr register
   update_memptr_inc(op1);
   update_pc();
   // Update undocumented Q register
   flags_updated();
}


static void op_inc_r(InstrType *instr) {
   int reg_id = get_r_id((opcode >> 3) & 7);
   int *reg = reg_ptr[reg_id];
   if (*reg >= 0) {
      int result = ((*reg) + 1) & 0xff;
      set_sign_zero(result);
      flag_h  = (result & 0x0f) == 0;
      flag_pv = (result == 0x80);
      if (reg_id == ID_MEMORY) {
         if (arg_write != result) {
            failflag = FAIL_ERROR;
         }
      } else {
         *reg = result;
      }
   } else {
      set_sign_zero_undefined();
      flag_h = -1;
      flag_pv = -1;
   }
   flag_n  = 0;
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void op_inc_rr(InstrType *instr) {
   int reg_id = get_rr_id();
   int val = read_reg_pair1(reg_id);
   if (val >= 0) {
      write_reg_pair1(reg_id, (val + 1) & 0xffff);
   } else {
      write_reg_pair1(reg_id, -1);
   }
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_inc_idx_disp(InstrType *instr) {
   int result = (arg_read + 1) & 0xff;
   set_sign_zero(result);
   flag_h  = (result & 0x0f) == 0;
   flag_pv = (result == 0x80);
   flag_n  = 0;
   if (arg_write != result) {
      failflag = FAIL_ERROR;
   }
   update_pc();
   // Update undocumented memptr register
   update_memptr_idx_disp();
   // Update undocumented Q register
   flags_updated();
}

static void op_dec_r(InstrType *instr) {
   int reg_id = get_r_id((opcode >> 3) & 7);
   int *reg = reg_ptr[reg_id];
   if (*reg >= 0) {
      int result = ((*reg) - 1) & 0xff;
      set_sign_zero(result);
      flag_h  = (result & 0x0f) == 0x0f;
      flag_pv = (result == 0x7f);
      if (reg_id == ID_MEMORY) {
         if (arg_write != result) {
            failflag = FAIL_ERROR;
         }
      } else {
         *reg = result;
      }
   } else {
      set_sign_zero_undefined();
      flag_h = -1;
      flag_pv = -1;
   }
   flag_n  = 1;
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void op_dec_rr(InstrType *instr) {
   int reg_id = get_rr_id();
   int val = read_reg_pair1(reg_id);
   if (val >= 0) {
      write_reg_pair1(reg_id, (val - 1) & 0xffff);
   } else {
      write_reg_pair1(reg_id, -1);
   }
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_dec_idx_disp(InstrType *instr) {
   int result = (arg_read - 1) & 0xff;
   set_sign_zero(result);
   flag_h  = (result & 0x0f) == 0x0f;
   flag_pv = (result == 0x7f);
   flag_n  = 1;
   if (arg_write != result) {
      failflag = FAIL_ERROR;
   }
   update_pc();
   // Update undocumented memptr register
   update_memptr_idx_disp();
   // Update undocumented Q register
   flags_updated();
}

// ===================================================================
// Emulated instructions - Miscellaneous
// ===================================================================

static void op_di(InstrType *instr) {
   reg_iff1 = 0;
   reg_iff2 = 0;
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_ei(InstrType *instr) {
   reg_iff1 = 1;
   reg_iff2 = 1;
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_im(InstrType *instr) {
   switch ((opcode >> 3) & 3) {
   case 2:
      reg_im = 1;
      break;
   case 3:
      reg_im = 2;
      break;
   default:
      reg_im = 0;
      break;
   }
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_rrd(InstrType *instr) {
   if (reg_a >= 0) {
      reg_a = (reg_a & 0xf0) | (arg_read & 0x0f);
      set_sign_zero(reg_a);
      flag_pv = partab[reg_a];
   } else {
      set_sign_zero_undefined();
      flag_pv = -1;
   }
   flag_h = 0;
   flag_n = 0;
   // Update undocumented memptr register
   int hl = read_reg_pair1(ID_RR_HL);
   update_memptr_inc(hl);
   // Update undocumented Q register
   flags_updated();
}

static void op_rld(InstrType *instr) {
   if (reg_a >= 0) {
      reg_a = (reg_a & 0xf0) | ((arg_read >> 4) & 0x0f);
      set_sign_zero(reg_a);
      flag_pv = partab[reg_a];
   } else {
      set_sign_zero_undefined();
      flag_pv = -1;
   }
   flag_h = 0;
   flag_n = 0;
   // Update undocumented memptr register
   int hl = read_reg_pair1(ID_RR_HL);
   update_memptr_inc(hl);
   // Update undocumented Q register
   flags_updated();
}

static void op_misc_rotate(InstrType *instr) {
   if (reg_a < 0) {
      set_flags_undefined();
   } else {
      int rot_op = (opcode >> 3) & 3;
      int operand = reg_a;
      int result;
      switch (rot_op) {
      case 0:
         // RLC
         result = (operand << 1) | (operand >> 7);
         flag_c = (operand >> 7) & 1;
         break;
      case 1:
         // RRC
         result = (operand >> 1) | (operand << 7);
         flag_c = operand & 1;
         break;
      case 2:
         // RL
         if (flag_c >= 0) {
            result = (operand << 1) | flag_c;
         } else {
            result = -1;
         }
         flag_c = (operand >> 7) & 1;
         break;
      case 3:
         // RR
         if (flag_c >= 0) {
            result = (flag_c << 7) | (operand >> 1);
         } else {
            result = -1;
         }
         flag_c = operand & 1;
         break;
      }
      if (result >= 0) {
         result &= 0xff;
         flag_f5 = (result >> 5) & 1;
         flag_f3 = (result >> 3) & 1;
         reg_a = result;
      } else {
         set_flags_undefined();
      }
   }
   flag_h = 0;
   flag_n = 0;
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void op_misc_daa(InstrType *instr) {
   if (reg_a < 0 || flag_h < 0 || flag_c < 0 || flag_n < 0) {
      reg_a = -1;
      set_flags_undefined();
   } else {
      // Borrowed from YAZE
      int temp = reg_a & 0x0f;;
      if (flag_n) {
         // last operation was a subtract
         int hd = flag_c || reg_a > 0x99;
         if (flag_h || (temp > 9)) {
            // adjust low digit
            if (temp > 5) {
               flag_h = 0;
            }
            reg_a -= 6;
            reg_a &= 0xff;
         }
         if (hd) {
            // adjust high digit
            reg_a -= 0x160;
         }
      } else {
         // last operation was an add
         if (flag_h || (temp > 9)) {
            /* adjust low digit */
            flag_h = (temp > 9);
            reg_a += 6;
         }
         if (flag_c || ((reg_a & 0x1f0) > 0x90)) {
            /* adjust high digit */
            reg_a += 0x60;
         }
      }
      flag_c |= (reg_a >> 8) & 1;
      reg_a &= 0xff;
      set_sign_zero(reg_a);
      flag_pv = partab[reg_a];
   }
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void op_misc_cpl(InstrType *instr) {
   if (reg_a >= 0) {
      reg_a ^= 0xff;
      flag_f5 = (reg_a >> 5) & 1;
      flag_f3 = (reg_a >> 3) & 1;
   } else {
      flag_f5 = -1;
      flag_f3 = -1;
   }
   flag_h = 1;
   flag_n = 1;
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void scf_ccf_set_f5_f3_flags() {
#ifdef DEBUG_SCF_CCF
   tmp_a  = reg_a;
   tmp_f5 = flag_f5;
   tmp_f3 = flag_f3;
   tmp_q  = reg_q;
   // SCF = 0x37
   // CCF = 0x3F
   tmp_op = ((opcode >> 3) & 1) ^ 1;
#endif
   // Default to setting the flags as unknown
   int new_flag_f5 = -1;
   int new_flag_f3 = -1;
   // For some CPU types, we know the exact behaviour
   switch (cpu) {
   case CPU_NMOS_ZILOG:
      if (reg_a >= 0 && reg_q >= 0) {
         if (reg_q) {
            new_flag_f5 = (reg_a >> 5) & 1;
            new_flag_f3 = (reg_a >> 3) & 1;
         } else {
            new_flag_f5 = flag_f5 | ((reg_a >> 5) & 1);
            new_flag_f3 = flag_f3 | ((reg_a >> 3) & 1);
         }
      }
      break;
   case CPU_NMOS_NEC:
      if (reg_a >= 0) {
         new_flag_f5 = (reg_a >> 5) & 1;
         new_flag_f3 = (reg_a >> 3) & 1;
      }
      break;
   case CPU_CMOS:
      if (reg_a >= 0 && reg_q >= 0) {
         if (reg_q) {
            new_flag_f5 = (reg_a >> 5) & 1;
         } else {
            new_flag_f5 = flag_f5;
         }
         new_flag_f3 = (reg_a >> 3) & 1;
      }
      break;
   }
   // Copy the newly calculated flags
   flag_f5 = new_flag_f5;
   flag_f3 = new_flag_f3;
}

static void op_misc_scf(InstrType *instr) {
   flag_h = 0;
   flag_c = 1;
   flag_n = 0;
   scf_ccf_set_f5_f3_flags();
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void op_misc_ccf(InstrType *instr) {
   flag_h = flag_c;
   if (flag_c >= 0) {
      flag_c = flag_c ^ 1;
   }
   flag_n = 0;
   scf_ccf_set_f5_f3_flags();
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

// ===================================================================
// Emulated instructions - Exchange
// ===================================================================

static void op_ex_af(InstrType *instr) {
   swap(&reg_a,   &alt_reg_a);
   swap(&flag_s,  &alt_flag_s);
   swap(&flag_z,  &alt_flag_z);
   swap(&flag_f5, &alt_flag_f5);
   swap(&flag_h,  &alt_flag_h);
   swap(&flag_f3, &alt_flag_f3);
   swap(&flag_pv, &alt_flag_pv);
   swap(&flag_n,  &alt_flag_n);
   swap(&flag_c,  &alt_flag_c);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_exx(InstrType *instr) {
   swap(&reg_b,   &alt_reg_b);
   swap(&reg_c,   &alt_reg_c);
   swap(&reg_d,   &alt_reg_d);
   swap(&reg_e,   &alt_reg_e);
   swap(&reg_h,   &alt_reg_h);
   swap(&reg_l,   &alt_reg_l);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
};

static void op_ex_de_hl(InstrType *instr) {
   swap(&reg_d,   &reg_h);
   swap(&reg_e,   &reg_l);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_ex_tos_hl(InstrType *instr) {
   // (SP) <=> register L; (SP + 1) <=> register H
   // register is HL, IDX or IDY
   int reg_id = get_hl_or_idx_id();
   int reg = read_reg_pair1(reg_id);
   if (reg >= 0 && reg != arg_write) {
      failflag = FAIL_ERROR;
   }
   write_reg_pair1(reg_id, arg_read);
   // Update undocumented memptr register
   update_memptr(arg_read);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

// ===================================================================
// Emulated instructions - Load
// ===================================================================

static void op_load_a_i(InstrType *instr) {
   reg_a = reg_i;
   set_sign_zero(reg_a);
   flag_h = 0;
   flag_n = 0;
   flag_pv = reg_iff2;
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_load_a_r(InstrType *instr) {
   reg_a = reg_r;
   set_sign_zero(reg_a);
   flag_h = 0;
   flag_n = 0;
   flag_pv = reg_iff2;
   update_pc();
   // Update undocumented Q register
   flags_updated();
}

static void op_load_i_a(InstrType *instr) {
   reg_i = reg_a;
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_load_r_a(InstrType *instr) {
   reg_r = reg_a;
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_load_sp_hl(InstrType *instr) {
   int rr_id = get_hl_or_idx_id();
   reg_sp = read_reg_pair1(rr_id);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_load_reg8(InstrType *instr) {
   // LD r[y], r[z]
   int dst_id = (opcode >> 3) & 7;
   int src_id = opcode & 7;
   if (dst_id != ID_MEMORY && src_id != ID_MEMORY) {
      dst_id = get_r_id(dst_id);
      src_id = get_r_id(src_id);
  }
   int *dst = reg_ptr[dst_id];
   int *src = reg_ptr[src_id];
   if (dst_id == ID_MEMORY) {
      if ((*src) >= 0 && (*src) != arg_write) {
         failflag = FAIL_ERROR;
      }
   } else {
      *dst = *src;
   }
   update_pc();
   // Update undocumented memptr register if (ix+disp) addressing used
   if ((prefix == 0xdd || prefix == 0xfd) && (dst_id == ID_MEMORY || src_id == ID_MEMORY)) {
      update_memptr_idx_disp();
   }
   // Update undocumented Q register
   flags_not_updated();
}

static void op_load_idx_disp(InstrType *instr) {
   if (arg_imm != arg_write) {
      failflag = FAIL_ERROR;
   }
   update_pc();
   // Update undocumented memptr register
   update_memptr_idx_disp();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_load_imm8(InstrType *instr) {
   // LD r[y], n
   int reg_id = get_r_id((opcode >> 3) & 7);
   int *reg = reg_ptr[reg_id];
   if (reg_id == ID_MEMORY) {
      if (arg_imm != arg_write) {
         failflag = FAIL_ERROR;
      }
   } else {
      *reg = arg_imm;
   }
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_load_imm16(InstrType *instr) {
   int reg_id = get_rr_id();
   write_reg_pair1(reg_id, arg_imm);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}


static void op_load_a(InstrType *instr) {
   // EA = (BC) or (DE) or (nn)
   reg_a = arg_read;
   // Update undocumented memptr register
   int rr_id = (opcode >> 4) & 3;
   update_memptr_inc(rr_id < 2 ? read_reg_pair1(rr_id) : arg_imm);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_store_a(InstrType *instr) {
   // EA = (BC) or (DE) or (nn)
   if (reg_a >= 0 && reg_a != arg_write) {
      failflag = FAIL_ERROR;
   }
   reg_a = arg_write;
   // Update undocumented memptr register
   int rr_id = (opcode >> 4) & 3;
   update_memptr_inc_split(reg_a, rr_id < 2 ? read_reg_pair1(rr_id) : arg_imm);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_load_mem16(InstrType *instr) {
   int reg_id = get_rr_id();
   write_reg_pair1(reg_id, arg_read);
   // Update undocumented memptr register
   update_memptr_inc(arg_imm);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}


static void op_store_mem16(InstrType *instr) {
   int rr_id = get_rr_id();
   int rr = read_reg_pair1(rr_id);
   if (rr >= 0 && rr != arg_write) {
      failflag = FAIL_ERROR;
   }
   write_reg_pair1(rr_id, arg_write);
   // Update undocumented memptr register
   update_memptr_inc(arg_imm);
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}


// ===================================================================
// Emulated instructions - In/Out
// ===================================================================


static void op_in_a_nn(InstrType *instr) {
   // Update undocumented memptr register
   // MEMPTR = (A_before_operation << 8) + port + 1
   // TODO: this might be incorrect for port=0xff
   update_memptr_inc_split(reg_a, arg_imm);
   reg_a = arg_read;
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_out_nn_a(InstrType *instr) {
   // Update undocumented memptr register
   // MEMPTR_low = (port + 1) & #FF,  MEMPTR_hi = A
   update_memptr_inc_split(reg_a, arg_imm);
   if (reg_a >= 0 && reg_a != arg_write) {
      failflag = FAIL_ERROR;
   }
   reg_a = arg_write;
   update_pc();
   // Update undocumented Q register
   flags_not_updated();
}

static void op_in_r_c(InstrType *instr) {
   int reg_id = (opcode >> 3) & 7;
   int result = arg_read;
   // reg_id 6 is used for no destination
   if (reg_id != 6) {
      int *reg = reg_ptr[reg_id];
      *reg = result;
   }
   set_sign_zero(result);
   flag_h = 0;
   flag_n = 0;
   flag_pv = partab[result];
   update_pc();
   // Update undocumented memptr register
   int bc = read_reg_pair1(ID_RR_BC);
   update_memptr_inc(bc);
   // Update undocumented Q register
   flags_updated();
}

static void op_out_c_r(InstrType *instr) {
   int reg_id = (opcode >> 3) & 7;
   if (reg_id == 6) {
      // reg_id 6 is used for OUT (C),0
      if (arg_write != (cpu == CPU_CMOS ? 0xff : 0)) {
         failflag = 1;
      }
   } else {
      int *reg = reg_ptr[reg_id];
      if ((*reg) >= 0 && (*reg != arg_write)) {
         failflag = FAIL_ERROR;
      }
      *reg = arg_write;
   }
   update_pc();
   // Update undocumented memptr register
   int bc = read_reg_pair1(ID_RR_BC);
   update_memptr_inc(bc);
   // Update undocumented Q register
   flags_not_updated();
}

// ===================================================================
// Emulated instructions - Block In/Out
// ===================================================================


static void block_decrement_rr(int rr) {
   int value = read_reg_pair1(rr);
   if (value >= 0) {
      value = (value - 1) & 0xffff;
   }
   write_reg_pair1(rr, value);
}

static void block_increment_rr(int rr) {
   int value = read_reg_pair1(rr);
   if (value >= 0) {
      value = (value + 1) & 0xffff;
   }
   write_reg_pair1(rr, value);
}

static void block_decrement_bc() {
   block_decrement_rr(ID_RR_BC);
}

static void block_decrement_de() {
   block_decrement_rr(ID_RR_DE);
}

static void block_increment_de() {
   block_increment_rr(ID_RR_DE);
}

static void block_decrement_hl() {
   block_decrement_rr(ID_RR_HL);
}

static void block_increment_hl() {
   block_increment_rr(ID_RR_HL);
}

static void block_decrement_b(int io_data, int reg_other) {
   // Start by setting all the flags to unknown, as they will all be set
   set_flags_undefined();
   // Decrement B and set the flags S Z F5 F3 and N flags
   if (reg_b >= 0) {
      reg_b = (reg_b - 1) & 0xff;
      set_sign_zero(reg_b);
      flag_n = (io_data >> 7) & 1;
   }
   // Set the remaining flags
   if (reg_other >= 0) {
      flag_c = ((arg_write + reg_other) > 255);
      flag_h = flag_c;
   }
   if (reg_other >= 0 && reg_b >= 0) {
      flag_pv = partab[((io_data + reg_other) & 7) ^ reg_b];
   }
}

static void op_ind_ini(InstrType *instr) {
   // INI   0xA2
   // IND   0xAA
   // INIR  0xB2
   // INDR  0xBA
   int dec_op = opcode & 0x08;
   int repeat_op = opcode & 0x10;
   if (arg_write != arg_read) {
      failflag = FAIL_ERROR;
   }
   if (dec_op) {
      block_decrement_hl();
   } else {
      block_increment_hl();
   }
   // Update undocumented memptr register before B is decremented
   int bc = read_reg_pair1(ID_RR_BC);
   if (dec_op) {
      update_memptr_dec(bc);
   } else {
      update_memptr_inc(bc);
   }
   // Decrement B and set all the flags
   int reg_other = reg_c;
   if (reg_other >= 0) {
      reg_other = (reg_other + (dec_op ? -1 : 1)) & 0xff;
   }
   block_decrement_b(arg_read, reg_other);
   // TODO: Use cycles to infer termination
   if (!repeat_op || flag_z == 1)  {
      update_pc();
   } else if (flag_z < 0) {
      reg_pc = -1;
   }
   // Update undocumented Q register
   flags_updated();
}

static void op_outd_outi(InstrType *instr) {
   // OUTI   0xA3
   // OUTD   0xAB
   // OITIR  0xB3
   // OUTDR  0xBB
   int dec_op = opcode & 0x08;
   int repeat_op = opcode & 0x10;
   if (arg_write != arg_read) {
      failflag = FAIL_ERROR;
   }
   if (dec_op) {
      block_decrement_hl();
   } else {
      block_increment_hl();
   }
   // Decrement B and set all the flags
   int reg_other = reg_l;
   block_decrement_b(arg_write, reg_other);
   // TODO: Use cycles to infer termination
   if (!repeat_op || flag_z == 1)  {
      update_pc();
   } else if (flag_z < 0) {
      reg_pc = -1;
   }
   // Update undocumented memptr register after B is decremented
   int bc = read_reg_pair1(ID_RR_BC);
   if (dec_op) {
      update_memptr_dec(bc);
   } else {
      update_memptr_inc(bc);
   }
   // Update undocumented Q register
   flags_updated();
}

// ===================================================================
// Emulated instructions - Block load
// ===================================================================

static void op_ldd_ldi(InstrType *instr) {
   // LDI   0xA0
   // LDD   0xA8
   // LDIR  0xB0
   // LDDR  0xB8
   int dec_op = opcode & 0x08;
   int repeat_op = opcode & 0x10;
   if (arg_write != arg_read) {
      failflag = FAIL_ERROR;
   }
   block_decrement_bc();
   if (dec_op) {
      block_decrement_de();
      block_decrement_hl();
   } else {
      block_increment_de();
      block_increment_hl();
   }
   // Set the flags, see: page 16 of http://www.z80.info/zip/z80-documented.pdf
   flag_h = 0;
   flag_n = 0;
   if (reg_b >= 0 && reg_c >= 0) {
      flag_pv = reg_b != 0 || reg_c != 0;
   } else {
      flag_pv = -1;
   }
   // Update the undocumented f5/f3 flags
   if (repeat_op && flag_pv == 1 && reg_pc >= 0) {
      // If a LDIR/LDDR is interrupted, the f5/f3 flags come from the current PC
      flag_f5 = (reg_pc >> 13) & 1;
      flag_f3 = (reg_pc >> 11) & 1;
   } else if ((!repeat_op || flag_pv == 0) && reg_a >= 0) {
      // If a LDI/LDD/LDIR/LDDR ends normally, the f5/f3 flags come from A + data
      int result = reg_a + arg_read;
      flag_f5 = (result >> 1) & 1;
      flag_f3 = (result >> 3) & 1;
   } else {
      flag_f5 = -1;
      flag_f3 = -1;
   }
   // Update undocumented memptr register
   if (repeat_op && flag_pv == 1) {
      update_memptr_inc(reg_pc);
   } else if (repeat_op && flag_pv < 0) {
      update_memptr(-1);
   }
   // TODO: Use cycles to infer termination
   if (!repeat_op || flag_pv == 0) {
      update_pc();
   } else if (repeat_op && flag_pv < 0) {
      reg_pc = -1;
   }
   // Update undocumented Q register
   flags_updated();
}

static void op_cpd_cpi(InstrType *instr) {
   // CDI   0xA1
   // CPD   0xA9
   // CPIR  0xB1
   // CPDR  0xB9
   int dec_op = opcode & 0x08;
   int repeat_op = opcode & 0x10;
   block_decrement_bc();
   if (dec_op) {
      block_decrement_hl();
   } else {
      block_increment_hl();
   }
   // Set the flags, see: page 16 of http://www.z80.info/zip/z80-documented.pdf
   if (reg_a >= 0) {
      int result = reg_a - arg_read;
      int cbits = reg_a ^ arg_read ^ result;
      flag_s = (result >> 7) & 1;
      flag_z = ((result & 0xff) == 0);
      flag_h = (cbits >> 4) & 1;
      int n = (reg_a - arg_read - flag_h) & 0xff;
      flag_f5 = (n >> 1) & 1;
      flag_f3 = (n >> 3) & 1;
   } else {
      set_sign_zero_undefined();
      flag_h  = -1;
   }
   flag_n = 1;
   if (reg_b >= 0 && reg_c >= 0) {
      flag_pv = reg_b != 0 || reg_c != 0;
   } else {
      flag_pv = -1;
   }
   // Update undocumented memptr register
   if (!repeat_op || flag_pv == 0 || flag_z == 1) {
      if (dec_op) {
         update_memptr_dec(reg_memptr);
      } else {
         update_memptr_inc(reg_memptr);
      }
   } else if (flag_pv == 1 && flag_z == 0) {
      update_memptr_inc(reg_pc);
   } else {
      update_memptr(-1);
   }
   // TODO: Use cycles to infer termination
   if (!repeat_op || flag_pv == 0 || flag_z == 1) {
      update_pc();
   } else if (repeat_op && !(flag_pv == 1 || flag_z == 0)) {
      reg_pc = -1;
   }
   // Update undocumented Q register
   flags_updated();
}

// ===================================================================
// Emulated instructions - Bit
// ===================================================================

static void op_bit(InstrType *instr) {
   int reg_id   = opcode & 7;
   int major_op = (opcode >> 6) & 3;
   int minor_op = (opcode >> 3) & 7;
   int operand  = (prefix == 0xcb) ? *reg_ptr[reg_id] : arg_read;

   // If the operand is undefined (i.e. a register whose value is unknown)
   // then deal with the flags up front, and exit
   if (operand < 0) {

      switch (major_op) {
      case 0:
         // Rotate / Shift
         set_flags_undefined();
         flag_h  =  0;
         flag_n  =  0;
         break;

      case 1:
         // BIT
         set_sign_zero_undefined();
         flag_pv = -1;
         flag_h  =  1;
         flag_n  =  0;
         break;

      case 2:
         // RES
         // no effect on flags
         break;

      case 3:
         // SET
         // no effect on flags
         break;
      }

   } else {

      int result;

      switch (major_op) {

      case 0:
         // Rotate / Shift
         switch (minor_op) {
         case 0:
            // RLC
            result = (operand << 1) | (operand >> 7);
            flag_c = (operand >> 7) & 1;
            break;
         case 1:
            // RRC
            result = (operand >> 1) | (operand << 7);
            flag_c = operand & 1;
            break;
         case 2:
            // RL
            if (flag_c >= 0) {
               result = (operand << 1) | flag_c;
            } else {
               result = -1;
            }
            flag_c = (operand >> 7) & 1;
            break;
         case 3:
            // RR
            if (flag_c >= 0) {
               result = (flag_c << 7) | (operand >> 1);
            } else {
               result = -1;
            }
            flag_c = operand & 1;
            break;
         case 4:
            // SLA
            result = operand << 1;
            flag_c = (operand >> 7) & 1;
            break;
         case 5:
            // SRA
            result = (operand & 0x80) | (operand >> 1);
            flag_c = operand & 1;
            break;
         case 6:
            // SLL
            result = (operand << 1) | 1;
            flag_c = (operand >> 7) & 1;
            break;
         case 7:
            // SRL
            result = operand >> 1;
            flag_c = operand & 1;
            break;
         }
         if (result >= 0) {
            result &= 0xff;
            set_sign_zero(result);
            flag_pv = partab[result];
         } else {
            set_flags_undefined();
         }
         flag_h = 0;
         flag_n = 0;
         break;

      case 1:
         // BIT
         result = operand & (1 << minor_op);
         set_sign_zero(result);
         if (prefix == 0xddcb || prefix == 0xfdcb) {
            // Correct the f5 and f3 flags for BIT N,(IX+D)
            int rr_id = (prefix == 0xfdcb) ? ID_RR_IY : ID_RR_IX;
            int reg = read_reg_pair1(rr_id);
            if (reg >= 0) {
               reg += arg_dis;
               flag_f5 = (reg >> 13) & 1;
               flag_f3 = (reg >> 11) & 1;
            } else {
               flag_f5 = -1;
               flag_f3 = -1;
            }
         } else if (prefix == 0xcb && reg_id == ID_MEMORY) {
            // Correct the f5 and f3 flags for BIT N,(HL)
            if (reg_memptr >= 0) {
               flag_f5 = (reg_memptr >> 13) & 1;
               flag_f3 = (reg_memptr >> 11) & 1;
            } else {
               flag_f5 = -1;
               flag_f3 = -1;
            }
         } else {
            // This different to Sean Young's document, but matches Yaze, MAME and a real trace
            flag_f5 = (operand >> 5) & 1;
            flag_f3 = (operand >> 3) & 1;
         }
         flag_h = 1;
         flag_n = 0;
         flag_pv = flag_z;
         break;

      case 2:
         // RES
         result = operand & ~(1 << minor_op);
         break;

      case 3:
         // SET
         result = operand | (1 << minor_op);
         break;
      }
      if (major_op != 1) {
         if (reg_id == ID_MEMORY) {
            if (arg_write != result) {
               failflag = FAIL_ERROR;
            }
         } else {
            *reg_ptr[reg_id] = result;
         }
      }
   }
   update_pc();
   // Update undocumented memptr register if (ix+disp) addressing used
   if (prefix == 0xddcb || prefix == 0xfdcb || ((prefix == 0xdd || prefix == 0xfd) && reg_id == ID_MEMORY)) {
      update_memptr_idx_disp();
   }
   // Update undocumented Q register
   switch (major_op) {
   case 0:
      // Rotate / Shift
   case 1:
      // BIT
      flags_updated();
      break;
   case 2:
      // RES no effect on flags
   case 3:
      // SET no effect on flags
      flags_not_updated();
      break;
   }
}

//Instruction tuple: (d, i, ro, wo, conditional, format type, format string, emulate method)
//
//  The placeholders d and i are the number of bytes in the instruction
//  used for the displacement and the immediate operand, respectively. An
//  operand consisting of more than one byte is assembled in little endian
//  order.
//  The placeholders ro and wo are the number of bytes the instruction
//  is expected to read or write, respectively. These counts are used
//  for both memory and I/O access, but not for immediate operands.
//  A negative value indicates that the operand byte order is big endian
//  rather than the usual little endian.
//  The placeholder rep is a boolean used to mark repeating instructions.
//  The format string should refer to the {d} and {i} operands by name.
//  Displacements are interpreted as signed integers, whereas immediate
//  operands are always read as unsigned. The tables for instructions
//  operating on the IX/IY index registers additionally use {r} in the
//  format string as a placeholder for the register name.
//  Relative jump instructions may specify {j} instead of {d} to output
//  the displacement relative to the start of the instruction.

// Instructions without a prefix
InstrType main_instructions[256] = {
   {0, 0, 0, 0, False, TYPE_0, "NOP",               op_nop          }, // 0x00
   {0, 2, 0, 0, False, TYPE_8, "LD BC,%04Xh",       op_load_imm16   }, // 0x01
   {0, 0, 0, 1, False, TYPE_0, "LD (BC),A",         op_store_a      }, // 0x02
   {0, 0, 0, 0, False, TYPE_0, "INC BC",            op_inc_rr       }, // 0x03
   {0, 0, 0, 0, False, TYPE_0, "INC B",             op_inc_r        }, // 0x04
   {0, 0, 0, 0, False, TYPE_0, "DEC B",             op_dec_r        }, // 0x05
   {0, 1, 0, 0, False, TYPE_8, "LD B,%02Xh",        op_load_imm8    }, // 0x06
   {0, 0, 0, 0, False, TYPE_0, "RLCA",              op_misc_rotate  }, // 0x07
   {0, 0, 0, 0, False, TYPE_0, "EX AF,AF'",         op_ex_af        }, // 0x08
   {0, 0, 0, 0, False, TYPE_0, "ADD HL,BC",         op_add_hl_rr    }, // 0x09
   {0, 0, 1, 0, False, TYPE_0, "LD A,(BC)",         op_load_a       }, // 0x0A
   {0, 0, 0, 0, False, TYPE_0, "DEC BC",            op_dec_rr       }, // 0x0B
   {0, 0, 0, 0, False, TYPE_0, "INC C",             op_inc_r        }, // 0x0C
   {0, 0, 0, 0, False, TYPE_0, "DEC C",             op_dec_r        }, // 0x0D
   {0, 1, 0, 0, False, TYPE_8, "LD C,%02Xh",        op_load_imm8    }, // 0x0E
   {0, 0, 0, 0, False, TYPE_0, "RRCA",              op_misc_rotate  }, // 0x0F

   {1, 0, 0, 0, False, TYPE_7, "DJNZ %s",           op_djnz         }, // 0x10
   {0, 2, 0, 0, False, TYPE_8, "LD DE,%04Xh",       op_load_imm16   }, // 0x11
   {0, 0, 0, 1, False, TYPE_0, "LD (DE),A",         op_store_a      }, // 0x12
   {0, 0, 0, 0, False, TYPE_0, "INC DE",            op_inc_rr       }, // 0x13
   {0, 0, 0, 0, False, TYPE_0, "INC D",             op_inc_r        }, // 0x14
   {0, 0, 0, 0, False, TYPE_0, "DEC D",             op_dec_r        }, // 0x15
   {0, 1, 0, 0, False, TYPE_8, "LD D,%02Xh",        op_load_imm8    }, // 0x16
   {0, 0, 0, 0, False, TYPE_0, "RLA",               op_misc_rotate  }, // 0x17
   {1, 0, 0, 0, False, TYPE_7, "JR %s",             op_jr           }, // 0x18
   {0, 0, 0, 0, False, TYPE_0, "ADD HL,DE",         op_add_hl_rr    }, // 0x19
   {0, 0, 1, 0, False, TYPE_0, "LD A,(DE)",         op_load_a       }, // 0x1A
   {0, 0, 0, 0, False, TYPE_0, "DEC DE",            op_dec_rr       }, // 0x1B
   {0, 0, 0, 0, False, TYPE_0, "INC E",             op_inc_r        }, // 0x1C
   {0, 0, 0, 0, False, TYPE_0, "DEC E",             op_dec_r        }, // 0x1D
   {0, 1, 0, 0, False, TYPE_8, "LD E,%02Xh",        op_load_imm8    }, // 0x1E
   {0, 0, 0, 0, False, TYPE_0, "RRA",               op_misc_rotate  }, // 0x1F

   {1, 0, 0, 0, False, TYPE_7, "JR NZ,%s",          op_jr           }, // 0x20
   {0, 2, 0, 0, False, TYPE_8, "LD HL,%04Xh",       op_load_imm16   }, // 0x21
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),HL",     op_store_mem16  }, // 0x22
   {0, 0, 0, 0, False, TYPE_0, "INC HL",            op_inc_rr       }, // 0x23
   {0, 0, 0, 0, False, TYPE_0, "INC H",             op_inc_r        }, // 0x24
   {0, 0, 0, 0, False, TYPE_0, "DEC H",             op_dec_r        }, // 0x25
   {0, 1, 0, 0, False, TYPE_8, "LD H,%02Xh",        op_load_imm8    }, // 0x26
   {0, 0, 0, 0, False, TYPE_0, "DAA",               op_misc_daa     }, // 0x27
   {1, 0, 0, 0, False, TYPE_7, "JR Z,%s",           op_jr           }, // 0x28
   {0, 0, 0, 0, False, TYPE_0, "ADD HL,HL",         op_add_hl_rr    }, // 0x29
   {0, 2, 2, 0, False, TYPE_8, "LD HL,(%04Xh)",     op_load_mem16   }, // 0x2A
   {0, 0, 0, 0, False, TYPE_0, "DEC HL",            op_dec_rr       }, // 0x2B
   {0, 0, 0, 0, False, TYPE_0, "INC L",             op_inc_r        }, // 0x2C
   {0, 0, 0, 0, False, TYPE_0, "DEC L",             op_dec_r        }, // 0x2D
   {0, 1, 0, 0, False, TYPE_8, "LD L,%02Xh",        op_load_imm8    }, // 0x2E
   {0, 0, 0, 0, False, TYPE_0, "CPL",               op_misc_cpl     }, // 0x2F

   {1, 0, 0, 0, False, TYPE_7, "JR NC,%s",          op_jr           }, // 0x30
   {0, 2, 0, 0, False, TYPE_8, "LD SP,%04Xh",       op_load_imm16   }, // 0x31
   {0, 2, 0, 1, False, TYPE_8, "LD (%04Xh),A",      op_store_a      }, // 0x32
   {0, 0, 0, 0, False, TYPE_0, "INC SP",            op_inc_rr       }, // 0x33
   {0, 0, 1, 1, False, TYPE_0, "INC (HL)",          op_inc_r        }, // 0x34
   {0, 0, 1, 1, False, TYPE_0, "DEC (HL)",          op_dec_r        }, // 0x35
   {0, 1, 0, 1, False, TYPE_8, "LD (HL),%02Xh",     op_load_imm8    }, // 0x36
   {0, 0, 0, 0, False, TYPE_0, "SCF",               op_misc_scf     }, // 0x37
   {1, 0, 0, 0, False, TYPE_7, "JR C,%s",           op_jr           }, // 0x38
   {0, 0, 0, 0, False, TYPE_0, "ADD HL,SP",         op_add_hl_rr    }, // 0x39
   {0, 2, 1, 0, False, TYPE_8, "LD A,(%04Xh)",      op_load_a       }, // 0x3A
   {0, 0, 0, 0, False, TYPE_0, "DEC SP",            op_dec_rr       }, // 0x3B
   {0, 0, 0, 0, False, TYPE_0, "INC A",             op_inc_r        }, // 0x3C
   {0, 0, 0, 0, False, TYPE_0, "DEC A",             op_dec_r        }, // 0x3D
   {0, 1, 0, 0, False, TYPE_8, "LD A,%02Xh",        op_load_imm8    }, // 0x3E
   {0, 0, 0, 0, False, TYPE_0, "CCF",               op_misc_ccf     }, // 0x3F

   {0, 0, 0, 0, False, TYPE_0, "LD B,B",            op_load_reg8    }, // 0x40
   {0, 0, 0, 0, False, TYPE_0, "LD B,C",            op_load_reg8    }, // 0x41
   {0, 0, 0, 0, False, TYPE_0, "LD B,D",            op_load_reg8    }, // 0x42
   {0, 0, 0, 0, False, TYPE_0, "LD B,E",            op_load_reg8    }, // 0x43
   {0, 0, 0, 0, False, TYPE_0, "LD B,H",            op_load_reg8    }, // 0x44
   {0, 0, 0, 0, False, TYPE_0, "LD B,L",            op_load_reg8    }, // 0x45
   {0, 0, 1, 0, False, TYPE_0, "LD B,(HL)",         op_load_reg8    }, // 0x46
   {0, 0, 0, 0, False, TYPE_0, "LD B,A",            op_load_reg8    }, // 0x47
   {0, 0, 0, 0, False, TYPE_0, "LD C,B",            op_load_reg8    }, // 0x48
   {0, 0, 0, 0, False, TYPE_0, "LD C,C",            op_load_reg8    }, // 0x49
   {0, 0, 0, 0, False, TYPE_0, "LD C,D",            op_load_reg8    }, // 0x4A
   {0, 0, 0, 0, False, TYPE_0, "LD C,E",            op_load_reg8    }, // 0x4B
   {0, 0, 0, 0, False, TYPE_0, "LD C,H",            op_load_reg8    }, // 0x4C
   {0, 0, 0, 0, False, TYPE_0, "LD C,L",            op_load_reg8    }, // 0x4D
   {0, 0, 1, 0, False, TYPE_0, "LD C,(HL)",         op_load_reg8    }, // 0x4E
   {0, 0, 0, 0, False, TYPE_0, "LD C,A",            op_load_reg8    }, // 0x4F

   {0, 0, 0, 0, False, TYPE_0, "LD D,B",            op_load_reg8    }, // 0x50
   {0, 0, 0, 0, False, TYPE_0, "LD D,C",            op_load_reg8    }, // 0x51
   {0, 0, 0, 0, False, TYPE_0, "LD D,D",            op_load_reg8    }, // 0x52
   {0, 0, 0, 0, False, TYPE_0, "LD D,E",            op_load_reg8    }, // 0x53
   {0, 0, 0, 0, False, TYPE_0, "LD D,H",            op_load_reg8    }, // 0x54
   {0, 0, 0, 0, False, TYPE_0, "LD D,L",            op_load_reg8    }, // 0x55
   {0, 0, 1, 0, False, TYPE_0, "LD D,(HL)",         op_load_reg8    }, // 0x56
   {0, 0, 0, 0, False, TYPE_0, "LD D,A",            op_load_reg8    }, // 0x57
   {0, 0, 0, 0, False, TYPE_0, "LD E,B",            op_load_reg8    }, // 0x58
   {0, 0, 0, 0, False, TYPE_0, "LD E,C",            op_load_reg8    }, // 0x59
   {0, 0, 0, 0, False, TYPE_0, "LD E,D",            op_load_reg8    }, // 0x5A
   {0, 0, 0, 0, False, TYPE_0, "LD E,E",            op_load_reg8    }, // 0x5B
   {0, 0, 0, 0, False, TYPE_0, "LD E,H",            op_load_reg8    }, // 0x5C
   {0, 0, 0, 0, False, TYPE_0, "LD E,L",            op_load_reg8    }, // 0x5D
   {0, 0, 1, 0, False, TYPE_0, "LD E,(HL)",         op_load_reg8    }, // 0x5E
   {0, 0, 0, 0, False, TYPE_0, "LD E,A",            op_load_reg8    }, // 0x5F

   {0, 0, 0, 0, False, TYPE_0, "LD H,B",            op_load_reg8    }, // 0x60
   {0, 0, 0, 0, False, TYPE_0, "LD H,C",            op_load_reg8    }, // 0x61
   {0, 0, 0, 0, False, TYPE_0, "LD H,D",            op_load_reg8    }, // 0x62
   {0, 0, 0, 0, False, TYPE_0, "LD H,E",            op_load_reg8    }, // 0x63
   {0, 0, 0, 0, False, TYPE_0, "LD H,H",            op_load_reg8    }, // 0x64
   {0, 0, 0, 0, False, TYPE_0, "LD H,L",            op_load_reg8    }, // 0x65
   {0, 0, 1, 0, False, TYPE_0, "LD H,(HL)",         op_load_reg8    }, // 0x66
   {0, 0, 0, 0, False, TYPE_0, "LD H,A",            op_load_reg8    }, // 0x67
   {0, 0, 0, 0, False, TYPE_0, "LD L,B",            op_load_reg8    }, // 0x68
   {0, 0, 0, 0, False, TYPE_0, "LD L,C",            op_load_reg8    }, // 0x69
   {0, 0, 0, 0, False, TYPE_0, "LD L,D",            op_load_reg8    }, // 0x6A
   {0, 0, 0, 0, False, TYPE_0, "LD L,E",            op_load_reg8    }, // 0x6B
   {0, 0, 0, 0, False, TYPE_0, "LD L,H",            op_load_reg8    }, // 0x6C
   {0, 0, 0, 0, False, TYPE_0, "LD L,L",            op_load_reg8    }, // 0x6D
   {0, 0, 1, 0, False, TYPE_0, "LD L,(HL)",         op_load_reg8    }, // 0x6E
   {0, 0, 0, 0, False, TYPE_0, "LD L,A",            op_load_reg8    }, // 0x6F

   {0, 0, 0, 1, False, TYPE_0, "LD (HL),B",         op_load_reg8    }, // 0x70
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),C",         op_load_reg8    }, // 0x71
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),D",         op_load_reg8    }, // 0x72
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),E",         op_load_reg8    }, // 0x73
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),H",         op_load_reg8    }, // 0x74
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),L",         op_load_reg8    }, // 0x75
   {0, 0, 0, 0, False, TYPE_0, "HALT",              op_halt         }, // 0x76
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),A",         op_load_reg8    }, // 0x77
   {0, 0, 0, 0, False, TYPE_0, "LD A,B",            op_load_reg8    }, // 0x78
   {0, 0, 0, 0, False, TYPE_0, "LD A,C",            op_load_reg8    }, // 0x79
   {0, 0, 0, 0, False, TYPE_0, "LD A,D",            op_load_reg8    }, // 0x7A
   {0, 0, 0, 0, False, TYPE_0, "LD A,E",            op_load_reg8    }, // 0x7B
   {0, 0, 0, 0, False, TYPE_0, "LD A,H",            op_load_reg8    }, // 0x7C
   {0, 0, 0, 0, False, TYPE_0, "LD A,L",            op_load_reg8    }, // 0x7D
   {0, 0, 1, 0, False, TYPE_0, "LD A,(HL)",         op_load_reg8    }, // 0x7E
   {0, 0, 0, 0, False, TYPE_0, "LD A,A",            op_load_reg8    }, // 0x7F

   {0, 0, 0, 0, False, TYPE_0, "ADD A,B",           op_alu          }, // 0x80
   {0, 0, 0, 0, False, TYPE_0, "ADD A,C",           op_alu          }, // 0x81
   {0, 0, 0, 0, False, TYPE_0, "ADD A,D",           op_alu          }, // 0x82
   {0, 0, 0, 0, False, TYPE_0, "ADD A,E",           op_alu          }, // 0x83
   {0, 0, 0, 0, False, TYPE_0, "ADD A,H",           op_alu          }, // 0x84
   {0, 0, 0, 0, False, TYPE_0, "ADD A,L",           op_alu          }, // 0x85
   {0, 0, 1, 0, False, TYPE_0, "ADD A,(HL)",        op_alu          }, // 0x86
   {0, 0, 0, 0, False, TYPE_0, "ADD A,A",           op_alu          }, // 0x87
   {0, 0, 0, 0, False, TYPE_0, "ADC A,B",           op_alu          }, // 0x88
   {0, 0, 0, 0, False, TYPE_0, "ADC A,C",           op_alu          }, // 0x89
   {0, 0, 0, 0, False, TYPE_0, "ADC A,D",           op_alu          }, // 0x8A
   {0, 0, 0, 0, False, TYPE_0, "ADC A,E",           op_alu          }, // 0x8B
   {0, 0, 0, 0, False, TYPE_0, "ADC A,H",           op_alu          }, // 0x8C
   {0, 0, 0, 0, False, TYPE_0, "ADC A,L",           op_alu          }, // 0x8D
   {0, 0, 1, 0, False, TYPE_0, "ADC A,(HL)",        op_alu          }, // 0x8E
   {0, 0, 0, 0, False, TYPE_0, "ADC A,A",           op_alu          }, // 0x8F

   {0, 0, 0, 0, False, TYPE_0, "SUB B",             op_alu          }, // 0x90
   {0, 0, 0, 0, False, TYPE_0, "SUB C",             op_alu          }, // 0x91
   {0, 0, 0, 0, False, TYPE_0, "SUB D",             op_alu          }, // 0x92
   {0, 0, 0, 0, False, TYPE_0, "SUB E",             op_alu          }, // 0x93
   {0, 0, 0, 0, False, TYPE_0, "SUB H",             op_alu          }, // 0x94
   {0, 0, 0, 0, False, TYPE_0, "SUB L",             op_alu          }, // 0x95
   {0, 0, 1, 0, False, TYPE_0, "SUB (HL)",          op_alu          }, // 0x96
   {0, 0, 0, 0, False, TYPE_0, "SUB A",             op_alu          }, // 0x97
   {0, 0, 0, 0, False, TYPE_0, "SBC A,B",           op_alu          }, // 0x98
   {0, 0, 0, 0, False, TYPE_0, "SBC A,C",           op_alu          }, // 0x99
   {0, 0, 0, 0, False, TYPE_0, "SBC A,D",           op_alu          }, // 0x9A
   {0, 0, 0, 0, False, TYPE_0, "SBC A,E",           op_alu          }, // 0x9B
   {0, 0, 0, 0, False, TYPE_0, "SBC A,H",           op_alu          }, // 0x9C
   {0, 0, 0, 0, False, TYPE_0, "SBC A,L",           op_alu          }, // 0x9D
   {0, 0, 1, 0, False, TYPE_0, "SBC A,(HL)",        op_alu          }, // 0x9E
   {0, 0, 0, 0, False, TYPE_0, "SBC A,A",           op_alu          }, // 0x9F

   {0, 0, 0, 0, False, TYPE_0, "AND B",             op_alu          }, // 0xA0
   {0, 0, 0, 0, False, TYPE_0, "AND C",             op_alu          }, // 0xA1
   {0, 0, 0, 0, False, TYPE_0, "AND D",             op_alu          }, // 0xA2
   {0, 0, 0, 0, False, TYPE_0, "AND E",             op_alu          }, // 0xA3
   {0, 0, 0, 0, False, TYPE_0, "AND H",             op_alu          }, // 0xA4
   {0, 0, 0, 0, False, TYPE_0, "AND L",             op_alu          }, // 0xA5
   {0, 0, 1, 0, False, TYPE_0, "AND (HL)",          op_alu          }, // 0xA6
   {0, 0, 0, 0, False, TYPE_0, "AND A",             op_alu          }, // 0xA7
   {0, 0, 0, 0, False, TYPE_0, "XOR B",             op_alu          }, // 0xA8
   {0, 0, 0, 0, False, TYPE_0, "XOR C",             op_alu          }, // 0xA9
   {0, 0, 0, 0, False, TYPE_0, "XOR D",             op_alu          }, // 0xAA
   {0, 0, 0, 0, False, TYPE_0, "XOR E",             op_alu          }, // 0xAB
   {0, 0, 0, 0, False, TYPE_0, "XOR H",             op_alu          }, // 0xAC
   {0, 0, 0, 0, False, TYPE_0, "XOR L",             op_alu          }, // 0xAD
   {0, 0, 1, 0, False, TYPE_0, "XOR (HL)",          op_alu          }, // 0xAE
   {0, 0, 0, 0, False, TYPE_0, "XOR A",             op_alu          }, // 0xAF

   {0, 0, 0, 0, False, TYPE_0, "OR B",              op_alu          }, // 0xB0
   {0, 0, 0, 0, False, TYPE_0, "OR C",              op_alu          }, // 0xB1
   {0, 0, 0, 0, False, TYPE_0, "OR D",              op_alu          }, // 0xB2
   {0, 0, 0, 0, False, TYPE_0, "OR E",              op_alu          }, // 0xB3
   {0, 0, 0, 0, False, TYPE_0, "OR H",              op_alu          }, // 0xB4
   {0, 0, 0, 0, False, TYPE_0, "OR L",              op_alu          }, // 0xB5
   {0, 0, 1, 0, False, TYPE_0, "OR (HL)",           op_alu          }, // 0xB6
   {0, 0, 0, 0, False, TYPE_0, "OR A",              op_alu          }, // 0xB7
   {0, 0, 0, 0, False, TYPE_0, "CP B",              op_alu          }, // 0xB8
   {0, 0, 0, 0, False, TYPE_0, "CP C",              op_alu          }, // 0xB9
   {0, 0, 0, 0, False, TYPE_0, "CP D",              op_alu          }, // 0xBA
   {0, 0, 0, 0, False, TYPE_0, "CP E",              op_alu          }, // 0xBB
   {0, 0, 0, 0, False, TYPE_0, "CP H",              op_alu          }, // 0xBC
   {0, 0, 0, 0, False, TYPE_0, "CP L",              op_alu          }, // 0xBD
   {0, 0, 1, 0, False, TYPE_0, "CP (HL)",           op_alu          }, // 0xBE
   {0, 0, 0, 0, False, TYPE_0, "CP A",              op_alu          }, // 0xBF

   {0, 0, 2, 0, True,  TYPE_0, "RET NZ",            op_ret_cond     }, // 0xC0
   {0, 0, 2, 0, False, TYPE_0, "POP BC",            op_pop          }, // 0xC1
   {0, 2, 0, 0, False, TYPE_8, "JP NZ,%04Xh",       op_jp_cond      }, // 0xC2
   {0, 2, 0, 0, False, TYPE_8, "JP %04Xh",          op_jp           }, // 0xC3
   {0, 2, 0,-2, True,  TYPE_8, "CALL NZ,%04Xh",     op_call_cond    }, // 0xC4
   {0, 0, 0,-2, False, TYPE_0, "PUSH BC",           op_push         }, // 0xC5
   {0, 1, 0, 0, False, TYPE_8, "ADD A,%02Xh",       op_alu          }, // 0xC6
   {0, 0, 0,-2, False, TYPE_0, "RST 00h",           op_rst          }, // 0xC7
   {0, 0, 2, 0, True,  TYPE_0, "RET Z",             op_ret_cond     }, // 0xC8
   {0, 0, 2, 0, False, TYPE_0, "RET",               op_ret          }, // 0xC9
   {0, 2, 0, 0, False, TYPE_8, "JP Z,%04Xh",        op_jp_cond      }, // 0xCA
   UNDEFINED1,                                                         // 0xCB
   {0, 2, 0,-2, True,  TYPE_8, "CALL Z,%04Xh",      op_call_cond    }, // 0xCC
   {0, 2, 0,-2, False, TYPE_8, "CALL %04Xh",        op_call         }, // 0xCD
   {0, 1, 0, 0, False, TYPE_8, "ADC A,%02Xh",       op_alu          }, // 0xCE
   {0, 0, 0,-2, False, TYPE_0, "RST 08h",           op_rst          }, // 0xCF

   {0, 0, 2, 0, True,  TYPE_0, "RET NC",            op_ret_cond     }, // 0xD0
   {0, 0, 2, 0, False, TYPE_0, "POP DE",            op_pop          }, // 0xD1
   {0, 2, 0, 0, False, TYPE_8, "JP NC,%04Xh",       op_jp_cond      }, // 0xD2
   {0, 1, 0, 1, False, TYPE_8, "OUT (%02Xh),A",     op_out_nn_a     }, // 0xD3
   {0, 2, 0,-2, True,  TYPE_8, "CALL NC,%04Xh",     op_call_cond    }, // 0xD4
   {0, 0, 0,-2, False, TYPE_0, "PUSH DE",           op_push         }, // 0xD5
   {0, 1, 0, 0, False, TYPE_8, "SUB %02Xh",         op_alu          }, // 0xD6
   {0, 0, 0,-2, False, TYPE_0, "RST 10h",           op_rst          }, // 0xD7
   {0, 0, 2, 0, True,  TYPE_0, "RET C",             op_ret_cond     }, // 0xD8
   {0, 0, 0, 0, False, TYPE_0, "EXX",               op_exx          }, // 0xD9
   {0, 2, 0, 0, False, TYPE_8, "JP C,%04Xh",        op_jp_cond      }, // 0xDA
   {0, 1, 1, 0, False, TYPE_8, "IN A,(%02Xh)",      op_in_a_nn      }, // 0xDB
   {0, 2, 0,-2, True,  TYPE_8, "CALL C,%04Xh",      op_call_cond    }, // 0xDC
   UNDEFINED1,                                                         // 0xDD
   {0, 1, 0, 0, False, TYPE_8, "SBC A,%02Xh",       op_alu          }, // 0xDE
   {0, 0, 0,-2, False, TYPE_0, "RST 18h",           op_rst          }, // 0xDF

   {0, 0, 2, 0, True,  TYPE_0, "RET PO",            op_ret_cond     }, // 0xE0
   {0, 0, 2, 0, False, TYPE_0, "POP HL",            op_pop          }, // 0xE1
   {0, 2, 0, 0, False, TYPE_8, "JP PO,%04Xh",       op_jp_cond      }, // 0xE2
   {0, 0, 2,-2, False, TYPE_0, "EX (SP),HL",        op_ex_tos_hl    }, // 0xE3
   {0, 2, 0,-2, True,  TYPE_8, "CALL PO,%04Xh",     op_call_cond    }, // 0xE4
   {0, 0, 0,-2, False, TYPE_0, "PUSH HL",           op_push         }, // 0xE5
   {0, 1, 0, 0, False, TYPE_8, "AND %02Xh",         op_alu          }, // 0xE6
   {0, 0, 0,-2, False, TYPE_0, "RST 20h",           op_rst          }, // 0xE7
   {0, 0, 2, 0, True,  TYPE_0, "RET PE",            op_ret_cond     }, // 0xE8
   {0, 0, 0, 0, False, TYPE_0, "JP (HL)",           op_jp_hl        }, // 0xE9
   {0, 2, 0, 0, False, TYPE_8, "JP PE,%04Xh",       op_jp_cond      }, // 0xEA
   {0, 0, 0, 0, False, TYPE_0, "EX DE,HL",          op_ex_de_hl     }, // 0xEB
   {0, 2, 0,-2, True,  TYPE_8, "CALL PE,%04Xh",     op_call_cond    }, // 0xEC
   UNDEFINED1,                                                         // 0xED
   {0, 1, 0, 0, False, TYPE_8, "XOR %02Xh",         op_alu          }, // 0xEE
   {0, 0, 0,-2, False, TYPE_0, "RST 28h",           op_rst          }, // 0xEF

   {0, 0, 2, 0, True,  TYPE_0, "RET P",             op_ret_cond     }, // 0xF0
   {0, 0, 2, 0, False, TYPE_0, "POP AF",            op_pop          }, // 0xF1
   {0, 2, 0, 0, False, TYPE_8, "JP P,%04Xh",        op_jp_cond      }, // 0xF2
   {0, 0, 0, 0, False, TYPE_0, "DI",                op_di           }, // 0xF3
   {0, 2, 0,-2, True,  TYPE_8, "CALL P,%04Xh",      op_call_cond    }, // 0xF4
   {0, 0, 0,-2, False, TYPE_0, "PUSH AF",           op_push         }, // 0xF5
   {0, 1, 0, 0, False, TYPE_8, "OR %02Xh",          op_alu          }, // 0xF6
   {0, 0, 0,-2, False, TYPE_0, "RST 30h",           op_rst          }, // 0xF7
   {0, 0, 2, 0, True,  TYPE_0, "RET M",             op_ret_cond     }, // 0xF8
   {0, 0, 0, 0, False, TYPE_0, "LD SP,HL",          op_load_sp_hl   }, // 0xF9
   {0, 2, 0, 0, False, TYPE_8, "JP M,%04Xh",        op_jp_cond      }, // 0xFA
   {0, 0, 0, 0, False, TYPE_0, "EI",                op_ei           }, // 0xFB
   {0, 2, 0,-2, True,  TYPE_8, "CALL M,%04Xh",      op_call_cond    }, // 0xFC
   UNDEFINED1,                                                         // 0xFD
   {0, 1, 0, 0, False, TYPE_8, "CP %02Xh",          op_alu          }, // 0xFE
   {0, 0, 0,-2, False, TYPE_0, "RST 38h",           op_rst          }  // 0xFF
};

// Instructions with ED prefix
InstrType extended_instructions[256] = {
   UNDEFINED1,                                                         // 0x00
   UNDEFINED1,                                                         // 0x01
   UNDEFINED1,                                                         // 0x02
   UNDEFINED1,                                                         // 0x03
   UNDEFINED1,                                                         // 0x04
   UNDEFINED1,                                                         // 0x05
   UNDEFINED1,                                                         // 0x06
   UNDEFINED1,                                                         // 0x07
   UNDEFINED1,                                                         // 0x08
   UNDEFINED1,                                                         // 0x09
   UNDEFINED1,                                                         // 0x0A
   UNDEFINED1,                                                         // 0x0B
   UNDEFINED1,                                                         // 0x0C
   UNDEFINED1,                                                         // 0x0D
   UNDEFINED1,                                                         // 0x0E
   UNDEFINED1,                                                         // 0x0F

   UNDEFINED1,                                                         // 0x10
   UNDEFINED1,                                                         // 0x11
   UNDEFINED1,                                                         // 0x12
   UNDEFINED1,                                                         // 0x13
   UNDEFINED1,                                                         // 0x14
   UNDEFINED1,                                                         // 0x15
   UNDEFINED1,                                                         // 0x16
   UNDEFINED1,                                                         // 0x17
   UNDEFINED1,                                                         // 0x18
   UNDEFINED1,                                                         // 0x19
   UNDEFINED1,                                                         // 0x1A
   UNDEFINED1,                                                         // 0x1B
   UNDEFINED1,                                                         // 0x1C
   UNDEFINED1,                                                         // 0x1D
   UNDEFINED1,                                                         // 0x1E
   UNDEFINED1,                                                         // 0x1F

   UNDEFINED1,                                                         // 0x20
   UNDEFINED1,                                                         // 0x21
   UNDEFINED1,                                                         // 0x22
   UNDEFINED1,                                                         // 0x23
   UNDEFINED1,                                                         // 0x24
   UNDEFINED1,                                                         // 0x25
   UNDEFINED1,                                                         // 0x26
   UNDEFINED1,                                                         // 0x27
   UNDEFINED1,                                                         // 0x28
   UNDEFINED1,                                                         // 0x29
   UNDEFINED1,                                                         // 0x2A
   UNDEFINED1,                                                         // 0x2B
   UNDEFINED1,                                                         // 0x2C
   UNDEFINED1,                                                         // 0x2D
   UNDEFINED1,                                                         // 0x2E
   UNDEFINED1,                                                         // 0x2F

   UNDEFINED1,                                                         // 0x30
   UNDEFINED1,                                                         // 0x31
   UNDEFINED1,                                                         // 0x32
   UNDEFINED1,                                                         // 0x33
   UNDEFINED1,                                                         // 0x34
   UNDEFINED1,                                                         // 0x35
   UNDEFINED1,                                                         // 0x36
   UNDEFINED1,                                                         // 0x37
   UNDEFINED1,                                                         // 0x38
   UNDEFINED1,                                                         // 0x39
   UNDEFINED1,                                                         // 0x3A
   UNDEFINED1,                                                         // 0x3B
   UNDEFINED1,                                                         // 0x3C
   UNDEFINED1,                                                         // 0x3D
   UNDEFINED1,                                                         // 0x3E
   UNDEFINED1,                                                         // 0x3F

   {0, 0, 1, 0, False, TYPE_0, "IN B,(C)",          op_in_r_c       }, // 0x40
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),B",         op_out_c_r      }, // 0x41
   {0, 0, 0, 0, False, TYPE_0, "SBC HL,BC",         op_sbc_hl_rr    }, // 0x42
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),BC",     op_store_mem16  }, // 0x43
   {0, 0, 0, 0, False, TYPE_0, "NEG",               op_neg          }, // 0x44
   {0, 0, 2, 0, False, TYPE_0, "RETN",              op_retn         }, // 0x45
   {0, 0, 0, 0, False, TYPE_0, "IM 0",              op_im           }, // 0x46
   {0, 0, 0, 0, False, TYPE_0, "LD I,A",            op_load_i_a     }, // 0x47
   {0, 0, 1, 0, False, TYPE_0, "IN C,(C)",          op_in_r_c       }, // 0x48
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),C",         op_out_c_r      }, // 0x49
   {0, 0, 0, 0, False, TYPE_0, "ADC HL,BC",         op_adc_hl_rr    }, // 0x4A
   {0, 2, 2, 0, False, TYPE_8, "LD BC,(%04Xh)",     op_load_mem16   }, // 0x4B
   {0, 0, 0, 0, False, TYPE_0, "NEG",               op_neg          }, // 0x4C
   {0, 0, 2, 0, False, TYPE_0, "RETI",              op_retn         }, // 0x4D
   {0, 0, 0, 0, False, TYPE_0, "IM 0/1",            op_im           }, // 0x4E
   {0, 0, 0, 0, False, TYPE_0, "LD R,A",            op_load_r_a     }, // 0x4F

   {0, 0, 1, 0, False, TYPE_0, "IN D,(C)",          op_in_r_c       }, // 0x50
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),D",         op_out_c_r      }, // 0x51
   {0, 0, 0, 0, False, TYPE_0, "SBC HL,DE",         op_sbc_hl_rr    }, // 0x52
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),DE",     op_store_mem16  }, // 0x53
   {0, 0, 0, 0, False, TYPE_0, "NEG",               op_neg          }, // 0x54
   {0, 0, 2, 0, False, TYPE_0, "RETN",              op_retn         }, // 0x55
   {0, 0, 0, 0, False, TYPE_0, "IM 1",              op_im           }, // 0x56
   {0, 0, 0, 0, False, TYPE_0, "LD A,I",            op_load_a_i     }, // 0x57
   {0, 0, 1, 0, False, TYPE_0, "IN E,(C)",          op_in_r_c       }, // 0x58
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),E",         op_out_c_r      }, // 0x59
   {0, 0, 0, 0, False, TYPE_0, "ADC HL,DE",         op_adc_hl_rr    }, // 0x5A
   {0, 2, 2, 0, False, TYPE_8, "LD DE,(%04Xh)",     op_load_mem16   }, // 0x5B
   {0, 0, 0, 0, False, TYPE_0, "NEG",               op_neg          }, // 0x5C
   {0, 0, 2, 0, False, TYPE_0, "RETN",              op_retn         }, // 0x5D
   {0, 0, 0, 0, False, TYPE_0, "IM 2",              op_im           }, // 0x5E
   {0, 0, 0, 0, False, TYPE_0, "LD A,R",            op_load_a_r     }, // 0x5F

   {0, 0, 1, 0, False, TYPE_0, "IN H,(C)",          op_in_r_c       }, // 0x60
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),H",         op_out_c_r      }, // 0x61
   {0, 0, 0, 0, False, TYPE_0, "SBC HL,HL",         op_sbc_hl_rr    }, // 0x62
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),HL",     op_store_mem16  }, // 0x63
   {0, 0, 0, 0, False, TYPE_0, "NEG",               op_neg          }, // 0x64
   {0, 0, 2, 0, False, TYPE_0, "RETN",              op_retn         }, // 0x65
   {0, 0, 0, 0, False, TYPE_0, "IM 0",              op_im           }, // 0x66
   {0, 0, 1, 1, False, TYPE_0, "RRD",               op_rrd          }, // 0x67
   {0, 0, 1, 0, False, TYPE_0, "IN L,(C)",          op_in_r_c       }, // 0x68
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),L",         op_out_c_r      }, // 0x69
   {0, 0, 0, 0, False, TYPE_0, "ADC HL,HL",         op_adc_hl_rr    }, // 0x6A
   {0, 2, 2, 0, False, TYPE_8, "LD HL,(%04Xh)",     op_load_mem16   }, // 0x6B
   {0, 0, 0, 0, False, TYPE_0, "NEG",               op_neg          }, // 0x6C
   {0, 0, 2, 0, False, TYPE_0, "RETN",              op_retn         }, // 0x6D
   {0, 0, 0, 0, False, TYPE_0, "IM 0/1",            op_im           }, // 0x6E
   {0, 0, 1, 1, False, TYPE_0, "RLD",               op_rld          }, // 0x6F

   {0, 0, 1, 0, False, TYPE_0, "IN (C)",            op_in_r_c       }, // 0x70
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),0",         op_out_c_r      }, // 0x71
   {0, 0, 0, 0, False, TYPE_0, "SBC HL,SP",         op_sbc_hl_rr    }, // 0x72
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),SP",     op_store_mem16  }, // 0x73
   {0, 0, 0, 0, False, TYPE_0, "NEG",               op_neg          }, // 0x74
   {0, 0, 2, 0, False, TYPE_0, "RETN",              op_retn         }, // 0x75
   {0, 0, 0, 0, False, TYPE_0, "IM 1",              op_im           }, // 0x76
   UNDEFINED1,                                                         // 0x77
   {0, 0, 1, 0, False, TYPE_0, "IN A,(C)",          op_in_r_c       }, // 0x78
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),A",         op_out_c_r      }, // 0x79
   {0, 0, 0, 0, False, TYPE_0, "ADC HL,SP",         op_adc_hl_rr    }, // 0x7A
   {0, 2, 2, 0, False, TYPE_8, "LD SP,(%04Xh)",     op_load_mem16   }, // 0x7B
   {0, 0, 0, 0, False, TYPE_0, "NEG",               op_neg          }, // 0x7C
   {0, 0, 2, 0, False, TYPE_0, "RETN",              op_retn         }, // 0x7D
   {0, 0, 0, 0, False, TYPE_0, "IM 2",              op_im           }, // 0x7E
   UNDEFINED1,                                                         // 0x7F

   UNDEFINED1,                                                         // 0x80
   UNDEFINED1,                                                         // 0x81
   UNDEFINED1,                                                         // 0x82
   UNDEFINED1,                                                         // 0x83
   UNDEFINED1,                                                         // 0x84
   UNDEFINED1,                                                         // 0x85
   UNDEFINED1,                                                         // 0x86
   UNDEFINED1,                                                         // 0x87
   UNDEFINED1,                                                         // 0x88
   UNDEFINED1,                                                         // 0x89
   UNDEFINED1,                                                         // 0x8A
   UNDEFINED1,                                                         // 0x8B
   UNDEFINED1,                                                         // 0x8C
   UNDEFINED1,                                                         // 0x8D
   UNDEFINED1,                                                         // 0x8E
   UNDEFINED1,                                                         // 0x8F

   UNDEFINED1,                                                         // 0x90
   UNDEFINED1,                                                         // 0x91
   UNDEFINED1,                                                         // 0x92
   UNDEFINED1,                                                         // 0x93
   UNDEFINED1,                                                         // 0x94
   UNDEFINED1,                                                         // 0x95
   UNDEFINED1,                                                         // 0x96
   UNDEFINED1,                                                         // 0x97
   UNDEFINED1,                                                         // 0x98
   UNDEFINED1,                                                         // 0x99
   UNDEFINED1,                                                         // 0x9A
   UNDEFINED1,                                                         // 0x9B
   UNDEFINED1,                                                         // 0x9C
   UNDEFINED1,                                                         // 0x9D
   UNDEFINED1,                                                         // 0x9E
   UNDEFINED1,                                                         // 0x9F

   {0, 0, 1, 1, False, TYPE_0, "LDI",               op_ldd_ldi      }, // 0xA0
   {0, 0, 1, 0, False, TYPE_0, "CPI",               op_cpd_cpi      }, // 0xA1
   {0, 0, 1, 1, False, TYPE_0, "INI",               op_ind_ini      }, // 0xA2
   {0, 0, 1, 1, False, TYPE_0, "OUTI",              op_outd_outi    }, // 0xA3
   UNDEFINED1,                                                         // 0xA4
   UNDEFINED1,                                                         // 0xA5
   UNDEFINED1,                                                         // 0xA6
   UNDEFINED1,                                                         // 0xA7
   {0, 0, 1, 1, False, TYPE_0, "LDD",               op_ldd_ldi      }, // 0xA8
   {0, 0, 1, 0, False, TYPE_0, "CPD",               op_cpd_cpi      }, // 0xA9
   {0, 0, 1, 1, False, TYPE_0, "IND",               op_ind_ini      }, // 0xAA
   {0, 0, 1, 1, False, TYPE_0, "OUTD",              op_outd_outi    }, // 0xAB
   UNDEFINED1,                                                         // 0xAC
   UNDEFINED1,                                                         // 0xAD
   UNDEFINED1,                                                         // 0xAE
   UNDEFINED1,                                                         // 0xAF

   {0, 0, 1, 1, False, TYPE_0, "LDIR",              op_ldd_ldi      }, // 0xB0
   {0, 0, 1, 0, False, TYPE_0, "CPIR",              op_cpd_cpi      }, // 0xB1
   {0, 0, 1, 1, False, TYPE_0, "INIR",              op_ind_ini      }, // 0xB2
   {0, 0, 1, 1, False, TYPE_0, "OTIR",              op_outd_outi    }, // 0xB3
   UNDEFINED1,                                                         // 0xB4
   UNDEFINED1,                                                         // 0xB5
   UNDEFINED1,                                                         // 0xB6
   UNDEFINED1,                                                         // 0xB7
   {0, 0, 1, 1, False, TYPE_0, "LDDR",              op_ldd_ldi      }, // 0xB8
   {0, 0, 1, 0, False, TYPE_0, "CPDR",              op_cpd_cpi      }, // 0xB9
   {0, 0, 1, 1, False, TYPE_0, "INDR",              op_ind_ini      }, // 0xBA
   {0, 0, 1, 1, False, TYPE_0, "OTDR",              op_outd_outi    }, // 0xBB
   UNDEFINED1,                                                         // 0xBC
   UNDEFINED1,                                                         // 0xBD
   UNDEFINED1,                                                         // 0xBE
   UNDEFINED1,                                                         // 0xBF

   UNDEFINED1,                                                         // 0xC0
   UNDEFINED1,                                                         // 0xC1
   UNDEFINED1,                                                         // 0xC2
   UNDEFINED1,                                                         // 0xC3
   UNDEFINED1,                                                         // 0xC4
   UNDEFINED1,                                                         // 0xC5
   UNDEFINED1,                                                         // 0xC6
   UNDEFINED1,                                                         // 0xC7
   UNDEFINED1,                                                         // 0xC8
   UNDEFINED1,                                                         // 0xC9
   UNDEFINED1,                                                         // 0xCA
   UNDEFINED1,                                                         // 0xCB
   UNDEFINED1,                                                         // 0xCC
   UNDEFINED1,                                                         // 0xCD
   UNDEFINED1,                                                         // 0xCE
   UNDEFINED1,                                                         // 0xCF

   UNDEFINED1,                                                         // 0xD0
   UNDEFINED1,                                                         // 0xD1
   UNDEFINED1,                                                         // 0xD2
   UNDEFINED1,                                                         // 0xD3
   UNDEFINED1,                                                         // 0xD4
   UNDEFINED1,                                                         // 0xD5
   UNDEFINED1,                                                         // 0xD6
   UNDEFINED1,                                                         // 0xD7
   UNDEFINED1,                                                         // 0xD8
   UNDEFINED1,                                                         // 0xD9
   UNDEFINED1,                                                         // 0xDA
   UNDEFINED1,                                                         // 0xDB
   UNDEFINED1,                                                         // 0xDC
   UNDEFINED1,                                                         // 0xDD
   UNDEFINED1,                                                         // 0xDE
   UNDEFINED1,                                                         // 0xDF

   UNDEFINED1,                                                         // 0xE0
   UNDEFINED1,                                                         // 0xE1
   UNDEFINED1,                                                         // 0xE2
   UNDEFINED1,                                                         // 0xE3
   UNDEFINED1,                                                         // 0xE4
   UNDEFINED1,                                                         // 0xE5
   UNDEFINED1,                                                         // 0xE6
   UNDEFINED1,                                                         // 0xE7
   UNDEFINED1,                                                         // 0xE8
   UNDEFINED1,                                                         // 0xE9
   UNDEFINED1,                                                         // 0xEA
   UNDEFINED1,                                                         // 0xEB
   UNDEFINED1,                                                         // 0xEC
   UNDEFINED1,                                                         // 0xED
   UNDEFINED1,                                                         // 0xEE
   UNDEFINED1,                                                         // 0xEF

   UNDEFINED1,                                                         // 0xF0
   UNDEFINED1,                                                         // 0xF1
   UNDEFINED1,                                                         // 0xF2
   UNDEFINED1,                                                         // 0xF3
   UNDEFINED1,                                                         // 0xF4
   UNDEFINED1,                                                         // 0xF5
   UNDEFINED1,                                                         // 0xF6
   UNDEFINED1,                                                         // 0xF7
   UNDEFINED1,                                                         // 0xF8
   UNDEFINED1,                                                         // 0xF9
   UNDEFINED1,                                                         // 0xFA
   UNDEFINED1,                                                         // 0xFB
   UNDEFINED1,                                                         // 0xFC
   UNDEFINED1,                                                         // 0xFD
   UNDEFINED1,                                                         // 0xFE
   UNDEFINED1                                                          // 0xFF
};

// Instructions with CB prefix
InstrType bit_instructions[256] = {
   {0, 0, 0, 0, False, TYPE_0, "RLC B",             op_bit          }, // 0x00
   {0, 0, 0, 0, False, TYPE_0, "RLC C",             op_bit          }, // 0x01
   {0, 0, 0, 0, False, TYPE_0, "RLC D",             op_bit          }, // 0x02
   {0, 0, 0, 0, False, TYPE_0, "RLC E",             op_bit          }, // 0x03
   {0, 0, 0, 0, False, TYPE_0, "RLC H",             op_bit          }, // 0x04
   {0, 0, 0, 0, False, TYPE_0, "RLC L",             op_bit          }, // 0x05
   {0, 0, 1, 1, False, TYPE_0, "RLC (HL)",          op_bit          }, // 0x06
   {0, 0, 0, 0, False, TYPE_0, "RLC A",             op_bit          }, // 0x07
   {0, 0, 0, 0, False, TYPE_0, "RRC B",             op_bit          }, // 0x08
   {0, 0, 0, 0, False, TYPE_0, "RRC C",             op_bit          }, // 0x09
   {0, 0, 0, 0, False, TYPE_0, "RRC D",             op_bit          }, // 0x0A
   {0, 0, 0, 0, False, TYPE_0, "RRC E",             op_bit          }, // 0x0B
   {0, 0, 0, 0, False, TYPE_0, "RRC H",             op_bit          }, // 0x0C
   {0, 0, 0, 0, False, TYPE_0, "RRC L",             op_bit          }, // 0x0D
   {0, 0, 1, 1, False, TYPE_0, "RRC (HL)",          op_bit          }, // 0x0E
   {0, 0, 0, 0, False, TYPE_0, "RRC A",             op_bit          }, // 0x0F

   {0, 0, 0, 0, False, TYPE_0, "RL B",              op_bit          }, // 0x10
   {0, 0, 0, 0, False, TYPE_0, "RL C",              op_bit          }, // 0x11
   {0, 0, 0, 0, False, TYPE_0, "RL D",              op_bit          }, // 0x12
   {0, 0, 0, 0, False, TYPE_0, "RL E",              op_bit          }, // 0x13
   {0, 0, 0, 0, False, TYPE_0, "RL H",              op_bit          }, // 0x14
   {0, 0, 0, 0, False, TYPE_0, "RL L",              op_bit          }, // 0x15
   {0, 0, 1, 1, False, TYPE_0, "RL (HL)",           op_bit          }, // 0x16
   {0, 0, 0, 0, False, TYPE_0, "RL A",              op_bit          }, // 0x17
   {0, 0, 0, 0, False, TYPE_0, "RR B",              op_bit          }, // 0x18
   {0, 0, 0, 0, False, TYPE_0, "RR C",              op_bit          }, // 0x19
   {0, 0, 0, 0, False, TYPE_0, "RR D",              op_bit          }, // 0x1A
   {0, 0, 0, 0, False, TYPE_0, "RR E",              op_bit          }, // 0x1B
   {0, 0, 0, 0, False, TYPE_0, "RR H",              op_bit          }, // 0x1C
   {0, 0, 0, 0, False, TYPE_0, "RR L",              op_bit          }, // 0x1D
   {0, 0, 1, 1, False, TYPE_0, "RR (HL)",           op_bit          }, // 0x1E
   {0, 0, 0, 0, False, TYPE_0, "RR A",              op_bit          }, // 0x1F

   {0, 0, 0, 0, False, TYPE_0, "SLA B",             op_bit          }, // 0x20
   {0, 0, 0, 0, False, TYPE_0, "SLA C",             op_bit          }, // 0x21
   {0, 0, 0, 0, False, TYPE_0, "SLA D",             op_bit          }, // 0x22
   {0, 0, 0, 0, False, TYPE_0, "SLA E",             op_bit          }, // 0x23
   {0, 0, 0, 0, False, TYPE_0, "SLA H",             op_bit          }, // 0x24
   {0, 0, 0, 0, False, TYPE_0, "SLA L",             op_bit          }, // 0x25
   {0, 0, 1, 1, False, TYPE_0, "SLA (HL)",          op_bit          }, // 0x26
   {0, 0, 0, 0, False, TYPE_0, "SLA A",             op_bit          }, // 0x27
   {0, 0, 0, 0, False, TYPE_0, "SRA B",             op_bit          }, // 0x28
   {0, 0, 0, 0, False, TYPE_0, "SRA C",             op_bit          }, // 0x29
   {0, 0, 0, 0, False, TYPE_0, "SRA D",             op_bit          }, // 0x2A
   {0, 0, 0, 0, False, TYPE_0, "SRA E",             op_bit          }, // 0x2B
   {0, 0, 0, 0, False, TYPE_0, "SRA H",             op_bit          }, // 0x2C
   {0, 0, 0, 0, False, TYPE_0, "SRA L",             op_bit          }, // 0x2D
   {0, 0, 1, 1, False, TYPE_0, "SRA (HL)",          op_bit          }, // 0x2E
   {0, 0, 0, 0, False, TYPE_0, "SRA A",             op_bit          }, // 0x2F

   {0, 0, 0, 0, False, TYPE_0, "SLL B",             op_bit          }, // 0x30
   {0, 0, 0, 0, False, TYPE_0, "SLL C",             op_bit          }, // 0x31
   {0, 0, 0, 0, False, TYPE_0, "SLL D",             op_bit          }, // 0x32
   {0, 0, 0, 0, False, TYPE_0, "SLL E",             op_bit          }, // 0x33
   {0, 0, 0, 0, False, TYPE_0, "SLL H",             op_bit          }, // 0x34
   {0, 0, 0, 0, False, TYPE_0, "SLL L",             op_bit          }, // 0x35
   {0, 0, 1, 1, False, TYPE_0, "SLL (HL)",          op_bit          }, // 0x36
   {0, 0, 0, 0, False, TYPE_0, "SLL A",             op_bit          }, // 0x37
   {0, 0, 0, 0, False, TYPE_0, "SRL B",             op_bit          }, // 0x38
   {0, 0, 0, 0, False, TYPE_0, "SRL C",             op_bit          }, // 0x39
   {0, 0, 0, 0, False, TYPE_0, "SRL D",             op_bit          }, // 0x3A
   {0, 0, 0, 0, False, TYPE_0, "SRL E",             op_bit          }, // 0x3B
   {0, 0, 0, 0, False, TYPE_0, "SRL H",             op_bit          }, // 0x3C
   {0, 0, 0, 0, False, TYPE_0, "SRL L",             op_bit          }, // 0x3D
   {0, 0, 1, 1, False, TYPE_0, "SRL (HL)",          op_bit          }, // 0x3E
   {0, 0, 0, 0, False, TYPE_0, "SRL A",             op_bit          }, // 0x3F

   {0, 0, 0, 0, False, TYPE_0, "BIT 0,B",           op_bit          }, // 0x40
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,C",           op_bit          }, // 0x41
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,D",           op_bit          }, // 0x42
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,E",           op_bit          }, // 0x43
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,H",           op_bit          }, // 0x44
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,L",           op_bit          }, // 0x45
   {0, 0, 1, 0, False, TYPE_0, "BIT 0,(HL)",        op_bit          }, // 0x46
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,A",           op_bit          }, // 0x47
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,B",           op_bit          }, // 0x48
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,C",           op_bit          }, // 0x49
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,D",           op_bit          }, // 0x4A
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,E",           op_bit          }, // 0x4B
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,H",           op_bit          }, // 0x4C
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,L",           op_bit          }, // 0x4D
   {0, 0, 1, 0, False, TYPE_0, "BIT 1,(HL)",        op_bit          }, // 0x4E
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,A",           op_bit          }, // 0x4F

   {0, 0, 0, 0, False, TYPE_0, "BIT 2,B",           op_bit          }, // 0x50
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,C",           op_bit          }, // 0x51
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,D",           op_bit          }, // 0x52
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,E",           op_bit          }, // 0x53
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,H",           op_bit          }, // 0x54
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,L",           op_bit          }, // 0x55
   {0, 0, 1, 0, False, TYPE_0, "BIT 2,(HL)",        op_bit          }, // 0x56
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,A",           op_bit          }, // 0x57
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,B",           op_bit          }, // 0x58
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,C",           op_bit          }, // 0x59
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,D",           op_bit          }, // 0x5A
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,E",           op_bit          }, // 0x5B
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,H",           op_bit          }, // 0x5C
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,L",           op_bit          }, // 0x5D
   {0, 0, 1, 0, False, TYPE_0, "BIT 3,(HL)",        op_bit          }, // 0x5E
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,A",           op_bit          }, // 0x5F

   {0, 0, 0, 0, False, TYPE_0, "BIT 4,B",           op_bit          }, // 0x60
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,C",           op_bit          }, // 0x61
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,D",           op_bit          }, // 0x62
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,E",           op_bit          }, // 0x63
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,H",           op_bit          }, // 0x64
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,L",           op_bit          }, // 0x65
   {0, 0, 1, 0, False, TYPE_0, "BIT 4,(HL)",        op_bit          }, // 0x66
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,A",           op_bit          }, // 0x67
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,B",           op_bit          }, // 0x68
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,C",           op_bit          }, // 0x69
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,D",           op_bit          }, // 0x6A
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,E",           op_bit          }, // 0x6B
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,H",           op_bit          }, // 0x6C
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,L",           op_bit          }, // 0x6D
   {0, 0, 1, 0, False, TYPE_0, "BIT 5,(HL)",        op_bit          }, // 0x6E
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,A",           op_bit          }, // 0x6F

   {0, 0, 0, 0, False, TYPE_0, "BIT 6,B",           op_bit          }, // 0x70
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,C",           op_bit          }, // 0x71
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,D",           op_bit          }, // 0x72
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,E",           op_bit          }, // 0x73
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,H",           op_bit          }, // 0x74
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,L",           op_bit          }, // 0x75
   {0, 0, 1, 0, False, TYPE_0, "BIT 6,(HL)",        op_bit          }, // 0x76
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,A",           op_bit          }, // 0x77
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,B",           op_bit          }, // 0x78
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,C",           op_bit          }, // 0x79
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,D",           op_bit          }, // 0x7A
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,E",           op_bit          }, // 0x7B
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,H",           op_bit          }, // 0x7C
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,L",           op_bit          }, // 0x7D
   {0, 0, 1, 0, False, TYPE_0, "BIT 7,(HL)",        op_bit          }, // 0x7E
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,A",           op_bit          }, // 0x7F

   {0, 0, 0, 0, False, TYPE_0, "RES 0,B",           op_bit          }, // 0x80
   {0, 0, 0, 0, False, TYPE_0, "RES 0,C",           op_bit          }, // 0x81
   {0, 0, 0, 0, False, TYPE_0, "RES 0,D",           op_bit          }, // 0x82
   {0, 0, 0, 0, False, TYPE_0, "RES 0,E",           op_bit          }, // 0x83
   {0, 0, 0, 0, False, TYPE_0, "RES 0,H",           op_bit          }, // 0x84
   {0, 0, 0, 0, False, TYPE_0, "RES 0,L",           op_bit          }, // 0x85
   {0, 0, 1, 1, False, TYPE_0, "RES 0,(HL)",        op_bit          }, // 0x86
   {0, 0, 0, 0, False, TYPE_0, "RES 0,A",           op_bit          }, // 0x87
   {0, 0, 0, 0, False, TYPE_0, "RES 1,B",           op_bit          }, // 0x88
   {0, 0, 0, 0, False, TYPE_0, "RES 1,C",           op_bit          }, // 0x89
   {0, 0, 0, 0, False, TYPE_0, "RES 1,D",           op_bit          }, // 0x8A
   {0, 0, 0, 0, False, TYPE_0, "RES 1,E",           op_bit          }, // 0x8B
   {0, 0, 0, 0, False, TYPE_0, "RES 1,H",           op_bit          }, // 0x8C
   {0, 0, 0, 0, False, TYPE_0, "RES 1,L",           op_bit          }, // 0x8D
   {0, 0, 1, 1, False, TYPE_0, "RES 1,(HL)",        op_bit          }, // 0x8E
   {0, 0, 0, 0, False, TYPE_0, "RES 1,A",           op_bit          }, // 0x8F

   {0, 0, 0, 0, False, TYPE_0, "RES 2,B",           op_bit          }, // 0x90
   {0, 0, 0, 0, False, TYPE_0, "RES 2,C",           op_bit          }, // 0x91
   {0, 0, 0, 0, False, TYPE_0, "RES 2,D",           op_bit          }, // 0x92
   {0, 0, 0, 0, False, TYPE_0, "RES 2,E",           op_bit          }, // 0x93
   {0, 0, 0, 0, False, TYPE_0, "RES 2,H",           op_bit          }, // 0x94
   {0, 0, 0, 0, False, TYPE_0, "RES 2,L",           op_bit          }, // 0x95
   {0, 0, 1, 1, False, TYPE_0, "RES 2,(HL)",        op_bit          }, // 0x96
   {0, 0, 0, 0, False, TYPE_0, "RES 2,A",           op_bit          }, // 0x97
   {0, 0, 0, 0, False, TYPE_0, "RES 3,B",           op_bit          }, // 0x98
   {0, 0, 0, 0, False, TYPE_0, "RES 3,C",           op_bit          }, // 0x99
   {0, 0, 0, 0, False, TYPE_0, "RES 3,D",           op_bit          }, // 0x9A
   {0, 0, 0, 0, False, TYPE_0, "RES 3,E",           op_bit          }, // 0x9B
   {0, 0, 0, 0, False, TYPE_0, "RES 3,H",           op_bit          }, // 0x9C
   {0, 0, 0, 0, False, TYPE_0, "RES 3,L",           op_bit          }, // 0x9D
   {0, 0, 1, 1, False, TYPE_0, "RES 3,(HL)",        op_bit          }, // 0x9E
   {0, 0, 0, 0, False, TYPE_0, "RES 3,A",           op_bit          }, // 0x9F

   {0, 0, 0, 0, False, TYPE_0, "RES 4,B",           op_bit          }, // 0xA0
   {0, 0, 0, 0, False, TYPE_0, "RES 4,C",           op_bit          }, // 0xA1
   {0, 0, 0, 0, False, TYPE_0, "RES 4,D",           op_bit          }, // 0xA2
   {0, 0, 0, 0, False, TYPE_0, "RES 4,E",           op_bit          }, // 0xA3
   {0, 0, 0, 0, False, TYPE_0, "RES 4,H",           op_bit          }, // 0xA4
   {0, 0, 0, 0, False, TYPE_0, "RES 4,L",           op_bit          }, // 0xA5
   {0, 0, 1, 1, False, TYPE_0, "RES 4,(HL)",        op_bit          }, // 0xA6
   {0, 0, 0, 0, False, TYPE_0, "RES 4,A",           op_bit          }, // 0xA7
   {0, 0, 0, 0, False, TYPE_0, "RES 5,B",           op_bit          }, // 0xA8
   {0, 0, 0, 0, False, TYPE_0, "RES 5,C",           op_bit          }, // 0xA9
   {0, 0, 0, 0, False, TYPE_0, "RES 5,D",           op_bit          }, // 0xAA
   {0, 0, 0, 0, False, TYPE_0, "RES 5,E",           op_bit          }, // 0xAB
   {0, 0, 0, 0, False, TYPE_0, "RES 5,H",           op_bit          }, // 0xAC
   {0, 0, 0, 0, False, TYPE_0, "RES 5,L",           op_bit          }, // 0xAD
   {0, 0, 1, 1, False, TYPE_0, "RES 5,(HL)",        op_bit          }, // 0xAE
   {0, 0, 0, 0, False, TYPE_0, "RES 5,A",           op_bit          }, // 0xAF

   {0, 0, 0, 0, False, TYPE_0, "RES 6,B",           op_bit          }, // 0xB0
   {0, 0, 0, 0, False, TYPE_0, "RES 6,C",           op_bit          }, // 0xB1
   {0, 0, 0, 0, False, TYPE_0, "RES 6,D",           op_bit          }, // 0xB2
   {0, 0, 0, 0, False, TYPE_0, "RES 6,E",           op_bit          }, // 0xB3
   {0, 0, 0, 0, False, TYPE_0, "RES 6,H",           op_bit          }, // 0xB4
   {0, 0, 0, 0, False, TYPE_0, "RES 6,L",           op_bit          }, // 0xB5
   {0, 0, 1, 1, False, TYPE_0, "RES 6,(HL)",        op_bit          }, // 0xB6
   {0, 0, 0, 0, False, TYPE_0, "RES 6,A",           op_bit          }, // 0xB7
   {0, 0, 0, 0, False, TYPE_0, "RES 7,B",           op_bit          }, // 0xB8
   {0, 0, 0, 0, False, TYPE_0, "RES 7,C",           op_bit          }, // 0xB9
   {0, 0, 0, 0, False, TYPE_0, "RES 7,D",           op_bit          }, // 0xBA
   {0, 0, 0, 0, False, TYPE_0, "RES 7,E",           op_bit          }, // 0xBB
   {0, 0, 0, 0, False, TYPE_0, "RES 7,H",           op_bit          }, // 0xBC
   {0, 0, 0, 0, False, TYPE_0, "RES 7,L",           op_bit          }, // 0xBD
   {0, 0, 1, 1, False, TYPE_0, "RES 7,(HL)",        op_bit          }, // 0xBE
   {0, 0, 0, 0, False, TYPE_0, "RES 7,A",           op_bit          }, // 0xBF

   {0, 0, 0, 0, False, TYPE_0, "SET 0,B",           op_bit          }, // 0xC0
   {0, 0, 0, 0, False, TYPE_0, "SET 0,C",           op_bit          }, // 0xC1
   {0, 0, 0, 0, False, TYPE_0, "SET 0,D",           op_bit          }, // 0xC2
   {0, 0, 0, 0, False, TYPE_0, "SET 0,E",           op_bit          }, // 0xC3
   {0, 0, 0, 0, False, TYPE_0, "SET 0,H",           op_bit          }, // 0xC4
   {0, 0, 0, 0, False, TYPE_0, "SET 0,L",           op_bit          }, // 0xC5
   {0, 0, 1, 1, False, TYPE_0, "SET 0,(HL)",        op_bit          }, // 0xC6
   {0, 0, 0, 0, False, TYPE_0, "SET 0,A",           op_bit          }, // 0xC7
   {0, 0, 0, 0, False, TYPE_0, "SET 1,B",           op_bit          }, // 0xC8
   {0, 0, 0, 0, False, TYPE_0, "SET 1,C",           op_bit          }, // 0xC9
   {0, 0, 0, 0, False, TYPE_0, "SET 1,D",           op_bit          }, // 0xCA
   {0, 0, 0, 0, False, TYPE_0, "SET 1,E",           op_bit          }, // 0xCB
   {0, 0, 0, 0, False, TYPE_0, "SET 1,H",           op_bit          }, // 0xCC
   {0, 0, 0, 0, False, TYPE_0, "SET 1,L",           op_bit          }, // 0xCD
   {0, 0, 1, 1, False, TYPE_0, "SET 1,(HL)",        op_bit          }, // 0xCE
   {0, 0, 0, 0, False, TYPE_0, "SET 1,A",           op_bit          }, // 0xCF

   {0, 0, 0, 0, False, TYPE_0, "SET 2,B",           op_bit          }, // 0xD0
   {0, 0, 0, 0, False, TYPE_0, "SET 2,C",           op_bit          }, // 0xD1
   {0, 0, 0, 0, False, TYPE_0, "SET 2,D",           op_bit          }, // 0xD2
   {0, 0, 0, 0, False, TYPE_0, "SET 2,E",           op_bit          }, // 0xD3
   {0, 0, 0, 0, False, TYPE_0, "SET 2,H",           op_bit          }, // 0xD4
   {0, 0, 0, 0, False, TYPE_0, "SET 2,L",           op_bit          }, // 0xD5
   {0, 0, 1, 1, False, TYPE_0, "SET 2,(HL)",        op_bit          }, // 0xD6
   {0, 0, 0, 0, False, TYPE_0, "SET 2,A",           op_bit          }, // 0xD7
   {0, 0, 0, 0, False, TYPE_0, "SET 3,B",           op_bit          }, // 0xD8
   {0, 0, 0, 0, False, TYPE_0, "SET 3,C",           op_bit          }, // 0xD9
   {0, 0, 0, 0, False, TYPE_0, "SET 3,D",           op_bit          }, // 0xDA
   {0, 0, 0, 0, False, TYPE_0, "SET 3,E",           op_bit          }, // 0xDB
   {0, 0, 0, 0, False, TYPE_0, "SET 3,H",           op_bit          }, // 0xDC
   {0, 0, 0, 0, False, TYPE_0, "SET 3,L",           op_bit          }, // 0xDD
   {0, 0, 1, 1, False, TYPE_0, "SET 3,(HL)",        op_bit          }, // 0xDE
   {0, 0, 0, 0, False, TYPE_0, "SET 3,A",           op_bit          }, // 0xDF

   {0, 0, 0, 0, False, TYPE_0, "SET 4,B",           op_bit          }, // 0xE0
   {0, 0, 0, 0, False, TYPE_0, "SET 4,C",           op_bit          }, // 0xE1
   {0, 0, 0, 0, False, TYPE_0, "SET 4,D",           op_bit          }, // 0xE2
   {0, 0, 0, 0, False, TYPE_0, "SET 4,E",           op_bit          }, // 0xE3
   {0, 0, 0, 0, False, TYPE_0, "SET 4,H",           op_bit          }, // 0xE4
   {0, 0, 0, 0, False, TYPE_0, "SET 4,L",           op_bit          }, // 0xE5
   {0, 0, 1, 1, False, TYPE_0, "SET 4,(HL)",        op_bit          }, // 0xE6
   {0, 0, 0, 0, False, TYPE_0, "SET 4,A",           op_bit          }, // 0xE7
   {0, 0, 0, 0, False, TYPE_0, "SET 5,B",           op_bit          }, // 0xE8
   {0, 0, 0, 0, False, TYPE_0, "SET 5,C",           op_bit          }, // 0xE9
   {0, 0, 0, 0, False, TYPE_0, "SET 5,D",           op_bit          }, // 0xEA
   {0, 0, 0, 0, False, TYPE_0, "SET 5,E",           op_bit          }, // 0xEB
   {0, 0, 0, 0, False, TYPE_0, "SET 5,H",           op_bit          }, // 0xEC
   {0, 0, 0, 0, False, TYPE_0, "SET 5,L",           op_bit          }, // 0xED
   {0, 0, 1, 1, False, TYPE_0, "SET 5,(HL)",        op_bit          }, // 0xEE
   {0, 0, 0, 0, False, TYPE_0, "SET 5,A",           op_bit          }, // 0xEF

   {0, 0, 0, 0, False, TYPE_0, "SET 6,B",           op_bit          }, // 0xF0
   {0, 0, 0, 0, False, TYPE_0, "SET 6,C",           op_bit          }, // 0xF1
   {0, 0, 0, 0, False, TYPE_0, "SET 6,D",           op_bit          }, // 0xF2
   {0, 0, 0, 0, False, TYPE_0, "SET 6,E",           op_bit          }, // 0xF3
   {0, 0, 0, 0, False, TYPE_0, "SET 6,H",           op_bit          }, // 0xF4
   {0, 0, 0, 0, False, TYPE_0, "SET 6,L",           op_bit          }, // 0xF5
   {0, 0, 1, 1, False, TYPE_0, "SET 6,(HL)",        op_bit          }, // 0xF6
   {0, 0, 0, 0, False, TYPE_0, "SET 6,A",           op_bit          }, // 0xF7
   {0, 0, 0, 0, False, TYPE_0, "SET 7,B",           op_bit          }, // 0xF8
   {0, 0, 0, 0, False, TYPE_0, "SET 7,C",           op_bit          }, // 0xF9
   {0, 0, 0, 0, False, TYPE_0, "SET 7,D",           op_bit          }, // 0xFA
   {0, 0, 0, 0, False, TYPE_0, "SET 7,E",           op_bit          }, // 0xFB
   {0, 0, 0, 0, False, TYPE_0, "SET 7,H",           op_bit          }, // 0xFC
   {0, 0, 0, 0, False, TYPE_0, "SET 7,L",           op_bit          }, // 0xFD
   {0, 0, 1, 1, False, TYPE_0, "SET 7,(HL)",        op_bit          }, // 0xFE
   {0, 0, 0, 0, False, TYPE_0, "SET 7,A",           op_bit          }  // 0xFF
};

// Instructions with DD or FD prefix
InstrType index_instructions[256] = {
   UNDEFINED2,                                                         // 0x00
   UNDEFINED2,                                                         // 0x01
   UNDEFINED2,                                                         // 0x02
   UNDEFINED2,                                                         // 0x03
   UNDEFINED2,                                                         // 0x04
   UNDEFINED2,                                                         // 0x05
   UNDEFINED2,                                                         // 0x06
   UNDEFINED2,                                                         // 0x07
   UNDEFINED2,                                                         // 0x08
   {0, 0, 0, 0, False, TYPE_1, "ADD %s,BC",         op_add_hl_rr    }, // 0x09
   UNDEFINED2,                                                         // 0x0A
   UNDEFINED2,                                                         // 0x0B
   UNDEFINED2,                                                         // 0x0C
   UNDEFINED2,                                                         // 0x0D
   UNDEFINED2,                                                         // 0x0E
   UNDEFINED2,                                                         // 0x0F

   UNDEFINED2,                                                         // 0x10
   UNDEFINED2,                                                         // 0x11
   UNDEFINED2,                                                         // 0x12
   UNDEFINED2,                                                         // 0x13
   UNDEFINED2,                                                         // 0x14
   UNDEFINED2,                                                         // 0x15
   UNDEFINED2,                                                         // 0x16
   UNDEFINED2,                                                         // 0x17
   UNDEFINED2,                                                         // 0x18
   {0, 0, 0, 0, False, TYPE_1, "ADD %s,DE",         op_add_hl_rr    }, // 0x19
   UNDEFINED2,                                                         // 0x1A
   UNDEFINED2,                                                         // 0x1B
   UNDEFINED2,                                                         // 0x1C
   UNDEFINED2,                                                         // 0x1D
   UNDEFINED2,                                                         // 0x1E
   UNDEFINED2,                                                         // 0x1F

   UNDEFINED2,                                                         // 0x20
   {0, 2, 0, 0, False, TYPE_4, "LD %s,%04Xh",       op_load_imm16   }, // 0x21
   {0, 2, 0, 2, False, TYPE_3, "LD (%04Xh),%s",     op_store_mem16  }, // 0x22
   {0, 0, 0, 0, False, TYPE_1, "INC %s",            op_inc_rr       }, // 0x23
   {0, 0, 0, 0, False, TYPE_1, "INC %sh",           op_inc_r        }, // 0x24
   {0, 0, 0, 0, False, TYPE_1, "DEC %sh",           op_dec_r        }, // 0x25
   {0, 1, 0, 0, False, TYPE_4, "LD %sh,%02Xh",      op_load_imm8    }, // 0x26
   UNDEFINED2,                                                         // 0x27
   UNDEFINED2,                                                         // 0x28
   {0, 0, 0, 0, False, TYPE_2, "ADD %s,%s",         op_add_hl_rr    }, // 0x29
   {0, 2, 2, 0, False, TYPE_4, "LD %s,(%04Xh)",     op_load_mem16   }, // 0x2A
   {0, 0, 0, 0, False, TYPE_1, "DEC %s",            op_dec_rr       }, // 0x2B
   {0, 0, 0, 0, False, TYPE_1, "INC %sl",           op_inc_r        }, // 0x2C
   {0, 0, 0, 0, False, TYPE_1, "DEC %sl",           op_dec_r        }, // 0x2D
   {0, 1, 0, 0, False, TYPE_4, "LD %sl,%02Xh",      op_load_imm8    }, // 0x2E
   UNDEFINED2,                                                         // 0x2F

   UNDEFINED2,                                                         // 0x30
   UNDEFINED2,                                                         // 0x31
   UNDEFINED2,                                                         // 0x32
   UNDEFINED2,                                                         // 0x33
   {1, 0, 1, 1, False, TYPE_5, "INC (%s%+d)",       op_inc_idx_disp }, // 0x34
   {1, 0, 1, 1, False, TYPE_5, "DEC (%s%+d)",       op_dec_idx_disp }, // 0x35
   {1, 1, 0, 1, False, TYPE_6, "LD (%s%+d),%02xh",  op_load_idx_disp },// 0x36
   UNDEFINED2,                                                         // 0x37
   UNDEFINED2,                                                         // 0x38
   {0, 0, 0, 0, False, TYPE_1, "ADD %s,SP",         op_add_hl_rr    }, // 0x39
   UNDEFINED2,                                                         // 0x3A
   UNDEFINED2,                                                         // 0x3B
   UNDEFINED2,                                                         // 0x3C
   UNDEFINED2,                                                         // 0x3D
   UNDEFINED2,                                                         // 0x3D
   UNDEFINED2,                                                         // 0x3F

   UNDEFINED2,                                                         // 0x40
   UNDEFINED2,                                                         // 0x41
   UNDEFINED2,                                                         // 0x42
   UNDEFINED2,                                                         // 0x44
   {0, 0, 0, 0, False, TYPE_1, "LD B,%sh",          op_load_reg8    }, // 0x44
   {0, 0, 0, 0, False, TYPE_1, "LD B,%sl",          op_load_reg8    }, // 0x45
   {1, 0, 1, 0, False, TYPE_5, "LD B,(%s%+d)",      op_load_reg8    }, // 0x46
   UNDEFINED2,                                                         // 0x47
   UNDEFINED2,                                                         // 0x48
   UNDEFINED2,                                                         // 0x49
   UNDEFINED2,                                                         // 0x4A
   UNDEFINED2,                                                         // 0x4B
   {0, 0, 0, 0, False, TYPE_1, "LD C,%sh",          op_load_reg8    }, // 0x4C
   {0, 0, 0, 0, False, TYPE_1, "LD C,%sl",          op_load_reg8    }, // 0x4D
   {1, 0, 1, 0, False, TYPE_5, "LD C,(%s%+d)",      op_load_reg8    }, // 0x4E
   UNDEFINED2,                                                         // 0x4F

   UNDEFINED2,                                                         // 0x50
   UNDEFINED2,                                                         // 0x51
   UNDEFINED2,                                                         // 0x52
   UNDEFINED2,                                                         // 0x52
   {0, 0, 0, 0, False, TYPE_1, "LD D,%sh",          op_load_reg8    }, // 0x54
   {0, 0, 0, 0, False, TYPE_1, "LD D,%sl",          op_load_reg8    }, // 0x55
   {1, 0, 1, 0, False, TYPE_5, "LD D,(%s%+d)",      op_load_reg8    }, // 0x56
   UNDEFINED2,                                                         // 0x57
   UNDEFINED2,                                                         // 0x58
   UNDEFINED2,                                                         // 0x59
   UNDEFINED2,                                                         // 0x5A
   UNDEFINED2,                                                         // 0x5B
   {0, 0, 0, 0, False, TYPE_1, "LD E,%sh",          op_load_reg8    }, // 0x5C
   {0, 0, 0, 0, False, TYPE_1, "LD E,%sl",          op_load_reg8    }, // 0x5D
   {1, 0, 1, 0, False, TYPE_5, "LD E,(%s%+d)",      op_load_reg8    }, // 0x5E
   UNDEFINED2,                                                         // 0x5F

   {0, 0, 0, 0, False, TYPE_1, "LD %sh,B",          op_load_reg8    }, // 0x60
   {0, 0, 0, 0, False, TYPE_1, "LD %sh,C",          op_load_reg8    }, // 0x61
   {0, 0, 0, 0, False, TYPE_1, "LD %sh,D",          op_load_reg8    }, // 0x62
   {0, 0, 0, 0, False, TYPE_1, "LD %sh,E",          op_load_reg8    }, // 0x63
   {0, 0, 0, 0, False, TYPE_2, "LD %sh,%sh",        op_load_reg8    }, // 0x64
   {0, 0, 0, 0, False, TYPE_2, "LD %sh,%sl",        op_load_reg8    }, // 0x65
   {1, 0, 1, 0, False, TYPE_5, "LD H,(%s%+d)",      op_load_reg8    }, // 0x66
   {0, 0, 0, 0, False, TYPE_1, "LD %sh,A",          op_load_reg8    }, // 0x67
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,B",          op_load_reg8    }, // 0x68
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,C",          op_load_reg8    }, // 0x69
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,D",          op_load_reg8    }, // 0x6A
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,E",          op_load_reg8    }, // 0x6B
   {0, 0, 0, 0, False, TYPE_2, "LD %sl,%sh",        op_load_reg8    }, // 0x6C
   {0, 0, 0, 0, False, TYPE_2, "LD %sl,%sl",        op_load_reg8    }, // 0x6D
   {1, 0, 1, 0, False, TYPE_5, "LD L,(%s%+d)",      op_load_reg8    }, // 0x6E
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,A",          op_load_reg8    }, // 0x6F

   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),B",      op_load_reg8    }, // 0x70
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),C",      op_load_reg8    }, // 0x71
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),D",      op_load_reg8    }, // 0x72
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),E",      op_load_reg8    }, // 0x73
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),H",      op_load_reg8    }, // 0x74
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),L",      op_load_reg8    }, // 0x75
   UNDEFINED2,                                                         // 0x76
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),A",      op_load_reg8    }, // 0x77
   UNDEFINED2,                                                         // 0x78
   UNDEFINED2,                                                         // 0x79
   UNDEFINED2,                                                         // 0x7A
   UNDEFINED2,                                                         // 0x7B
   {0, 0, 0, 0, False, TYPE_1, "LD A,%sh",          op_load_reg8    }, // 0x7C
   {0, 0, 0, 0, False, TYPE_1, "LD A,%sl",          op_load_reg8    }, // 0x7D
   {1, 0, 1, 0, False, TYPE_5, "LD A,(%s%+d)",      op_load_reg8    }, // 0x7E
   UNDEFINED2,                                                         // 0x7F

   UNDEFINED2,                                                         // 0x80
   UNDEFINED2,                                                         // 0x81
   UNDEFINED2,                                                         // 0x82
   UNDEFINED2,                                                         // 0x83
   {0, 0, 0, 0, False, TYPE_1, "ADD A,%sh",         op_alu          }, // 0x84
   {0, 0, 0, 0, False, TYPE_1, "ADD A,%sl",         op_alu          }, // 0x85
   {1, 0, 1, 0, False, TYPE_5, "ADD A,(%s%+d)",     op_alu          }, // 0x86
   UNDEFINED2,                                                         // 0x87
   UNDEFINED2,                                                         // 0x88
   UNDEFINED2,                                                         // 0x89
   UNDEFINED2,                                                         // 0x8A
   UNDEFINED2,                                                         // 0x8B
   {0, 0, 0, 0, False, TYPE_1, "ADC A,%sh",         op_alu          }, // 0x8C
   {0, 0, 0, 0, False, TYPE_1, "ADC A,%sl",         op_alu          }, // 0x8D
   {1, 0, 1, 0, False, TYPE_5, "ADC A,(%s%+d)",     op_alu          }, // 0x8E
   UNDEFINED2,                                                         // 0x8F

   UNDEFINED2,                                                         // 0x90
   UNDEFINED2,                                                         // 0x91
   UNDEFINED2,                                                         // 0x92
   UNDEFINED2,                                                         // 0x93
   {0, 0, 0, 0, False, TYPE_1, "SUB %sh",           op_alu          }, // 0x94
   {0, 0, 0, 0, False, TYPE_1, "SUB %sl",           op_alu          }, // 0x95
   {1, 0, 1, 0, False, TYPE_5, "SUB (%s%+d)",       op_alu          }, // 0x96
   UNDEFINED2,                                                         // 0x97
   UNDEFINED2,                                                         // 0x98
   UNDEFINED2,                                                         // 0x99
   UNDEFINED2,                                                         // 0x9A
   UNDEFINED2,                                                         // 0x9B
   {0, 0, 0, 0, False, TYPE_1, "SBC A,%sh",         op_alu          }, // 0x9C
   {0, 0, 0, 0, False, TYPE_1, "SBC A,%sl",         op_alu          }, // 0x9D
   {1, 0, 1, 0, False, TYPE_5, "SBC A,(%s%+d)",     op_alu          }, // 0x9E
   UNDEFINED2,                                                         // 0x9F

   UNDEFINED2,                                                         // 0xA0
   UNDEFINED2,                                                         // 0xA1
   UNDEFINED2,                                                         // 0xA2
   UNDEFINED2,                                                         // 0xA3
   {0, 0, 0, 0, False, TYPE_1, "AND %sh",           op_alu          }, // 0xA4
   {0, 0, 0, 0, False, TYPE_1, "AND %sl",           op_alu          }, // 0xA5
   {1, 0, 1, 0, False, TYPE_5, "AND (%s%+d)",       op_alu          }, // 0xA6
   UNDEFINED2,                                                         // 0xA7
   UNDEFINED2,                                                         // 0xA8
   UNDEFINED2,                                                         // 0xA9
   UNDEFINED2,                                                         // 0xAA
   UNDEFINED2,                                                         // 0xAB
   {0, 0, 0, 0, False, TYPE_1, "XOR %sh",           op_alu          }, // 0xAC
   {0, 0, 0, 0, False, TYPE_1, "XOR %sl",           op_alu          }, // 0xAD
   {1, 0, 1, 0, False, TYPE_5, "XOR (%s%+d)",       op_alu          }, // 0xAE
   UNDEFINED2,                                                         // 0xEF

   UNDEFINED2,                                                         // 0xB0
   UNDEFINED2,                                                         // 0xB1
   UNDEFINED2,                                                         // 0xB2
   UNDEFINED2,                                                         // 0xB3
   {0, 0, 0, 0, False, TYPE_1, "OR %sh",            op_alu          }, // 0xB4
   {0, 0, 0, 0, False, TYPE_1, "OR %sl",            op_alu          }, // 0xB5
   {1, 0, 1, 0, False, TYPE_5, "OR (%s%+d)",        op_alu          }, // 0xB6
   UNDEFINED2,                                                         // 0xB7
   UNDEFINED2,                                                         // 0xB8
   UNDEFINED2,                                                         // 0xB9
   UNDEFINED2,                                                         // 0xBA
   UNDEFINED2,                                                         // 0xBB
   {0, 0, 0, 0, False, TYPE_1, "CP %sh",            op_alu          }, // 0xBC
   {0, 0, 0, 0, False, TYPE_1, "CP %sl",            op_alu          }, // 0xBD
   {1, 0, 1, 0, False, TYPE_5, "CP (%s%+d)",        op_alu          }, // 0xBE
   UNDEFINED2,                                                         // 0xBF

   UNDEFINED2,                                                         // 0xC0
   UNDEFINED2,                                                         // 0xC1
   UNDEFINED2,                                                         // 0xC2
   UNDEFINED2,                                                         // 0xC3
   UNDEFINED2,                                                         // 0xC4
   UNDEFINED2,                                                         // 0xC5
   UNDEFINED2,                                                         // 0xC6
   UNDEFINED2,                                                         // 0xC7
   UNDEFINED2,                                                         // 0xC8
   UNDEFINED2,                                                         // 0xC9
   UNDEFINED2,                                                         // 0xCA
   UNDEFINED2,                                                         // 0xCB
   UNDEFINED2,                                                         // 0xCC
   UNDEFINED2,                                                         // 0xCD
   UNDEFINED2,                                                         // 0xCE
   UNDEFINED2,                                                         // 0xCF

   UNDEFINED2,                                                         // 0xD0
   UNDEFINED2,                                                         // 0xD1
   UNDEFINED2,                                                         // 0xD2
   UNDEFINED2,                                                         // 0xD3
   UNDEFINED2,                                                         // 0xD4
   UNDEFINED2,                                                         // 0xD5
   UNDEFINED2,                                                         // 0xD6
   UNDEFINED2,                                                         // 0xD7
   UNDEFINED2,                                                         // 0xD8
   UNDEFINED2,                                                         // 0xD9
   UNDEFINED2,                                                         // 0xDA
   UNDEFINED2,                                                         // 0xDB
   UNDEFINED2,                                                         // 0xDC
   UNDEFINED2,                                                         // 0xDD
   UNDEFINED2,                                                         // 0xDE
   UNDEFINED2,                                                         // 0xDF

   UNDEFINED2,                                                         // 0xE0
   {0, 0, 2, 0, False, TYPE_1, "POP %s",            op_pop          }, // 0xE1
   UNDEFINED2,                                                         // 0xE2
   {0, 0, 2,-2, False, TYPE_1, "EX (SP),%s",        op_ex_tos_hl    }, // 0xE3
   UNDEFINED2,                                                         // 0xE4
   {0, 0, 0,-2, False, TYPE_1, "PUSH %s",           op_push         }, // 0xE5
   UNDEFINED2,                                                         // 0xE6
   UNDEFINED2,                                                         // 0xE7
   UNDEFINED2,                                                         // 0xE8
   {0, 0, 0, 0, False, TYPE_1, "JP (%s)",           op_jp_hl        }, // 0xE9
   UNDEFINED2,                                                         // 0xEA
   UNDEFINED2,                                                         // 0xEB
   UNDEFINED2,                                                         // 0xEC
   UNDEFINED2,                                                         // 0xED
   UNDEFINED2,                                                         // 0xEE
   UNDEFINED2,                                                         // 0xEF

   UNDEFINED2,                                                         // 0xF0
   UNDEFINED2,                                                         // 0xF1
   UNDEFINED2,                                                         // 0xF2
   UNDEFINED2,                                                         // 0xF3
   UNDEFINED2,                                                         // 0xF4
   UNDEFINED2,                                                         // 0xF5
   UNDEFINED2,                                                         // 0xF7
   UNDEFINED2,                                                         // 0xF7
   UNDEFINED2,                                                         // 0xF8
   {0, 0, 0, 0, False, TYPE_1, "LD SP,%s",          op_load_sp_hl   }, // 0xF9
   UNDEFINED2,                                                         // 0xFA
   UNDEFINED2,                                                         // 0xFB
   UNDEFINED2,                                                         // 0xFC
   UNDEFINED2,                                                         // 0xFD
   UNDEFINED2,                                                         // 0xFE
   UNDEFINED2                                                          // 0xFF
};

// Instructions with DD CB or FD CB prefix.
// For these instructions, the displacement precedes the opcode byte.
// This is handled as a special case in the code, and thus the entries
// in this table specify 0 for the displacement length.
InstrType index_bit_instructions[256] = {
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),B",     op_bit          }, // 0x00
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),C",     op_bit          }, // 0x01
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),D",     op_bit          }, // 0x02
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),E",     op_bit          }, // 0x03
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),H",     op_bit          }, // 0x04
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),L",     op_bit          }, // 0x05
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d)",       op_bit          }, // 0x06
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),A",     op_bit          }, // 0x07
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),B",     op_bit          }, // 0x08
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),C",     op_bit          }, // 0x09
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),D",     op_bit          }, // 0x0A
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),E",     op_bit          }, // 0x0B
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),H",     op_bit          }, // 0x0C
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),L",     op_bit          }, // 0x0D
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d)",       op_bit          }, // 0x0E
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),A",     op_bit          }, // 0x0F

   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),B",      op_bit          }, // 0x10
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),C",      op_bit          }, // 0x11
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),D",      op_bit          }, // 0x12
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),E",      op_bit          }, // 0x13
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),H",      op_bit          }, // 0x14
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),L",      op_bit          }, // 0x15
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d)",        op_bit          }, // 0x16
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),A",      op_bit          }, // 0x17
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),B",      op_bit          }, // 0x18
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),C",      op_bit          }, // 0x19
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),D",      op_bit          }, // 0x1A
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),E",      op_bit          }, // 0x1B
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),H",      op_bit          }, // 0x1C
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),L",      op_bit          }, // 0x1D
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d)",        op_bit          }, // 0x1E
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),A",      op_bit          }, // 0x1F

   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),B",     op_bit          }, // 0x20
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),C",     op_bit          }, // 0x21
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),D",     op_bit          }, // 0x22
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),E",     op_bit          }, // 0x23
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),H",     op_bit          }, // 0x24
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),L",     op_bit          }, // 0x25
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d)",       op_bit          }, // 0x26
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),A",     op_bit          }, // 0x27
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),B",     op_bit          }, // 0x28
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),C",     op_bit          }, // 0x29
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),D",     op_bit          }, // 0x2A
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),E",     op_bit          }, // 0x2B
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),H",     op_bit          }, // 0x2C
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),L",     op_bit          }, // 0x2D
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d)",       op_bit          }, // 0x2E
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),A",     op_bit          }, // 0x2F

   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),B",     op_bit          }, // 0x30
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),C",     op_bit          }, // 0x31
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),D",     op_bit          }, // 0x32
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),E",     op_bit          }, // 0x33
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),H",     op_bit          }, // 0x34
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),L",     op_bit          }, // 0x35
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d)",       op_bit          }, // 0x36
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),A",     op_bit          }, // 0x37
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),B",     op_bit          }, // 0x38
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),C",     op_bit          }, // 0x39
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),D",     op_bit          }, // 0x3A
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),E",     op_bit          }, // 0x3B
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),H",     op_bit          }, // 0x3C
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),L",     op_bit          }, // 0x3D
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d)",       op_bit          }, // 0x3E
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),A",     op_bit          }, // 0x3F

   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     op_bit          }, // 0x40
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     op_bit          }, // 0x41
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     op_bit          }, // 0x42
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     op_bit          }, // 0x43
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     op_bit          }, // 0x44
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     op_bit          }, // 0x45
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     op_bit          }, // 0x46
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     op_bit          }, // 0x47
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     op_bit          }, // 0x48
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     op_bit          }, // 0x49
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     op_bit          }, // 0x4A
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     op_bit          }, // 0x4B
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     op_bit          }, // 0x4C
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     op_bit          }, // 0x4D
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     op_bit          }, // 0x4E
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     op_bit          }, // 0x4F

   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     op_bit          }, // 0x50
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     op_bit          }, // 0x51
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     op_bit          }, // 0x52
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     op_bit          }, // 0x53
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     op_bit          }, // 0x54
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     op_bit          }, // 0x55
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     op_bit          }, // 0x56
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     op_bit          }, // 0x57
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     op_bit          }, // 0x58
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     op_bit          }, // 0x59
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     op_bit          }, // 0x5A
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     op_bit          }, // 0x5B
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     op_bit          }, // 0x5C
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     op_bit          }, // 0x5D
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     op_bit          }, // 0x5E
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     op_bit          }, // 0x5F

   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     op_bit          }, // 0x60
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     op_bit          }, // 0x61
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     op_bit          }, // 0x62
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     op_bit          }, // 0x63
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     op_bit          }, // 0x64
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     op_bit          }, // 0x65
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     op_bit          }, // 0x66
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     op_bit          }, // 0x67
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     op_bit          }, // 0x68
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     op_bit          }, // 0x69
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     op_bit          }, // 0x6A
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     op_bit          }, // 0x6B
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     op_bit          }, // 0x6C
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     op_bit          }, // 0x6D
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     op_bit          }, // 0x6E
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     op_bit          }, // 0x6F

   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     op_bit          }, // 0x70
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     op_bit          }, // 0x71
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     op_bit          }, // 0x72
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     op_bit          }, // 0x73
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     op_bit          }, // 0x74
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     op_bit          }, // 0x75
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     op_bit          }, // 0x76
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     op_bit          }, // 0x77
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     op_bit          }, // 0x78
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     op_bit          }, // 0x79
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     op_bit          }, // 0x7A
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     op_bit          }, // 0x7B
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     op_bit          }, // 0x7C
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     op_bit          }, // 0x7D
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     op_bit          }, // 0x7E
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     op_bit          }, // 0x7F

   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),B",   op_bit          }, // 0x80
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),C",   op_bit          }, // 0x81
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),D",   op_bit          }, // 0x82
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),E",   op_bit          }, // 0x83
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),H",   op_bit          }, // 0x84
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),L",   op_bit          }, // 0x85
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d)",     op_bit          }, // 0x86
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),A",   op_bit          }, // 0x87
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),B",   op_bit          }, // 0x88
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),C",   op_bit          }, // 0x89
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),D",   op_bit          }, // 0x8A
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),E",   op_bit          }, // 0x8B
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),H",   op_bit          }, // 0x8C
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),L",   op_bit          }, // 0x8D
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d)",     op_bit          }, // 0x8E
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),A",   op_bit          }, // 0x8F

   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),B",   op_bit          }, // 0x90
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),C",   op_bit          }, // 0x91
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),D",   op_bit          }, // 0x92
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),E",   op_bit          }, // 0x93
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),H",   op_bit          }, // 0x94
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),L",   op_bit          }, // 0x95
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d)",     op_bit          }, // 0x96
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),A",   op_bit          }, // 0x97
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),B",   op_bit          }, // 0x98
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),C",   op_bit          }, // 0x99
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),D",   op_bit          }, // 0x9A
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),E",   op_bit          }, // 0x9B
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),H",   op_bit          }, // 0x9C
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),L",   op_bit          }, // 0x9D
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d)",     op_bit          }, // 0x9E
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),A",   op_bit          }, // 0x9F

   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),B",   op_bit          }, // 0xA0
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),C",   op_bit          }, // 0xA1
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),D",   op_bit          }, // 0xA2
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),E",   op_bit          }, // 0xA3
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),H",   op_bit          }, // 0xA4
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),L",   op_bit          }, // 0xA5
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d)",     op_bit          }, // 0xA6
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),A",   op_bit          }, // 0xA7
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),B",   op_bit          }, // 0xA8
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),C",   op_bit          }, // 0xA9
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),D",   op_bit          }, // 0xAA
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),E",   op_bit          }, // 0xAB
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),H",   op_bit          }, // 0xAC
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),L",   op_bit          }, // 0xAD
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d)",     op_bit          }, // 0xAE
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),A",   op_bit          }, // 0xAF

   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),B",   op_bit          }, // 0xB0
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),C",   op_bit          }, // 0xB1
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),D",   op_bit          }, // 0xB2
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),E",   op_bit          }, // 0xB3
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),H",   op_bit          }, // 0xB4
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),L",   op_bit          }, // 0xB5
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d)",     op_bit          }, // 0xB6
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),A",   op_bit          }, // 0xB7
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),B",   op_bit          }, // 0xB8
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),C",   op_bit          }, // 0xB9
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),D",   op_bit          }, // 0xBA
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),E",   op_bit          }, // 0xBB
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),H",   op_bit          }, // 0xBC
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),L",   op_bit          }, // 0xBD
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d)",     op_bit          }, // 0xBE
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),A",   op_bit          }, // 0xBF

   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),B",   op_bit          }, // 0xC0
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),C",   op_bit          }, // 0xC1
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),D",   op_bit          }, // 0xC2
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),E",   op_bit          }, // 0xC3
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),H",   op_bit          }, // 0xC4
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),L",   op_bit          }, // 0xC5
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d)",     op_bit          }, // 0xC6
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),A",   op_bit          }, // 0xC7
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),B",   op_bit          }, // 0xC8
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),C",   op_bit          }, // 0xC9
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),D",   op_bit          }, // 0xCA
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),E",   op_bit          }, // 0xCB
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),H",   op_bit          }, // 0xCC
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),L",   op_bit          }, // 0xCD
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d)",     op_bit          }, // 0xCE
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),A",   op_bit          }, // 0xCF

   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),B",   op_bit          }, // 0xD0
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),C",   op_bit          }, // 0xD1
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),D",   op_bit          }, // 0xD2
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),E",   op_bit          }, // 0xD3
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),H",   op_bit          }, // 0xD4
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),L",   op_bit          }, // 0xD5
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d)",     op_bit          }, // 0xD6
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),A",   op_bit          }, // 0xD7
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),B",   op_bit          }, // 0xD8
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),C",   op_bit          }, // 0xD9
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),D",   op_bit          }, // 0xDA
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),E",   op_bit          }, // 0xDB
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),H",   op_bit          }, // 0xDC
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),L",   op_bit          }, // 0xDD
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d)",     op_bit          }, // 0xDE
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),A",   op_bit          }, // 0xDF

   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),B",   op_bit          }, // 0xE0
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),C",   op_bit          }, // 0xE1
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),D",   op_bit          }, // 0xE2
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),E",   op_bit          }, // 0xE3
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),H",   op_bit          }, // 0xE4
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),L",   op_bit          }, // 0xE5
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d)",     op_bit          }, // 0xE6
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),A",   op_bit          }, // 0xE7
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),B",   op_bit          }, // 0xE8
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),C",   op_bit          }, // 0xE9
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),D",   op_bit          }, // 0xEA
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),E",   op_bit          }, // 0xEB
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),H",   op_bit          }, // 0xEC
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),L",   op_bit          }, // 0xED
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d)",     op_bit          }, // 0xEE
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),A",   op_bit          }, // 0xEF

   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),B",   op_bit          }, // 0xF0
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),C",   op_bit          }, // 0xF1
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),D",   op_bit          }, // 0xF2
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),E",   op_bit          }, // 0xF3
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),H",   op_bit          }, // 0xF4
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),L",   op_bit          }, // 0xF5
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d)",     op_bit          }, // 0xF6
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),A",   op_bit          }, // 0xF7
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),B",   op_bit          }, // 0xF8
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),C",   op_bit          }, // 0xF9
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),D",   op_bit          }, // 0xFA
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),E",   op_bit          }, // 0xFB
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),H",   op_bit          }, // 0xFC
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),L",   op_bit          }, // 0xFD
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d)",     op_bit          }, // 0xFE
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),A",   op_bit          }  // 0xFF
};


InstrType z80_interrupt_int =
{0, 0, 0, -2, False, TYPE_0, "INT", op_interrupt_int };

InstrType z80_interrupt_nmi =
{0, 0, 0, -2, False, TYPE_0, "NMI", op_interrupt_nmi };

InstrType *table_by_prefix(int prefix) {
   switch (prefix) {
   case 0:
      return main_instructions;
   case 0xED:
      return extended_instructions;
   case 0xCB:
      return bit_instructions;
   case 0xDD:
      return index_instructions;
   case 0xFD:
      return index_instructions;
   case 0xDDCB:
      return index_bit_instructions;
   case 0xFDCB:
      return index_bit_instructions;
   }
   printf("illegal prefix %x\n", prefix);
   return NULL;
}

char *reg_by_prefix(int prefix) {
   switch (prefix) {
   case 0:
      return "";
   case 0xED:
      return "";
   case 0xCB:
      return "";
   case 0xDD:
      return "IX";
   case 0xFD:
      return "IY";
   case 0xDDCB:
      return "IX";
   case 0xFDCB:
      return "IY";
   }
   printf("illegal prefix %x\n", prefix);
   return "";
}
