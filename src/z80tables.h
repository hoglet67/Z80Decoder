typedef struct {
   int d;
   int i;
   int ro;
   int wo;
   int rep;
   const char *fmt;
} InstrType;

extern InstrType main_instructions[256];
extern InstrType extended_instructions[256];
extern InstrType bit_instructions[256];
extern InstrType index_instructions[256];
extern InstrType index_bit_instructions[256];
