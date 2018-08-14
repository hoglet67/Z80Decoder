#ifndef _INCLUDE_EM_Z80_H
#define _INCLUDE_EM_Z80_H

#define False 0
#define True  1

#define FAIL_NONE                 0
#define FAIL_ERROR                1
#define FAIL_NOT_IMPLEMENTED      2
#define FAIL_IMPLEMENTATION_ERROR 3

#define CPU_DEFAULT               0
#define CPU_NMOS_ZILOG            1
#define CPU_NMOS_NEC              2
#define CPU_CMOS                  3

typedef enum {
   TYPE_0,   // no params
   TYPE_1,   // {arg_reg}
   TYPE_2,   // {arg_reg} {arg_reg}
   TYPE_3,   // {arg_imm} {arg_reg}
   TYPE_4,   // {arg_reg} {arg_imm}
   TYPE_5,   // {arg_reg} {arg_dis}
   TYPE_6,   // {arg_reg} {arg_dis} {arg_imm}
   TYPE_7,   // {jump}
   TYPE_8    // {arg_dis}
} FormatType;

typedef struct Instr {
   int want_dis;
   int want_imm;
   int want_read;
   int want_write;
   int conditional;
   FormatType format;
   const char *mnemonic;
   void (*emulate)(struct Instr *);
   int count;
} InstrType;

extern InstrType z80_interrupt_int;
extern InstrType z80_interrupt_nmi;

InstrType *table_by_prefix(int prefix);
char *reg_by_prefix(int prefix);
char *z80_get_state(int verbosity);
void z80_init(int cpu_type);
void z80_reset();
int z80_get_pc();
int z80_get_im();
void z80_increment_r();
int z80_halted();

#endif
