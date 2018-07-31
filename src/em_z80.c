//
// This file is part of the libsigrokdecode project.
//
// Copyright (C) 2014 Daniel Elstner <daniel.kitta@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

//Instruction tuple: (d, i, ro, wo, rep, format string)
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

#include <stdio.h>
#include "em_z80.h"

#define UNDEFINED {-1, -1, -1, -1, False, TYPE_0, "???", NULL}

// Instructions without a prefix
InstrType main_instructions[256] = {
   {0, 0, 0, 0, False, TYPE_0, "NOP",               NULL            }, // 0x00
   {0, 2, 0, 0, False, TYPE_8, "LD BC,%04Xh",       NULL            }, // 0x01
   {0, 0, 0, 1, False, TYPE_0, "LD (BC),A",         NULL            }, // 0x02
   {0, 0, 0, 0, False, TYPE_0, "INC BC",            NULL            }, // 0x03
   {0, 0, 0, 0, False, TYPE_0, "INC B",             NULL            }, // 0x04
   {0, 0, 0, 0, False, TYPE_0, "DEC B",             NULL            }, // 0x05
   {0, 1, 0, 0, False, TYPE_8, "LD B,%02Xh",        NULL            }, // 0x06
   {0, 0, 0, 0, False, TYPE_0, "RLCA",              NULL            }, // 0x07
   {0, 0, 0, 0, False, TYPE_0, "EX AF,AF'",         NULL            }, // 0x08
   {0, 0, 0, 0, False, TYPE_0, "ADD HL,BC",         NULL            }, // 0x09
   {0, 0, 1, 0, False, TYPE_0, "LD A,(BC)",         NULL            }, // 0x0A
   {0, 0, 0, 0, False, TYPE_0, "DEC BC",            NULL            }, // 0x0B
   {0, 0, 0, 0, False, TYPE_0, "INC C",             NULL            }, // 0x0C
   {0, 0, 0, 0, False, TYPE_0, "DEC C",             NULL            }, // 0x0D
   {0, 1, 0, 0, False, TYPE_8, "LD C,%02Xh",        NULL            }, // 0x0E
   {0, 0, 0, 0, False, TYPE_0, "RRCA",              NULL            }, // 0x0F

   {1, 0, 0, 0, False, TYPE_7, "DJNZ $%+d",         NULL            }, // 0x10
   {0, 2, 0, 0, False, TYPE_8, "LD DE,%04Xh",       NULL            }, // 0x11
   {0, 0, 0, 1, False, TYPE_0, "LD (DE),A",         NULL            }, // 0x12
   {0, 0, 0, 0, False, TYPE_0, "INC DE",            NULL            }, // 0x13
   {0, 0, 0, 0, False, TYPE_0, "INC D",             NULL            }, // 0x14
   {0, 0, 0, 0, False, TYPE_0, "DEC D",             NULL            }, // 0x15
   {0, 1, 0, 0, False, TYPE_8, "LD D,%02Xh",        NULL            }, // 0x16
   {0, 0, 0, 0, False, TYPE_0, "RLA",               NULL            }, // 0x17
   {1, 0, 0, 0, False, TYPE_7, "JR $%+d",           NULL            }, // 0x18
   {0, 0, 0, 0, False, TYPE_0, "ADD HL,DE",         NULL            }, // 0x19
   {0, 0, 1, 0, False, TYPE_0, "LD A,(DE)",         NULL            }, // 0x1A
   {0, 0, 0, 0, False, TYPE_0, "DEC DE",            NULL            }, // 0x1B
   {0, 0, 0, 0, False, TYPE_0, "INC E",             NULL            }, // 0x1C
   {0, 0, 0, 0, False, TYPE_0, "DEC E",             NULL            }, // 0x1D
   {0, 1, 0, 0, False, TYPE_8, "LD E,%02Xh",        NULL            }, // 0x1E
   {0, 0, 0, 0, False, TYPE_0, "RRA",               NULL            }, // 0x1F

   {1, 0, 0, 0, False, TYPE_7, "JR NZ,$%+d",        NULL            }, // 0x20
   {0, 2, 0, 0, False, TYPE_8, "LD HL,%04Xh",       NULL            }, // 0x21
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),HL",     NULL            }, // 0x22
   {0, 0, 0, 0, False, TYPE_0, "INC HL",            NULL            }, // 0x23
   {0, 0, 0, 0, False, TYPE_0, "INC H",             NULL            }, // 0x24
   {0, 0, 0, 0, False, TYPE_0, "DEC H",             NULL            }, // 0x25
   {0, 1, 0, 0, False, TYPE_8, "LD H,%02Xh",        NULL            }, // 0x26
   {0, 0, 0, 0, False, TYPE_0, "DAA",               NULL            }, // 0x27
   {1, 0, 0, 0, False, TYPE_7, "JR Z,$%+d",         NULL            }, // 0x28
   {0, 0, 0, 0, False, TYPE_0, "ADD HL,HL",         NULL            }, // 0x29
   {0, 2, 2, 0, False, TYPE_8, "LD HL,(%04Xh)",     NULL            }, // 0x2A
   {0, 0, 0, 0, False, TYPE_0, "DEC HL",            NULL            }, // 0x2B
   {0, 0, 0, 0, False, TYPE_0, "INC L",             NULL            }, // 0x2C
   {0, 0, 0, 0, False, TYPE_0, "DEC L",             NULL            }, // 0x2D
   {0, 1, 0, 0, False, TYPE_8, "LD L,%02Xh",        NULL            }, // 0x2E
   {0, 0, 0, 0, False, TYPE_0, "CPL",               NULL            }, // 0x2F

   {1, 0, 0, 0, False, TYPE_7, "JR NC,$%+d",        NULL            }, // 0x30
   {0, 2, 0, 0, False, TYPE_8, "LD SP,%04Xh",       NULL            }, // 0x31
   {0, 2, 0, 1, False, TYPE_8, "LD (%04Xh),A",      NULL            }, // 0x32
   {0, 0, 0, 0, False, TYPE_0, "INC SP",            NULL            }, // 0x33
   {0, 0, 1, 1, False, TYPE_0, "INC (HL)",          NULL            }, // 0x34
   {0, 0, 1, 1, False, TYPE_0, "DEC (HL)",          NULL            }, // 0x35
   {0, 1, 0, 1, False, TYPE_8, "LD (HL),%02Xh",     NULL            }, // 0x36
   {0, 0, 0, 0, False, TYPE_0, "SCF",               NULL            }, // 0x37
   {1, 0, 0, 0, False, TYPE_7, "JR C,$%+d",         NULL            }, // 0x38
   {0, 0, 0, 0, False, TYPE_0, "ADD HL,SP",         NULL            }, // 0x39
   {0, 2, 1, 0, False, TYPE_8, "LD A,(%04Xh)",      NULL            }, // 0x3A
   {0, 0, 0, 0, False, TYPE_0, "DEC SP",            NULL            }, // 0x3B
   {0, 0, 0, 0, False, TYPE_0, "INC A",             NULL            }, // 0x3C
   {0, 0, 0, 0, False, TYPE_0, "DEC A",             NULL            }, // 0x3D
   {0, 1, 0, 0, False, TYPE_8, "LD A,%02Xh",        NULL            }, // 0x3E
   {0, 0, 0, 0, False, TYPE_0, "CCF",               NULL            }, // 0x3F

   {0, 0, 0, 0, False, TYPE_0, "LD B,B",            NULL            }, // 0x40
   {0, 0, 0, 0, False, TYPE_0, "LD B,C",            NULL            }, // 0x41
   {0, 0, 0, 0, False, TYPE_0, "LD B,D",            NULL            }, // 0x42
   {0, 0, 0, 0, False, TYPE_0, "LD B,E",            NULL            }, // 0x43
   {0, 0, 0, 0, False, TYPE_0, "LD B,H",            NULL            }, // 0x44
   {0, 0, 0, 0, False, TYPE_0, "LD B,L",            NULL            }, // 0x45
   {0, 0, 1, 0, False, TYPE_0, "LD B,(HL)",         NULL            }, // 0x46
   {0, 0, 0, 0, False, TYPE_0, "LD B,A",            NULL            }, // 0x47
   {0, 0, 0, 0, False, TYPE_0, "LD C,B",            NULL            }, // 0x48
   {0, 0, 0, 0, False, TYPE_0, "LD C,C",            NULL            }, // 0x49
   {0, 0, 0, 0, False, TYPE_0, "LD C,D",            NULL            }, // 0x4A
   {0, 0, 0, 0, False, TYPE_0, "LD C,E",            NULL            }, // 0x4B
   {0, 0, 0, 0, False, TYPE_0, "LD C,H",            NULL            }, // 0x4C
   {0, 0, 0, 0, False, TYPE_0, "LD C,L",            NULL            }, // 0x4D
   {0, 0, 1, 0, False, TYPE_0, "LD C,(HL)",         NULL            }, // 0x4E
   {0, 0, 0, 0, False, TYPE_0, "LD C,A",            NULL            }, // 0x4F

   {0, 0, 0, 0, False, TYPE_0, "LD D,B",            NULL            }, // 0x50
   {0, 0, 0, 0, False, TYPE_0, "LD D,C",            NULL            }, // 0x51
   {0, 0, 0, 0, False, TYPE_0, "LD D,D",            NULL            }, // 0x52
   {0, 0, 0, 0, False, TYPE_0, "LD D,E",            NULL            }, // 0x53
   {0, 0, 0, 0, False, TYPE_0, "LD D,H",            NULL            }, // 0x54
   {0, 0, 0, 0, False, TYPE_0, "LD D,L",            NULL            }, // 0x55
   {0, 0, 1, 0, False, TYPE_0, "LD D,(HL)",         NULL            }, // 0x56
   {0, 0, 0, 0, False, TYPE_0, "LD D,A",            NULL            }, // 0x57
   {0, 0, 0, 0, False, TYPE_0, "LD E,B",            NULL            }, // 0x58
   {0, 0, 0, 0, False, TYPE_0, "LD E,C",            NULL            }, // 0x59
   {0, 0, 0, 0, False, TYPE_0, "LD E,D",            NULL            }, // 0x5A
   {0, 0, 0, 0, False, TYPE_0, "LD E,E",            NULL            }, // 0x5B
   {0, 0, 0, 0, False, TYPE_0, "LD E,H",            NULL            }, // 0x5C
   {0, 0, 0, 0, False, TYPE_0, "LD E,L",            NULL            }, // 0x5D
   {0, 0, 1, 0, False, TYPE_0, "LD E,(HL)",         NULL            }, // 0x5E
   {0, 0, 0, 0, False, TYPE_0, "LD E,A",            NULL            }, // 0x5F

   {0, 0, 0, 0, False, TYPE_0, "LD H,B",            NULL            }, // 0x60
   {0, 0, 0, 0, False, TYPE_0, "LD H,C",            NULL            }, // 0x61
   {0, 0, 0, 0, False, TYPE_0, "LD H,D",            NULL            }, // 0x62
   {0, 0, 0, 0, False, TYPE_0, "LD H,E",            NULL            }, // 0x63
   {0, 0, 0, 0, False, TYPE_0, "LD H,H",            NULL            }, // 0x64
   {0, 0, 0, 0, False, TYPE_0, "LD H,L",            NULL            }, // 0x65
   {0, 0, 1, 0, False, TYPE_0, "LD H,(HL)",         NULL            }, // 0x66
   {0, 0, 0, 0, False, TYPE_0, "LD H,A",            NULL            }, // 0x67
   {0, 0, 0, 0, False, TYPE_0, "LD L,B",            NULL            }, // 0x68
   {0, 0, 0, 0, False, TYPE_0, "LD L,C",            NULL            }, // 0x69
   {0, 0, 0, 0, False, TYPE_0, "LD L,D",            NULL            }, // 0x6A
   {0, 0, 0, 0, False, TYPE_0, "LD L,E",            NULL            }, // 0x6B
   {0, 0, 0, 0, False, TYPE_0, "LD L,H",            NULL            }, // 0x6C
   {0, 0, 0, 0, False, TYPE_0, "LD L,L",            NULL            }, // 0x6D
   {0, 0, 1, 0, False, TYPE_0, "LD L,(HL)",         NULL            }, // 0x6E
   {0, 0, 0, 0, False, TYPE_0, "LD L,A",            NULL            }, // 0x6F

   {0, 0, 0, 1, False, TYPE_0, "LD (HL),B",         NULL            }, // 0x70
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),C",         NULL            }, // 0x71
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),D",         NULL            }, // 0x72
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),E",         NULL            }, // 0x73
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),H",         NULL            }, // 0x74
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),L",         NULL            }, // 0x75
   {0, 0, 0, 0, False, TYPE_0, "HALT",              NULL            }, // 0x76
   {0, 0, 0, 1, False, TYPE_0, "LD (HL),A",         NULL            }, // 0x77
   {0, 0, 0, 0, False, TYPE_0, "LD A,B",            NULL            }, // 0x78
   {0, 0, 0, 0, False, TYPE_0, "LD A,C",            NULL            }, // 0x79
   {0, 0, 0, 0, False, TYPE_0, "LD A,D",            NULL            }, // 0x7A
   {0, 0, 0, 0, False, TYPE_0, "LD A,E",            NULL            }, // 0x7B
   {0, 0, 0, 0, False, TYPE_0, "LD A,H",            NULL            }, // 0x7C
   {0, 0, 0, 0, False, TYPE_0, "LD A,L",            NULL            }, // 0x7D
   {0, 0, 1, 0, False, TYPE_0, "LD A,(HL)",         NULL            }, // 0x7E
   {0, 0, 0, 0, False, TYPE_0, "LD A,A",            NULL            }, // 0x7F

   {0, 0, 0, 0, False, TYPE_0, "ADD A,B",           NULL            }, // 0x80
   {0, 0, 0, 0, False, TYPE_0, "ADD A,C",           NULL            }, // 0x81
   {0, 0, 0, 0, False, TYPE_0, "ADD A,D",           NULL            }, // 0x82
   {0, 0, 0, 0, False, TYPE_0, "ADD A,E",           NULL            }, // 0x83
   {0, 0, 0, 0, False, TYPE_0, "ADD A,H",           NULL            }, // 0x84
   {0, 0, 0, 0, False, TYPE_0, "ADD A,L",           NULL            }, // 0x85
   {0, 0, 1, 0, False, TYPE_0, "ADD A,(HL)",        NULL            }, // 0x86
   {0, 0, 0, 0, False, TYPE_0, "ADD A,A",           NULL            }, // 0x87
   {0, 0, 0, 0, False, TYPE_0, "ADC A,B",           NULL            }, // 0x88
   {0, 0, 0, 0, False, TYPE_0, "ADC A,C",           NULL            }, // 0x89
   {0, 0, 0, 0, False, TYPE_0, "ADC A,D",           NULL            }, // 0x8A
   {0, 0, 0, 0, False, TYPE_0, "ADC A,E",           NULL            }, // 0x8B
   {0, 0, 0, 0, False, TYPE_0, "ADC A,H",           NULL            }, // 0x8C
   {0, 0, 0, 0, False, TYPE_0, "ADC A,L",           NULL            }, // 0x8D
   {0, 0, 1, 0, False, TYPE_0, "ADC A,(HL)",        NULL            }, // 0x8E
   {0, 0, 0, 0, False, TYPE_0, "ADC A,A",           NULL            }, // 0x8F

   {0, 0, 0, 0, False, TYPE_0, "SUB B",             NULL            }, // 0x90
   {0, 0, 0, 0, False, TYPE_0, "SUB C",             NULL            }, // 0x91
   {0, 0, 0, 0, False, TYPE_0, "SUB D",             NULL            }, // 0x92
   {0, 0, 0, 0, False, TYPE_0, "SUB E",             NULL            }, // 0x93
   {0, 0, 0, 0, False, TYPE_0, "SUB H",             NULL            }, // 0x94
   {0, 0, 0, 0, False, TYPE_0, "SUB L",             NULL            }, // 0x95
   {0, 0, 1, 0, False, TYPE_0, "SUB (HL)",          NULL            }, // 0x96
   {0, 0, 0, 0, False, TYPE_0, "SUB A",             NULL            }, // 0x97
   {0, 0, 0, 0, False, TYPE_0, "SBC A,B",           NULL            }, // 0x98
   {0, 0, 0, 0, False, TYPE_0, "SBC A,C",           NULL            }, // 0x99
   {0, 0, 0, 0, False, TYPE_0, "SBC A,D",           NULL            }, // 0x9A
   {0, 0, 0, 0, False, TYPE_0, "SBC A,E",           NULL            }, // 0x9B
   {0, 0, 0, 0, False, TYPE_0, "SBC A,H",           NULL            }, // 0x9C
   {0, 0, 0, 0, False, TYPE_0, "SBC A,L",           NULL            }, // 0x9D
   {0, 0, 1, 0, False, TYPE_0, "SBC A,(HL)",        NULL            }, // 0x9E
   {0, 0, 0, 0, False, TYPE_0, "SBC A,A",           NULL            }, // 0x9F

   {0, 0, 0, 0, False, TYPE_0, "AND B",             NULL            }, // 0xA0
   {0, 0, 0, 0, False, TYPE_0, "AND C",             NULL            }, // 0xA1
   {0, 0, 0, 0, False, TYPE_0, "AND D",             NULL            }, // 0xA2
   {0, 0, 0, 0, False, TYPE_0, "AND E",             NULL            }, // 0xA3
   {0, 0, 0, 0, False, TYPE_0, "AND H",             NULL            }, // 0xA4
   {0, 0, 0, 0, False, TYPE_0, "AND L",             NULL            }, // 0xA5
   {0, 0, 1, 0, False, TYPE_0, "AND (HL)",          NULL            }, // 0xA6
   {0, 0, 0, 0, False, TYPE_0, "AND A",             NULL            }, // 0xA7
   {0, 0, 0, 0, False, TYPE_0, "XOR B",             NULL            }, // 0xA8
   {0, 0, 0, 0, False, TYPE_0, "XOR C",             NULL            }, // 0xA9
   {0, 0, 0, 0, False, TYPE_0, "XOR D",             NULL            }, // 0xAA
   {0, 0, 0, 0, False, TYPE_0, "XOR E",             NULL            }, // 0xAB
   {0, 0, 0, 0, False, TYPE_0, "XOR H",             NULL            }, // 0xAC
   {0, 0, 0, 0, False, TYPE_0, "XOR L",             NULL            }, // 0xAD
   {0, 0, 1, 0, False, TYPE_0, "XOR (HL)",          NULL            }, // 0xAE
   {0, 0, 0, 0, False, TYPE_0, "XOR A",             NULL            }, // 0xAF

   {0, 0, 0, 0, False, TYPE_0, "OR B",              NULL            }, // 0xB0
   {0, 0, 0, 0, False, TYPE_0, "OR C",              NULL            }, // 0xB1
   {0, 0, 0, 0, False, TYPE_0, "OR D",              NULL            }, // 0xB2
   {0, 0, 0, 0, False, TYPE_0, "OR E",              NULL            }, // 0xB3
   {0, 0, 0, 0, False, TYPE_0, "OR H",              NULL            }, // 0xB4
   {0, 0, 0, 0, False, TYPE_0, "OR L",              NULL            }, // 0xB5
   {0, 0, 1, 0, False, TYPE_0, "OR (HL)",           NULL            }, // 0xB6
   {0, 0, 0, 0, False, TYPE_0, "OR A",              NULL            }, // 0xB7
   {0, 0, 0, 0, False, TYPE_0, "CP B",              NULL            }, // 0xB8
   {0, 0, 0, 0, False, TYPE_0, "CP C",              NULL            }, // 0xB9
   {0, 0, 0, 0, False, TYPE_0, "CP D",              NULL            }, // 0xBA
   {0, 0, 0, 0, False, TYPE_0, "CP E",              NULL            }, // 0xBB
   {0, 0, 0, 0, False, TYPE_0, "CP H",              NULL            }, // 0xBC
   {0, 0, 0, 0, False, TYPE_0, "CP L",              NULL            }, // 0xBD
   {0, 0, 1, 0, False, TYPE_0, "CP (HL)",           NULL            }, // 0xBE
   {0, 0, 0, 0, False, TYPE_0, "CP A",              NULL            }, // 0xBF

   {0, 0, 2, 0, True,  TYPE_0, "RET NZ",            NULL            }, // 0xC0
   {0, 0, 2, 0, False, TYPE_0, "POP BC",            NULL            }, // 0xC1
   {0, 2, 0, 0, False, TYPE_8, "JP NZ,%04Xh",       NULL            }, // 0xC2
   {0, 2, 0, 0, False, TYPE_8, "JP %04Xh",          NULL            }, // 0xC3
   {0, 2, 0,-2, True,  TYPE_8, "CALL NZ,%04Xh",     NULL            }, // 0xC4
   {0, 0, 0,-2, False, TYPE_0, "PUSH BC",           NULL            }, // 0xC5
   {0, 1, 0, 0, False, TYPE_8, "ADD A,%02Xh",       NULL            }, // 0xC6
   {0, 0, 0,-2, False, TYPE_0, "RST 00h",           NULL            }, // 0xC7
   {0, 0, 2, 0, True,  TYPE_0, "RET Z",             NULL            }, // 0xC8
   {0, 0, 2, 0, False, TYPE_0, "RET",               NULL            }, // 0xC9
   {0, 2, 0, 0, False, TYPE_8, "JP Z,%04Xh",        NULL            }, // 0xCA
   UNDEFINED,                                                          // 0xCB
   {0, 2, 0,-2, True,  TYPE_8, "CALL Z,%04Xh",      NULL            }, // 0xCC
   {0, 2, 0,-2, False, TYPE_8, "CALL %04Xh",        NULL            }, // 0xCD
   {0, 1, 0, 0, False, TYPE_8, "ADC A,%02Xh",       NULL            }, // 0xCE
   {0, 0, 0,-2, False, TYPE_0, "RST 08h",           NULL            }, // 0xCF

   {0, 0, 2, 0, True,  TYPE_0, "RET NC",            NULL            }, // 0xD0
   {0, 0, 2, 0, False, TYPE_0, "POP DE",            NULL            }, // 0xD1
   {0, 2, 0, 0, False, TYPE_8, "JP NC,%04Xh",       NULL            }, // 0xD2
   {0, 1, 0, 1, False, TYPE_8, "OUT (%02Xh),A",     NULL            }, // 0xD3
   {0, 2, 0,-2, True,  TYPE_8, "CALL NC,%04Xh",     NULL            }, // 0xD4
   {0, 0, 0,-2, False, TYPE_0, "PUSH DE",           NULL            }, // 0xD5
   {0, 1, 0, 0, False, TYPE_8, "SUB %02Xh",         NULL            }, // 0xD6
   {0, 0, 0,-2, False, TYPE_0, "RST 10h",           NULL            }, // 0xD7
   {0, 0, 2, 0, True,  TYPE_0, "RET C",             NULL            }, // 0xD8
   {0, 0, 0, 0, False, TYPE_0, "EXX",               NULL            }, // 0xD9
   {0, 2, 0, 0, False, TYPE_8, "JP C,%04Xh",        NULL            }, // 0xDA
   {0, 1, 1, 0, False, TYPE_8, "IN A,(%02Xh)",      NULL            }, // 0xDB
   {0, 2, 0,-2, True,  TYPE_8, "CALL C,%04Xh",      NULL            }, // 0xDC
   UNDEFINED,                                                          // 0xDD
   {0, 1, 0, 0, False, TYPE_8, "SBC A,%02Xh",       NULL            }, // 0xDE
   {0, 0, 0,-2, False, TYPE_0, "RST 18h",           NULL            }, // 0xDF

   {0, 0, 2, 0, True,  TYPE_0, "RET PO",            NULL            }, // 0xE0
   {0, 0, 2, 0, False, TYPE_0, "POP HL",            NULL            }, // 0xE1
   {0, 2, 0, 0, False, TYPE_8, "JP PO,%04Xh",       NULL            }, // 0xE2
   {0, 0, 2, 2, False, TYPE_0, "EX (SP),HL",        NULL            }, // 0xE3
   {0, 2, 0,-2, True,  TYPE_8, "CALL PO,%04Xh",     NULL            }, // 0xE4
   {0, 0, 0,-2, False, TYPE_0, "PUSH HL",           NULL            }, // 0xE5
   {0, 1, 0, 0, False, TYPE_8, "AND %02Xh",         NULL            }, // 0xE6
   {0, 0, 0,-2, False, TYPE_0, "RST 20h",           NULL            }, // 0xE7
   {0, 0, 2, 0, True,  TYPE_0, "RET PE",            NULL            }, // 0xE8
   {0, 0, 0, 0, False, TYPE_0, "JP (HL)",           NULL            }, // 0xE9
   {0, 2, 0, 0, False, TYPE_8, "JP PE,%04Xh",       NULL            }, // 0xEA
   {0, 0, 0, 0, False, TYPE_0, "EX DE,HL",          NULL            }, // 0xEB
   {0, 2, 0,-2, True,  TYPE_8, "CALL PE,%04Xh",     NULL            }, // 0xEC
   UNDEFINED,                                                          // 0xED
   {0, 1, 0, 0, False, TYPE_8, "XOR %02Xh",         NULL            }, // 0xEE
   {0, 0, 0,-2, False, TYPE_0, "RST 28h",           NULL            }, // 0xEF

   {0, 0, 2, 0, True,  TYPE_0, "RET P",             NULL            }, // 0xF0
   {0, 0, 2, 0, False, TYPE_0, "POP AF",            NULL            }, // 0xF1
   {0, 2, 0, 0, False, TYPE_8, "JP P,%04Xh",        NULL            }, // 0xF2
   {0, 0, 0, 0, False, TYPE_0, "DI",                NULL            }, // 0xF3
   {0, 2, 0,-2, True,  TYPE_8, "CALL P,%04Xh",      NULL            }, // 0xF4
   {0, 0, 0,-2, False, TYPE_0, "PUSH AF",           NULL            }, // 0xF5
   {0, 1, 0, 0, False, TYPE_8, "OR %02Xh",          NULL            }, // 0xF6
   {0, 0, 0,-2, False, TYPE_0, "RST 30h",           NULL            }, // 0xF7
   {0, 0, 2, 0, True,  TYPE_0, "RET M",             NULL            }, // 0xF8
   {0, 0, 0, 0, False, TYPE_0, "LD SP,HL",          NULL            }, // 0xF9
   {0, 2, 0, 0, False, TYPE_8, "JP M,%04Xh",        NULL            }, // 0xFA
   {0, 0, 0, 0, False, TYPE_0, "EI",                NULL            }, // 0xFB
   {0, 2, 0,-2, True,  TYPE_8, "CALL M,%04Xh",      NULL            }, // 0xFC
   UNDEFINED,                                                          // 0xFD
   {0, 1, 0, 0, False, TYPE_8, "CP %02Xh",          NULL            }, // 0xFE
   {0, 0, 0,-2, False, TYPE_0, "RST 38h",           NULL            }  // 0xFF
};

// Instructions with ED prefix
InstrType extended_instructions[256] = {
   UNDEFINED,                                                          // 0x00
   UNDEFINED,                                                          // 0x01
   UNDEFINED,                                                          // 0x02
   UNDEFINED,                                                          // 0x03
   UNDEFINED,                                                          // 0x04
   UNDEFINED,                                                          // 0x05
   UNDEFINED,                                                          // 0x06
   UNDEFINED,                                                          // 0x07
   UNDEFINED,                                                          // 0x08
   UNDEFINED,                                                          // 0x09
   UNDEFINED,                                                          // 0x0A
   UNDEFINED,                                                          // 0x0B
   UNDEFINED,                                                          // 0x0C
   UNDEFINED,                                                          // 0x0D
   UNDEFINED,                                                          // 0x0E
   UNDEFINED,                                                          // 0x0F

   UNDEFINED,                                                          // 0x10
   UNDEFINED,                                                          // 0x11
   UNDEFINED,                                                          // 0x12
   UNDEFINED,                                                          // 0x13
   UNDEFINED,                                                          // 0x14
   UNDEFINED,                                                          // 0x15
   UNDEFINED,                                                          // 0x16
   UNDEFINED,                                                          // 0x17
   UNDEFINED,                                                          // 0x18
   UNDEFINED,                                                          // 0x19
   UNDEFINED,                                                          // 0x1A
   UNDEFINED,                                                          // 0x1B
   UNDEFINED,                                                          // 0x1C
   UNDEFINED,                                                          // 0x1D
   UNDEFINED,                                                          // 0x1E
   UNDEFINED,                                                          // 0x1F

   UNDEFINED,                                                          // 0x20
   UNDEFINED,                                                          // 0x21
   UNDEFINED,                                                          // 0x22
   UNDEFINED,                                                          // 0x23
   UNDEFINED,                                                          // 0x24
   UNDEFINED,                                                          // 0x25
   UNDEFINED,                                                          // 0x26
   UNDEFINED,                                                          // 0x27
   UNDEFINED,                                                          // 0x28
   UNDEFINED,                                                          // 0x29
   UNDEFINED,                                                          // 0x2A
   UNDEFINED,                                                          // 0x2B
   UNDEFINED,                                                          // 0x2C
   UNDEFINED,                                                          // 0x2D
   UNDEFINED,                                                          // 0x2E
   UNDEFINED,                                                          // 0x2F

   UNDEFINED,                                                          // 0x30
   UNDEFINED,                                                          // 0x31
   UNDEFINED,                                                          // 0x32
   UNDEFINED,                                                          // 0x33
   UNDEFINED,                                                          // 0x34
   UNDEFINED,                                                          // 0x35
   UNDEFINED,                                                          // 0x36
   UNDEFINED,                                                          // 0x37
   UNDEFINED,                                                          // 0x38
   UNDEFINED,                                                          // 0x39
   UNDEFINED,                                                          // 0x3A
   UNDEFINED,                                                          // 0x3B
   UNDEFINED,                                                          // 0x3C
   UNDEFINED,                                                          // 0x3D
   UNDEFINED,                                                          // 0x3E
   UNDEFINED,                                                          // 0x3F

   {0, 0, 1, 0, False, TYPE_0, "IN B,(C)",          NULL            }, // 0x40
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),B",         NULL            }, // 0x41
   {0, 0, 0, 0, False, TYPE_0, "SBC HL,BC",         NULL            }, // 0x42
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),BC",     NULL            }, // 0x43
   {0, 0, 0, 0, False, TYPE_0, "NEG",               NULL            }, // 0x44
   {0, 0, 2, 0, False, TYPE_0, "RETN",              NULL            }, // 0x45
   {0, 0, 0, 0, False, TYPE_0, "IM 0",              NULL            }, // 0x46
   {0, 0, 0, 0, False, TYPE_0, "LD I,A",            NULL            }, // 0x47
   {0, 0, 1, 0, False, TYPE_0, "IN C,(C)",          NULL            }, // 0x48
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),C",         NULL            }, // 0x49
   {0, 0, 0, 0, False, TYPE_0, "ADC HL,BC",         NULL            }, // 0x4A
   {0, 2, 2, 0, False, TYPE_8, "LD BC,(%04Xh)",     NULL            }, // 0x4B
   {0, 0, 0, 0, False, TYPE_0, "NEG",               NULL            }, // 0x4C
   {0, 0, 2, 0, False, TYPE_0, "RETI",              NULL            }, // 0x4D
   {0, 0, 0, 0, False, TYPE_0, "IM 0/1",            NULL            }, // 0x4E
   {0, 0, 0, 0, False, TYPE_0, "LD R,A",            NULL            }, // 0x4F

   {0, 0, 1, 0, False, TYPE_0, "IN D,(C)",          NULL            }, // 0x50
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),D",         NULL            }, // 0x51
   {0, 0, 0, 0, False, TYPE_0, "SBC HL,DE",         NULL            }, // 0x52
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),DE",     NULL            }, // 0x53
   {0, 0, 0, 0, False, TYPE_0, "NEG",               NULL            }, // 0x54
   {0, 0, 2, 0, False, TYPE_0, "RETN",              NULL            }, // 0x55
   {0, 0, 0, 0, False, TYPE_0, "IM 1",              NULL            }, // 0x56
   {0, 0, 0, 0, False, TYPE_0, "LD A,I",            NULL            }, // 0x57
   {0, 0, 1, 0, False, TYPE_0, "IN E,(C)",          NULL            }, // 0x58
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),E",         NULL            }, // 0x59
   {0, 0, 0, 0, False, TYPE_0, "ADC HL,DE",         NULL            }, // 0x5A
   {0, 2, 2, 0, False, TYPE_8, "LD DE,(%04Xh)",     NULL            }, // 0x5B
   {0, 0, 0, 0, False, TYPE_0, "NEG",               NULL            }, // 0x5C
   {0, 0, 2, 0, False, TYPE_0, "RETN",              NULL            }, // 0x5D
   {0, 0, 0, 0, False, TYPE_0, "IM 2",              NULL            }, // 0x5E
   {0, 0, 0, 0, False, TYPE_0, "LD A,R",            NULL            }, // 0x5F

   {0, 0, 1, 0, False, TYPE_0, "IN H,(C)",          NULL            }, // 0x60
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),H",         NULL            }, // 0x61
   {0, 0, 0, 0, False, TYPE_0, "SBC HL,HL",         NULL            }, // 0x62
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),HL",     NULL            }, // 0x63
   {0, 0, 0, 0, False, TYPE_0, "NEG",               NULL            }, // 0x64
   {0, 0, 2, 0, False, TYPE_0, "RETN",              NULL            }, // 0x65
   {0, 0, 0, 0, False, TYPE_0, "IM 0",              NULL            }, // 0x66
   {0, 0, 1, 1, False, TYPE_0, "RRD",               NULL            }, // 0x67
   {0, 0, 1, 0, False, TYPE_0, "IN L,(C)",          NULL            }, // 0x68
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),L",         NULL            }, // 0x69
   {0, 0, 0, 0, False, TYPE_0, "ADC HL,HL",         NULL            }, // 0x6A
   {0, 2, 2, 0, False, TYPE_8, "LD HL,(%04Xh)",     NULL            }, // 0x6B
   {0, 0, 0, 0, False, TYPE_0, "NEG",               NULL            }, // 0x6C
   {0, 0, 2, 0, False, TYPE_0, "RETN",              NULL            }, // 0x6D
   {0, 0, 0, 0, False, TYPE_0, "IM 0/1",            NULL            }, // 0x6E
   {0, 0, 1, 1, False, TYPE_0, "RLD",               NULL            }, // 0x6F

   {0, 0, 1, 0, False, TYPE_0, "IN (C)",            NULL            }, // 0x70
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),0",         NULL            }, // 0x71
   {0, 0, 0, 0, False, TYPE_0, "SBC HL,SP",         NULL            }, // 0x72
   {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),SP",     NULL            }, // 0x73
   {0, 0, 0, 0, False, TYPE_0, "NEG",               NULL            }, // 0x74
   {0, 0, 2, 0, False, TYPE_0, "RETN",              NULL            }, // 0x75
   {0, 0, 0, 0, False, TYPE_0, "IM 1",              NULL            }, // 0x76
   UNDEFINED,                                                          // 0x77
   {0, 0, 1, 0, False, TYPE_0, "IN A,(C)",          NULL            }, // 0x78
   {0, 0, 0, 1, False, TYPE_0, "OUT (C),A",         NULL            }, // 0x79
   {0, 0, 0, 0, False, TYPE_0, "ADC HL,SP",         NULL            }, // 0x7A
   {0, 2, 2, 0, False, TYPE_8, "LD SP,(%04Xh)",     NULL            }, // 0x7B
   {0, 0, 0, 0, False, TYPE_0, "NEG",               NULL            }, // 0x7C
   {0, 0, 2, 0, False, TYPE_0, "RETN",              NULL            }, // 0x7D
   {0, 0, 0, 0, False, TYPE_0, "IM 2",              NULL            }, // 0x7E
   UNDEFINED,                                                          // 0x7F

   UNDEFINED,                                                          // 0x80
   UNDEFINED,                                                          // 0x81
   UNDEFINED,                                                          // 0x82
   UNDEFINED,                                                          // 0x83
   UNDEFINED,                                                          // 0x84
   UNDEFINED,                                                          // 0x85
   UNDEFINED,                                                          // 0x86
   UNDEFINED,                                                          // 0x87
   UNDEFINED,                                                          // 0x88
   UNDEFINED,                                                          // 0x89
   UNDEFINED,                                                          // 0x8A
   UNDEFINED,                                                          // 0x8B
   UNDEFINED,                                                          // 0x8C
   UNDEFINED,                                                          // 0x8D
   UNDEFINED,                                                          // 0x8E
   UNDEFINED,                                                          // 0x8F

   UNDEFINED,                                                          // 0x90
   UNDEFINED,                                                          // 0x91
   UNDEFINED,                                                          // 0x92
   UNDEFINED,                                                          // 0x93
   UNDEFINED,                                                          // 0x94
   UNDEFINED,                                                          // 0x95
   UNDEFINED,                                                          // 0x96
   UNDEFINED,                                                          // 0x97
   UNDEFINED,                                                          // 0x98
   UNDEFINED,                                                          // 0x99
   UNDEFINED,                                                          // 0x9A
   UNDEFINED,                                                          // 0x9B
   UNDEFINED,                                                          // 0x9C
   UNDEFINED,                                                          // 0x9D
   UNDEFINED,                                                          // 0x9E
   UNDEFINED,                                                          // 0x9F

   {0, 0, 1, 1, False, TYPE_0, "LDI",               NULL            }, // 0xA0
   {0, 0, 1, 0, False, TYPE_0, "CPI",               NULL            }, // 0xA1
   {0, 0, 1, 1, False, TYPE_0, "INI",               NULL            }, // 0xA2
   {0, 0, 1, 1, False, TYPE_0, "OUTI",              NULL            }, // 0xA3
   UNDEFINED,                                                          // 0xA4
   UNDEFINED,                                                          // 0xA5
   UNDEFINED,                                                          // 0xA6
   UNDEFINED,                                                          // 0xA7
   {0, 0, 1, 1, False, TYPE_0, "LDD",               NULL            }, // 0xA8
   {0, 0, 1, 0, False, TYPE_0, "CPD",               NULL            }, // 0xA9
   {0, 0, 1, 1, False, TYPE_0, "IND",               NULL            }, // 0xAA
   {0, 0, 1, 1, False, TYPE_0, "OUTD",              NULL            }, // 0xAB
   UNDEFINED,                                                          // 0xAC
   UNDEFINED,                                                          // 0xAD
   UNDEFINED,                                                          // 0xAE
   UNDEFINED,                                                          // 0xAF

   {0, 0, 1, 1, False, TYPE_0, "LDIR",              NULL            }, // 0xB0
   {0, 0, 1, 0, False, TYPE_0, "CPIR",              NULL            }, // 0xB1
   {0, 0, 1, 1, False, TYPE_0, "INIR",              NULL            }, // 0xB2
   {0, 0, 1, 1, False, TYPE_0, "OTIR",              NULL            }, // 0xB3
   UNDEFINED,                                                          // 0xB4
   UNDEFINED,                                                          // 0xB5
   UNDEFINED,                                                          // 0xB6
   UNDEFINED,                                                          // 0xB7
   {0, 0, 1, 1, False, TYPE_0, "LDDR",              NULL            }, // 0xB8
   {0, 0, 1, 0, False, TYPE_0, "CPDR",              NULL            }, // 0xB9
   {0, 0, 1, 1, False, TYPE_0, "INDR",              NULL            }, // 0xBA
   {0, 0, 1, 1, False, TYPE_0, "OTDR",              NULL            }, // 0xBB
   UNDEFINED,                                                          // 0xBC
   UNDEFINED,                                                          // 0xBD
   UNDEFINED,                                                          // 0xBE
   UNDEFINED,                                                          // 0xBF

   UNDEFINED,                                                          // 0xC0
   UNDEFINED,                                                          // 0xC1
   UNDEFINED,                                                          // 0xC2
   UNDEFINED,                                                          // 0xC3
   UNDEFINED,                                                          // 0xC4
   UNDEFINED,                                                          // 0xC5
   UNDEFINED,                                                          // 0xC6
   UNDEFINED,                                                          // 0xC7
   UNDEFINED,                                                          // 0xC8
   UNDEFINED,                                                          // 0xC9
   UNDEFINED,                                                          // 0xCA
   UNDEFINED,                                                          // 0xCB
   UNDEFINED,                                                          // 0xCC
   UNDEFINED,                                                          // 0xCD
   UNDEFINED,                                                          // 0xCE
   UNDEFINED,                                                          // 0xCF

   UNDEFINED,                                                          // 0xD0
   UNDEFINED,                                                          // 0xD1
   UNDEFINED,                                                          // 0xD2
   UNDEFINED,                                                          // 0xD3
   UNDEFINED,                                                          // 0xD4
   UNDEFINED,                                                          // 0xD5
   UNDEFINED,                                                          // 0xD6
   UNDEFINED,                                                          // 0xD7
   UNDEFINED,                                                          // 0xD8
   UNDEFINED,                                                          // 0xD9
   UNDEFINED,                                                          // 0xDA
   UNDEFINED,                                                          // 0xDB
   UNDEFINED,                                                          // 0xDC
   UNDEFINED,                                                          // 0xDD
   UNDEFINED,                                                          // 0xDE
   UNDEFINED,                                                          // 0xDF

   UNDEFINED,                                                          // 0xE0
   UNDEFINED,                                                          // 0xE1
   UNDEFINED,                                                          // 0xE2
   UNDEFINED,                                                          // 0xE3
   UNDEFINED,                                                          // 0xE4
   UNDEFINED,                                                          // 0xE5
   UNDEFINED,                                                          // 0xE6
   UNDEFINED,                                                          // 0xE7
   UNDEFINED,                                                          // 0xE8
   UNDEFINED,                                                          // 0xE9
   UNDEFINED,                                                          // 0xEA
   UNDEFINED,                                                          // 0xEB
   UNDEFINED,                                                          // 0xEC
   UNDEFINED,                                                          // 0xED
   UNDEFINED,                                                          // 0xEE
   UNDEFINED,                                                          // 0xEF

   UNDEFINED,                                                          // 0xF0
   UNDEFINED,                                                          // 0xF1
   UNDEFINED,                                                          // 0xF2
   UNDEFINED,                                                          // 0xF3
   UNDEFINED,                                                          // 0xF4
   UNDEFINED,                                                          // 0xF5
   UNDEFINED,                                                          // 0xF6
   UNDEFINED,                                                          // 0xF7
   UNDEFINED,                                                          // 0xF8
   UNDEFINED,                                                          // 0xF9
   UNDEFINED,                                                          // 0xFA
   UNDEFINED,                                                          // 0xFB
   UNDEFINED,                                                          // 0xFC
   UNDEFINED,                                                          // 0xFD
   UNDEFINED,                                                          // 0xFE
   UNDEFINED                                                           // 0xFF
};

// Instructions with CB prefix
InstrType bit_instructions[256] = {
   {0, 0, 0, 0, False, TYPE_0, "RLC B",             NULL            }, // 0x00
   {0, 0, 0, 0, False, TYPE_0, "RLC C",             NULL            }, // 0x01
   {0, 0, 0, 0, False, TYPE_0, "RLC D",             NULL            }, // 0x02
   {0, 0, 0, 0, False, TYPE_0, "RLC E",             NULL            }, // 0x03
   {0, 0, 0, 0, False, TYPE_0, "RLC H",             NULL            }, // 0x04
   {0, 0, 0, 0, False, TYPE_0, "RLC L",             NULL            }, // 0x05
   {0, 0, 1, 1, False, TYPE_0, "RLC (HL)",          NULL            }, // 0x06
   {0, 0, 0, 0, False, TYPE_0, "RLC A",             NULL            }, // 0x07
   {0, 0, 0, 0, False, TYPE_0, "RRC B",             NULL            }, // 0x08
   {0, 0, 0, 0, False, TYPE_0, "RRC C",             NULL            }, // 0x09
   {0, 0, 0, 0, False, TYPE_0, "RRC D",             NULL            }, // 0x0A
   {0, 0, 0, 0, False, TYPE_0, "RRC E",             NULL            }, // 0x0B
   {0, 0, 0, 0, False, TYPE_0, "RRC H",             NULL            }, // 0x0C
   {0, 0, 0, 0, False, TYPE_0, "RRC L",             NULL            }, // 0x0D
   {0, 0, 1, 1, False, TYPE_0, "RRC (HL)",          NULL            }, // 0x0E
   {0, 0, 0, 0, False, TYPE_0, "RRC A",             NULL            }, // 0x0F

   {0, 0, 0, 0, False, TYPE_0, "RL B",              NULL            }, // 0x10
   {0, 0, 0, 0, False, TYPE_0, "RL C",              NULL            }, // 0x11
   {0, 0, 0, 0, False, TYPE_0, "RL D",              NULL            }, // 0x12
   {0, 0, 0, 0, False, TYPE_0, "RL E",              NULL            }, // 0x13
   {0, 0, 0, 0, False, TYPE_0, "RL H",              NULL            }, // 0x14
   {0, 0, 0, 0, False, TYPE_0, "RL L",              NULL            }, // 0x15
   {0, 0, 1, 1, False, TYPE_0, "RL (HL)",           NULL            }, // 0x16
   {0, 0, 0, 0, False, TYPE_0, "RL A",              NULL            }, // 0x17
   {0, 0, 0, 0, False, TYPE_0, "RR B",              NULL            }, // 0x18
   {0, 0, 0, 0, False, TYPE_0, "RR C",              NULL            }, // 0x19
   {0, 0, 0, 0, False, TYPE_0, "RR D",              NULL            }, // 0x1A
   {0, 0, 0, 0, False, TYPE_0, "RR E",              NULL            }, // 0x1B
   {0, 0, 0, 0, False, TYPE_0, "RR H",              NULL            }, // 0x1C
   {0, 0, 0, 0, False, TYPE_0, "RR L",              NULL            }, // 0x1D
   {0, 0, 1, 1, False, TYPE_0, "RR (HL)",           NULL            }, // 0x1E
   {0, 0, 0, 0, False, TYPE_0, "RR A",              NULL            }, // 0x1F

   {0, 0, 0, 0, False, TYPE_0, "SLA B",             NULL            }, // 0x20
   {0, 0, 0, 0, False, TYPE_0, "SLA C",             NULL            }, // 0x21
   {0, 0, 0, 0, False, TYPE_0, "SLA D",             NULL            }, // 0x22
   {0, 0, 0, 0, False, TYPE_0, "SLA E",             NULL            }, // 0x23
   {0, 0, 0, 0, False, TYPE_0, "SLA H",             NULL            }, // 0x24
   {0, 0, 0, 0, False, TYPE_0, "SLA L",             NULL            }, // 0x25
   {0, 0, 1, 1, False, TYPE_0, "SLA (HL)",          NULL            }, // 0x26
   {0, 0, 0, 0, False, TYPE_0, "SLA A",             NULL            }, // 0x27
   {0, 0, 0, 0, False, TYPE_0, "SRA B",             NULL            }, // 0x28
   {0, 0, 0, 0, False, TYPE_0, "SRA C",             NULL            }, // 0x29
   {0, 0, 0, 0, False, TYPE_0, "SRA D",             NULL            }, // 0x2A
   {0, 0, 0, 0, False, TYPE_0, "SRA E",             NULL            }, // 0x2B
   {0, 0, 0, 0, False, TYPE_0, "SRA H",             NULL            }, // 0x2C
   {0, 0, 0, 0, False, TYPE_0, "SRA L",             NULL            }, // 0x2D
   {0, 0, 1, 1, False, TYPE_0, "SRA (HL)",          NULL            }, // 0x2E
   {0, 0, 0, 0, False, TYPE_0, "SRA A",             NULL            }, // 0x2F

   {0, 0, 0, 0, False, TYPE_0, "SLL B",             NULL            }, // 0x30
   {0, 0, 0, 0, False, TYPE_0, "SLL C",             NULL            }, // 0x31
   {0, 0, 0, 0, False, TYPE_0, "SLL D",             NULL            }, // 0x32
   {0, 0, 0, 0, False, TYPE_0, "SLL E",             NULL            }, // 0x33
   {0, 0, 0, 0, False, TYPE_0, "SLL H",             NULL            }, // 0x34
   {0, 0, 0, 0, False, TYPE_0, "SLL L",             NULL            }, // 0x35
   {0, 0, 1, 1, False, TYPE_0, "SLL (HL)",          NULL            }, // 0x36
   {0, 0, 0, 0, False, TYPE_0, "SLL A",             NULL            }, // 0x37
   {0, 0, 0, 0, False, TYPE_0, "SRL B",             NULL            }, // 0x38
   {0, 0, 0, 0, False, TYPE_0, "SRL C",             NULL            }, // 0x39
   {0, 0, 0, 0, False, TYPE_0, "SRL D",             NULL            }, // 0x3A
   {0, 0, 0, 0, False, TYPE_0, "SRL E",             NULL            }, // 0x3B
   {0, 0, 0, 0, False, TYPE_0, "SRL H",             NULL            }, // 0x3C
   {0, 0, 0, 0, False, TYPE_0, "SRL L",             NULL            }, // 0x3D
   {0, 0, 1, 1, False, TYPE_0, "SRL (HL)",          NULL            }, // 0x3E
   {0, 0, 0, 0, False, TYPE_0, "SRL A",             NULL            }, // 0x3F

   {0, 0, 0, 0, False, TYPE_0, "BIT 0,B",           NULL            }, // 0x40
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,C",           NULL            }, // 0x41
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,D",           NULL            }, // 0x42
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,E",           NULL            }, // 0x43
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,H",           NULL            }, // 0x44
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,L",           NULL            }, // 0x45
   {0, 0, 1, 0, False, TYPE_0, "BIT 0,(HL)",        NULL            }, // 0x46
   {0, 0, 0, 0, False, TYPE_0, "BIT 0,A",           NULL            }, // 0x47
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,B",           NULL            }, // 0x48
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,C",           NULL            }, // 0x49
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,D",           NULL            }, // 0x4A
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,E",           NULL            }, // 0x4B
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,H",           NULL            }, // 0x4C
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,L",           NULL            }, // 0x4D
   {0, 0, 1, 0, False, TYPE_0, "BIT 1,(HL)",        NULL            }, // 0x4E
   {0, 0, 0, 0, False, TYPE_0, "BIT 1,A",           NULL            }, // 0x4F

   {0, 0, 0, 0, False, TYPE_0, "BIT 2,B",           NULL            }, // 0x50
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,C",           NULL            }, // 0x51
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,D",           NULL            }, // 0x52
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,E",           NULL            }, // 0x53
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,H",           NULL            }, // 0x54
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,L",           NULL            }, // 0x55
   {0, 0, 1, 0, False, TYPE_0, "BIT 2,(HL)",        NULL            }, // 0x56
   {0, 0, 0, 0, False, TYPE_0, "BIT 2,A",           NULL            }, // 0x57
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,B",           NULL            }, // 0x58
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,C",           NULL            }, // 0x59
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,D",           NULL            }, // 0x5A
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,E",           NULL            }, // 0x5B
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,H",           NULL            }, // 0x5C
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,L",           NULL            }, // 0x5D
   {0, 0, 1, 0, False, TYPE_0, "BIT 3,(HL)",        NULL            }, // 0x5E
   {0, 0, 0, 0, False, TYPE_0, "BIT 3,A",           NULL            }, // 0x5F

   {0, 0, 0, 0, False, TYPE_0, "BIT 4,B",           NULL            }, // 0x60
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,C",           NULL            }, // 0x61
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,D",           NULL            }, // 0x62
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,E",           NULL            }, // 0x63
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,H",           NULL            }, // 0x64
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,L",           NULL            }, // 0x65
   {0, 0, 1, 0, False, TYPE_0, "BIT 4,(HL)",        NULL            }, // 0x66
   {0, 0, 0, 0, False, TYPE_0, "BIT 4,A",           NULL            }, // 0x67
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,B",           NULL            }, // 0x68
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,C",           NULL            }, // 0x69
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,D",           NULL            }, // 0x6A
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,E",           NULL            }, // 0x6B
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,H",           NULL            }, // 0x6C
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,L",           NULL            }, // 0x6D
   {0, 0, 1, 0, False, TYPE_0, "BIT 5,(HL)",        NULL            }, // 0x6E
   {0, 0, 0, 0, False, TYPE_0, "BIT 5,A",           NULL            }, // 0x6F

   {0, 0, 0, 0, False, TYPE_0, "BIT 6,B",           NULL            }, // 0x70
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,C",           NULL            }, // 0x71
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,D",           NULL            }, // 0x72
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,E",           NULL            }, // 0x73
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,H",           NULL            }, // 0x74
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,L",           NULL            }, // 0x75
   {0, 0, 1, 0, False, TYPE_0, "BIT 6,(HL)",        NULL            }, // 0x76
   {0, 0, 0, 0, False, TYPE_0, "BIT 6,A",           NULL            }, // 0x77
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,B",           NULL            }, // 0x78
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,C",           NULL            }, // 0x79
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,D",           NULL            }, // 0x7A
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,E",           NULL            }, // 0x7B
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,H",           NULL            }, // 0x7C
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,L",           NULL            }, // 0x7D
   {0, 0, 1, 0, False, TYPE_0, "BIT 7,(HL)",        NULL            }, // 0x7E
   {0, 0, 0, 0, False, TYPE_0, "BIT 7,A",           NULL            }, // 0x7F

   {0, 0, 0, 0, False, TYPE_0, "RES 0,B",           NULL            }, // 0x80
   {0, 0, 0, 0, False, TYPE_0, "RES 0,C",           NULL            }, // 0x81
   {0, 0, 0, 0, False, TYPE_0, "RES 0,D",           NULL            }, // 0x82
   {0, 0, 0, 0, False, TYPE_0, "RES 0,E",           NULL            }, // 0x83
   {0, 0, 0, 0, False, TYPE_0, "RES 0,H",           NULL            }, // 0x84
   {0, 0, 0, 0, False, TYPE_0, "RES 0,L",           NULL            }, // 0x85
   {0, 0, 1, 1, False, TYPE_0, "RES 0,(HL)",        NULL            }, // 0x86
   {0, 0, 0, 0, False, TYPE_0, "RES 0,A",           NULL            }, // 0x87
   {0, 0, 0, 0, False, TYPE_0, "RES 1,B",           NULL            }, // 0x88
   {0, 0, 0, 0, False, TYPE_0, "RES 1,C",           NULL            }, // 0x89
   {0, 0, 0, 0, False, TYPE_0, "RES 1,D",           NULL            }, // 0x8A
   {0, 0, 0, 0, False, TYPE_0, "RES 1,E",           NULL            }, // 0x8B
   {0, 0, 0, 0, False, TYPE_0, "RES 1,H",           NULL            }, // 0x8C
   {0, 0, 0, 0, False, TYPE_0, "RES 1,L",           NULL            }, // 0x8D
   {0, 0, 1, 1, False, TYPE_0, "RES 1,(HL)",        NULL            }, // 0x8E
   {0, 0, 0, 0, False, TYPE_0, "RES 1,A",           NULL            }, // 0x8F

   {0, 0, 0, 0, False, TYPE_0, "RES 2,B",           NULL            }, // 0x90
   {0, 0, 0, 0, False, TYPE_0, "RES 2,C",           NULL            }, // 0x91
   {0, 0, 0, 0, False, TYPE_0, "RES 2,D",           NULL            }, // 0x92
   {0, 0, 0, 0, False, TYPE_0, "RES 2,E",           NULL            }, // 0x93
   {0, 0, 0, 0, False, TYPE_0, "RES 2,H",           NULL            }, // 0x94
   {0, 0, 0, 0, False, TYPE_0, "RES 2,L",           NULL            }, // 0x95
   {0, 0, 1, 1, False, TYPE_0, "RES 2,(HL)",        NULL            }, // 0x96
   {0, 0, 0, 0, False, TYPE_0, "RES 2,A",           NULL            }, // 0x97
   {0, 0, 0, 0, False, TYPE_0, "RES 3,B",           NULL            }, // 0x98
   {0, 0, 0, 0, False, TYPE_0, "RES 3,C",           NULL            }, // 0x99
   {0, 0, 0, 0, False, TYPE_0, "RES 3,D",           NULL            }, // 0x9A
   {0, 0, 0, 0, False, TYPE_0, "RES 3,E",           NULL            }, // 0x9B
   {0, 0, 0, 0, False, TYPE_0, "RES 3,H",           NULL            }, // 0x9C
   {0, 0, 0, 0, False, TYPE_0, "RES 3,L",           NULL            }, // 0x9D
   {0, 0, 1, 1, False, TYPE_0, "RES 3,(HL)",        NULL            }, // 0x9E
   {0, 0, 0, 0, False, TYPE_0, "RES 3,A",           NULL            }, // 0x9F

   {0, 0, 0, 0, False, TYPE_0, "RES 4,B",           NULL            }, // 0xA0
   {0, 0, 0, 0, False, TYPE_0, "RES 4,C",           NULL            }, // 0xA1
   {0, 0, 0, 0, False, TYPE_0, "RES 4,D",           NULL            }, // 0xA2
   {0, 0, 0, 0, False, TYPE_0, "RES 4,E",           NULL            }, // 0xA3
   {0, 0, 0, 0, False, TYPE_0, "RES 4,H",           NULL            }, // 0xA4
   {0, 0, 0, 0, False, TYPE_0, "RES 4,L",           NULL            }, // 0xA5
   {0, 0, 1, 1, False, TYPE_0, "RES 4,(HL)",        NULL            }, // 0xA6
   {0, 0, 0, 0, False, TYPE_0, "RES 4,A",           NULL            }, // 0xA7
   {0, 0, 0, 0, False, TYPE_0, "RES 5,B",           NULL            }, // 0xA8
   {0, 0, 0, 0, False, TYPE_0, "RES 5,C",           NULL            }, // 0xA9
   {0, 0, 0, 0, False, TYPE_0, "RES 5,D",           NULL            }, // 0xAA
   {0, 0, 0, 0, False, TYPE_0, "RES 5,E",           NULL            }, // 0xAB
   {0, 0, 0, 0, False, TYPE_0, "RES 5,H",           NULL            }, // 0xAC
   {0, 0, 0, 0, False, TYPE_0, "RES 5,L",           NULL            }, // 0xAD
   {0, 0, 1, 1, False, TYPE_0, "RES 5,(HL)",        NULL            }, // 0xAE
   {0, 0, 0, 0, False, TYPE_0, "RES 5,A",           NULL            }, // 0xAF

   {0, 0, 0, 0, False, TYPE_0, "RES 6,B",           NULL            }, // 0xB0
   {0, 0, 0, 0, False, TYPE_0, "RES 6,C",           NULL            }, // 0xB1
   {0, 0, 0, 0, False, TYPE_0, "RES 6,D",           NULL            }, // 0xB2
   {0, 0, 0, 0, False, TYPE_0, "RES 6,E",           NULL            }, // 0xB3
   {0, 0, 0, 0, False, TYPE_0, "RES 6,H",           NULL            }, // 0xB4
   {0, 0, 0, 0, False, TYPE_0, "RES 6,L",           NULL            }, // 0xB5
   {0, 0, 1, 1, False, TYPE_0, "RES 6,(HL)",        NULL            }, // 0xB6
   {0, 0, 0, 0, False, TYPE_0, "RES 6,A",           NULL            }, // 0xB7
   {0, 0, 0, 0, False, TYPE_0, "RES 7,B",           NULL            }, // 0xB8
   {0, 0, 0, 0, False, TYPE_0, "RES 7,C",           NULL            }, // 0xB9
   {0, 0, 0, 0, False, TYPE_0, "RES 7,D",           NULL            }, // 0xBA
   {0, 0, 0, 0, False, TYPE_0, "RES 7,E",           NULL            }, // 0xBB
   {0, 0, 0, 0, False, TYPE_0, "RES 7,H",           NULL            }, // 0xBC
   {0, 0, 0, 0, False, TYPE_0, "RES 7,L",           NULL            }, // 0xBD
   {0, 0, 1, 1, False, TYPE_0, "RES 7,(HL)",        NULL            }, // 0xBE
   {0, 0, 0, 0, False, TYPE_0, "RES 7,A",           NULL            }, // 0xBF

   {0, 0, 0, 0, False, TYPE_0, "SET 0,B",           NULL            }, // 0xC0
   {0, 0, 0, 0, False, TYPE_0, "SET 0,C",           NULL            }, // 0xC1
   {0, 0, 0, 0, False, TYPE_0, "SET 0,D",           NULL            }, // 0xC2
   {0, 0, 0, 0, False, TYPE_0, "SET 0,E",           NULL            }, // 0xC3
   {0, 0, 0, 0, False, TYPE_0, "SET 0,H",           NULL            }, // 0xC4
   {0, 0, 0, 0, False, TYPE_0, "SET 0,L",           NULL            }, // 0xC5
   {0, 0, 1, 1, False, TYPE_0, "SET 0,(HL)",        NULL            }, // 0xC6
   {0, 0, 0, 0, False, TYPE_0, "SET 0,A",           NULL            }, // 0xC7
   {0, 0, 0, 0, False, TYPE_0, "SET 1,B",           NULL            }, // 0xC8
   {0, 0, 0, 0, False, TYPE_0, "SET 1,C",           NULL            }, // 0xC9
   {0, 0, 0, 0, False, TYPE_0, "SET 1,D",           NULL            }, // 0xCA
   {0, 0, 0, 0, False, TYPE_0, "SET 1,E",           NULL            }, // 0xCB
   {0, 0, 0, 0, False, TYPE_0, "SET 1,H",           NULL            }, // 0xCC
   {0, 0, 0, 0, False, TYPE_0, "SET 1,L",           NULL            }, // 0xCD
   {0, 0, 1, 1, False, TYPE_0, "SET 1,(HL)",        NULL            }, // 0xCE
   {0, 0, 0, 0, False, TYPE_0, "SET 1,A",           NULL            }, // 0xCF

   {0, 0, 0, 0, False, TYPE_0, "SET 2,B",           NULL            }, // 0xD0
   {0, 0, 0, 0, False, TYPE_0, "SET 2,C",           NULL            }, // 0xD1
   {0, 0, 0, 0, False, TYPE_0, "SET 2,D",           NULL            }, // 0xD2
   {0, 0, 0, 0, False, TYPE_0, "SET 2,E",           NULL            }, // 0xD3
   {0, 0, 0, 0, False, TYPE_0, "SET 2,H",           NULL            }, // 0xD4
   {0, 0, 0, 0, False, TYPE_0, "SET 2,L",           NULL            }, // 0xD5
   {0, 0, 1, 1, False, TYPE_0, "SET 2,(HL)",        NULL            }, // 0xD6
   {0, 0, 0, 0, False, TYPE_0, "SET 2,A",           NULL            }, // 0xD7
   {0, 0, 0, 0, False, TYPE_0, "SET 3,B",           NULL            }, // 0xD8
   {0, 0, 0, 0, False, TYPE_0, "SET 3,C",           NULL            }, // 0xD9
   {0, 0, 0, 0, False, TYPE_0, "SET 3,D",           NULL            }, // 0xDA
   {0, 0, 0, 0, False, TYPE_0, "SET 3,E",           NULL            }, // 0xDB
   {0, 0, 0, 0, False, TYPE_0, "SET 3,H",           NULL            }, // 0xDC
   {0, 0, 0, 0, False, TYPE_0, "SET 3,L",           NULL            }, // 0xDD
   {0, 0, 1, 1, False, TYPE_0, "SET 3,(HL)",        NULL            }, // 0xDE
   {0, 0, 0, 0, False, TYPE_0, "SET 3,A",           NULL            }, // 0xDF

   {0, 0, 0, 0, False, TYPE_0, "SET 4,B",           NULL            }, // 0xE0
   {0, 0, 0, 0, False, TYPE_0, "SET 4,C",           NULL            }, // 0xE1
   {0, 0, 0, 0, False, TYPE_0, "SET 4,D",           NULL            }, // 0xE2
   {0, 0, 0, 0, False, TYPE_0, "SET 4,E",           NULL            }, // 0xE3
   {0, 0, 0, 0, False, TYPE_0, "SET 4,H",           NULL            }, // 0xE4
   {0, 0, 0, 0, False, TYPE_0, "SET 4,L",           NULL            }, // 0xE5
   {0, 0, 1, 1, False, TYPE_0, "SET 4,(HL)",        NULL            }, // 0xE6
   {0, 0, 0, 0, False, TYPE_0, "SET 4,A",           NULL            }, // 0xE7
   {0, 0, 0, 0, False, TYPE_0, "SET 5,B",           NULL            }, // 0xE8
   {0, 0, 0, 0, False, TYPE_0, "SET 5,C",           NULL            }, // 0xE9
   {0, 0, 0, 0, False, TYPE_0, "SET 5,D",           NULL            }, // 0xEA
   {0, 0, 0, 0, False, TYPE_0, "SET 5,E",           NULL            }, // 0xEB
   {0, 0, 0, 0, False, TYPE_0, "SET 5,H",           NULL            }, // 0xEC
   {0, 0, 0, 0, False, TYPE_0, "SET 5,L",           NULL            }, // 0xED
   {0, 0, 1, 1, False, TYPE_0, "SET 5,(HL)",        NULL            }, // 0xEE
   {0, 0, 0, 0, False, TYPE_0, "SET 5,A",           NULL            }, // 0xEF

   {0, 0, 0, 0, False, TYPE_0, "SET 6,B",           NULL            }, // 0xF0
   {0, 0, 0, 0, False, TYPE_0, "SET 6,C",           NULL            }, // 0xF1
   {0, 0, 0, 0, False, TYPE_0, "SET 6,D",           NULL            }, // 0xF2
   {0, 0, 0, 0, False, TYPE_0, "SET 6,E",           NULL            }, // 0xF3
   {0, 0, 0, 0, False, TYPE_0, "SET 6,H",           NULL            }, // 0xF4
   {0, 0, 0, 0, False, TYPE_0, "SET 6,L",           NULL            }, // 0xF5
   {0, 0, 1, 1, False, TYPE_0, "SET 6,(HL)",        NULL            }, // 0xF6
   {0, 0, 0, 0, False, TYPE_0, "SET 6,A",           NULL            }, // 0xF7
   {0, 0, 0, 0, False, TYPE_0, "SET 7,B",           NULL            }, // 0xF8
   {0, 0, 0, 0, False, TYPE_0, "SET 7,C",           NULL            }, // 0xF9
   {0, 0, 0, 0, False, TYPE_0, "SET 7,D",           NULL            }, // 0xFA
   {0, 0, 0, 0, False, TYPE_0, "SET 7,E",           NULL            }, // 0xFB
   {0, 0, 0, 0, False, TYPE_0, "SET 7,H",           NULL            }, // 0xFC
   {0, 0, 0, 0, False, TYPE_0, "SET 7,L",           NULL            }, // 0xFD
   {0, 0, 1, 1, False, TYPE_0, "SET 7,(HL)",        NULL            }, // 0xFE
   {0, 0, 0, 0, False, TYPE_0, "SET 7,A",           NULL            }  // 0xFF
};

// Instructions with DD or FD prefix
InstrType index_instructions[256] = {
   UNDEFINED,                                                          // 0x00
   UNDEFINED,                                                          // 0x01
   UNDEFINED,                                                          // 0x02
   UNDEFINED,                                                          // 0x03
   UNDEFINED,                                                          // 0x04
   UNDEFINED,                                                          // 0x05
   UNDEFINED,                                                          // 0x06
   UNDEFINED,                                                          // 0x07
   UNDEFINED,                                                          // 0x08
   {0, 0, 0, 0, False, TYPE_1, "ADD %s,BC",         NULL            }, // 0x09
   UNDEFINED,                                                          // 0x0A
   UNDEFINED,                                                          // 0x0B
   UNDEFINED,                                                          // 0x0C
   UNDEFINED,                                                          // 0x0D
   UNDEFINED,                                                          // 0x0E
   UNDEFINED,                                                          // 0x0F

   UNDEFINED,                                                          // 0x10
   UNDEFINED,                                                          // 0x11
   UNDEFINED,                                                          // 0x12
   UNDEFINED,                                                          // 0x13
   UNDEFINED,                                                          // 0x14
   UNDEFINED,                                                          // 0x15
   UNDEFINED,                                                          // 0x16
   UNDEFINED,                                                          // 0x17
   UNDEFINED,                                                          // 0x18
   {0, 0, 0, 0, False, TYPE_1, "ADD %s,DE",         NULL            }, // 0x19
   UNDEFINED,                                                          // 0x1A
   UNDEFINED,                                                          // 0x1B
   UNDEFINED,                                                          // 0x1C
   UNDEFINED,                                                          // 0x1D
   UNDEFINED,                                                          // 0x1E
   UNDEFINED,                                                          // 0x1F

   UNDEFINED,                                                          // 0x20
   {0, 2, 0, 0, False, TYPE_4, "LD %s,%04Xh",       NULL            }, // 0x21
   {0, 2, 0, 2, False, TYPE_3, "LD (%04Xh),%s",     NULL            }, // 0x22
   {0, 0, 0, 0, False, TYPE_1, "INC %s",            NULL            }, // 0x23
   {0, 0, 0, 0, False, TYPE_1, "INC %sh",           NULL            }, // 0x24
   {0, 0, 0, 0, False, TYPE_1, "DEC %sh",           NULL            }, // 0x25
   {0, 1, 0, 0, False, TYPE_4, "LD %sh,%02Xh",      NULL            }, // 0x26
   UNDEFINED,                                                          // 0x27
   UNDEFINED,                                                          // 0x28
   {0, 0, 0, 0, False, TYPE_2, "ADD %s,%s",         NULL            }, // 0x29
   {0, 2, 2, 0, False, TYPE_4, "LD %s,(%04Xh)",     NULL            }, // 0x2A
   {0, 0, 0, 0, False, TYPE_1, "DEC %s",            NULL            }, // 0x2B
   {0, 0, 0, 0, False, TYPE_1, "INC %sl",           NULL            }, // 0x2C
   {0, 0, 0, 0, False, TYPE_1, "DEC %sl",           NULL            }, // 0x2D
   {0, 1, 0, 0, False, TYPE_4, "LD %sl,%02Xh",      NULL            }, // 0x2E
   UNDEFINED,                                                          // 0x2F

   UNDEFINED,                                                          // 0x30
   UNDEFINED,                                                          // 0x31
   UNDEFINED,                                                          // 0x32
   UNDEFINED,                                                          // 0x33
   {1, 0, 1, 1, False, TYPE_5, "INC (%s%+d)",       NULL            }, // 0x34
   {1, 0, 1, 1, False, TYPE_5, "DEC (%s%+d)",       NULL            }, // 0x35
   {1, 1, 0, 1, False, TYPE_6, "LD (%s%+d),%02xh",  NULL            }, // 0x36
   UNDEFINED,                                                          // 0x37
   UNDEFINED,                                                          // 0x38
   {0, 0, 0, 0, False, TYPE_1, "ADD %s,SP",         NULL            }, // 0x39
   UNDEFINED,                                                          // 0x3A
   UNDEFINED,                                                          // 0x3B
   UNDEFINED,                                                          // 0x3C
   UNDEFINED,                                                          // 0x3D
   UNDEFINED,                                                          // 0x3D
   UNDEFINED,                                                          // 0x3F

   UNDEFINED,                                                          // 0x40
   UNDEFINED,                                                          // 0x41
   UNDEFINED,                                                          // 0x42
   UNDEFINED,                                                          // 0x44
   {0, 0, 0, 0, False, TYPE_1, "LD B,%sh",          NULL            }, // 0x44
   {0, 0, 0, 0, False, TYPE_1, "LD B,%sl",          NULL            }, // 0x45
   {1, 0, 1, 0, False, TYPE_5, "LD B,(%s%+d)",      NULL            }, // 0x46
   UNDEFINED,                                                          // 0x47
   UNDEFINED,                                                          // 0x48
   UNDEFINED,                                                          // 0x49
   UNDEFINED,                                                          // 0x4A
   UNDEFINED,                                                          // 0x4B
   {0, 0, 0, 0, False, TYPE_1, "LD C,%sh",          NULL            }, // 0x4C
   {0, 0, 0, 0, False, TYPE_1, "LD C,%sl",          NULL            }, // 0x4D
   {1, 0, 1, 0, False, TYPE_5, "LD C,(%s%+d)",      NULL            }, // 0x4E
   UNDEFINED,                                                          // 0x4F

   UNDEFINED,                                                          // 0x50
   UNDEFINED,                                                          // 0x51
   UNDEFINED,                                                          // 0x52
   UNDEFINED,                                                          // 0x52
   {0, 0, 0, 0, False, TYPE_1, "LD D,%sh",          NULL            }, // 0x54
   {0, 0, 0, 0, False, TYPE_1, "LD D,%sl",          NULL            }, // 0x55
   {1, 0, 1, 0, False, TYPE_5, "LD D,(%s%+d)",      NULL            }, // 0x56
   UNDEFINED,                                                          // 0x57
   UNDEFINED,                                                          // 0x58
   UNDEFINED,                                                          // 0x59
   UNDEFINED,                                                          // 0x5A
   UNDEFINED,                                                          // 0x5B
   {0, 0, 0, 0, False, TYPE_1, "LD E,%sh",          NULL            }, // 0x5C
   {0, 0, 0, 0, False, TYPE_1, "LD E,%sl",          NULL            }, // 0x5D
   {1, 0, 1, 0, False, TYPE_5, "LD E,(%s%+d)",      NULL            }, // 0x5E
   UNDEFINED,                                                          // 0x5F

   {0, 0, 0, 0, False, TYPE_1, "LD %sh,B",          NULL            }, // 0x60
   {0, 0, 0, 0, False, TYPE_1, "LD %sh,C",          NULL            }, // 0x61
   {0, 0, 0, 0, False, TYPE_1, "LD %sh,D",          NULL            }, // 0x62
   {0, 0, 0, 0, False, TYPE_1, "LD %sh,E",          NULL            }, // 0x63
   {0, 0, 0, 0, False, TYPE_2, "LD %sh,%sh",        NULL            }, // 0x64
   {0, 0, 0, 0, False, TYPE_2, "LD %sh,%sl",        NULL            }, // 0x65
   {1, 0, 1, 0, False, TYPE_5, "LD H,(%s%+d)",      NULL            }, // 0x66
   {0, 0, 0, 0, False, TYPE_1, "LD %sh,A",          NULL            }, // 0x67
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,B",          NULL            }, // 0x68
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,C",          NULL            }, // 0x69
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,D",          NULL            }, // 0x6A
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,E",          NULL            }, // 0x6B
   {0, 0, 0, 0, False, TYPE_2, "LD %sl,%sh",        NULL            }, // 0x6C
   {0, 0, 0, 0, False, TYPE_2, "LD %sl,%sl",        NULL            }, // 0x6D
   {1, 0, 1, 0, False, TYPE_5, "LD L,(%s%+d)",      NULL            }, // 0x6E
   {0, 0, 0, 0, False, TYPE_1, "LD %sl,A",          NULL            }, // 0x6F

   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),B",      NULL            }, // 0x70
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),C",      NULL            }, // 0x71
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),D",      NULL            }, // 0x72
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),E",      NULL            }, // 0x73
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),H",      NULL            }, // 0x74
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),L",      NULL            }, // 0x75
   UNDEFINED,                                                          // 0x76
   {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),A",      NULL            }, // 0x77
   UNDEFINED,                                                          // 0x78
   UNDEFINED,                                                          // 0x79
   UNDEFINED,                                                          // 0x7A
   UNDEFINED,                                                          // 0x7B
   {0, 0, 0, 0, False, TYPE_1, "LD A,%sh",          NULL            }, // 0x7C
   {0, 0, 0, 0, False, TYPE_1, "LD A,%sl",          NULL            }, // 0x7D
   {1, 0, 1, 0, False, TYPE_5, "LD A,(%s%+d)",      NULL            }, // 0x7E
   UNDEFINED,                                                          // 0x7F

   UNDEFINED,                                                          // 0x80
   UNDEFINED,                                                          // 0x81
   UNDEFINED,                                                          // 0x82
   UNDEFINED,                                                          // 0x83
   {0, 0, 0, 0, False, TYPE_1, "ADD A,%sh",         NULL            }, // 0x84
   {0, 0, 0, 0, False, TYPE_1, "ADD A,%sl",         NULL            }, // 0x85
   {1, 0, 1, 0, False, TYPE_5, "ADD A,(%s%+d)",     NULL            }, // 0x86
   UNDEFINED,                                                          // 0x87
   UNDEFINED,                                                          // 0x88
   UNDEFINED,                                                          // 0x89
   UNDEFINED,                                                          // 0x8A
   UNDEFINED,                                                          // 0x8B
   {0, 0, 0, 0, False, TYPE_1, "ADC A,%sh",         NULL            }, // 0x8C
   {0, 0, 0, 0, False, TYPE_1, "ADC A,%sl",         NULL            }, // 0x8D
   {1, 0, 1, 0, False, TYPE_5, "ADC A,(%s%+d)",     NULL            }, // 0x8E
   UNDEFINED,                                                          // 0x8F

   UNDEFINED,                                                          // 0x90
   UNDEFINED,                                                          // 0x91
   UNDEFINED,                                                          // 0x92
   UNDEFINED,                                                          // 0x93
   {0, 0, 0, 0, False, TYPE_1, "SUB %sh",           NULL            }, // 0x94
   {0, 0, 0, 0, False, TYPE_1, "SUB %sl",           NULL            }, // 0x95
   {1, 0, 1, 0, False, TYPE_5, "SUB (%s%+d)",       NULL            }, // 0x96
   UNDEFINED,                                                          // 0x97
   UNDEFINED,                                                          // 0x98
   UNDEFINED,                                                          // 0x99
   UNDEFINED,                                                          // 0x9A
   UNDEFINED,                                                          // 0x9B
   {0, 0, 0, 0, False, TYPE_1, "SBC A,%sh",         NULL            }, // 0x9C
   {0, 0, 0, 0, False, TYPE_1, "SBC A,%sl",         NULL            }, // 0x9D
   {1, 0, 1, 0, False, TYPE_5, "SBC A,(%s%+d)",     NULL            }, // 0x9E
   UNDEFINED,                                                          // 0x9F

   UNDEFINED,                                                          // 0xA0
   UNDEFINED,                                                          // 0xA1
   UNDEFINED,                                                          // 0xA2
   UNDEFINED,                                                          // 0xA3
   {0, 0, 0, 0, False, TYPE_1, "AND %sh",           NULL            }, // 0xA4
   {0, 0, 0, 0, False, TYPE_1, "AND %sl",           NULL            }, // 0xA5
   {1, 0, 1, 0, False, TYPE_5, "AND (%s%+d)",       NULL            }, // 0xA6
   UNDEFINED,                                                          // 0xA7
   UNDEFINED,                                                          // 0xA8
   UNDEFINED,                                                          // 0xA9
   UNDEFINED,                                                          // 0xAA
   UNDEFINED,                                                          // 0xAB
   {0, 0, 0, 0, False, TYPE_1, "XOR %sh",           NULL            }, // 0xAC
   {0, 0, 0, 0, False, TYPE_1, "XOR %sl",           NULL            }, // 0xAD
   {1, 0, 1, 0, False, TYPE_5, "XOR (%s%+d)",       NULL            }, // 0xAE
   UNDEFINED,                                                          // 0xEF

   UNDEFINED,                                                          // 0xB0
   UNDEFINED,                                                          // 0xB1
   UNDEFINED,                                                          // 0xB2
   UNDEFINED,                                                          // 0xB3
   {0, 0, 0, 0, False, TYPE_1, "OR %sh",            NULL            }, // 0xB4
   {0, 0, 0, 0, False, TYPE_1, "OR %sl",            NULL            }, // 0xB5
   {1, 0, 1, 0, False, TYPE_5, "OR (%s%+d)",        NULL            }, // 0xB6
   UNDEFINED,                                                          // 0xB7
   UNDEFINED,                                                          // 0xB8
   UNDEFINED,                                                          // 0xB9
   UNDEFINED,                                                          // 0xBA
   UNDEFINED,                                                          // 0xBB
   {0, 0, 0, 0, False, TYPE_1, "CP %sh",            NULL            }, // 0xBC
   {0, 0, 0, 0, False, TYPE_1, "CP %sl",            NULL            }, // 0xBD
   {1, 0, 1, 0, False, TYPE_5, "CP (%s%+d)",        NULL            }, // 0xBE
   UNDEFINED,                                                          // 0xBF

   UNDEFINED,                                                          // 0xC0
   UNDEFINED,                                                          // 0xC1
   UNDEFINED,                                                          // 0xC2
   UNDEFINED,                                                          // 0xC3
   UNDEFINED,                                                          // 0xC4
   UNDEFINED,                                                          // 0xC5
   UNDEFINED,                                                          // 0xC6
   UNDEFINED,                                                          // 0xC7
   UNDEFINED,                                                          // 0xC8
   UNDEFINED,                                                          // 0xC9
   UNDEFINED,                                                          // 0xCA
   UNDEFINED,                                                          // 0xCB
   UNDEFINED,                                                          // 0xCC
   UNDEFINED,                                                          // 0xCD
   UNDEFINED,                                                          // 0xCE
   UNDEFINED,                                                          // 0xCF

   UNDEFINED,                                                          // 0xD0
   UNDEFINED,                                                          // 0xD1
   UNDEFINED,                                                          // 0xD2
   UNDEFINED,                                                          // 0xD3
   UNDEFINED,                                                          // 0xD4
   UNDEFINED,                                                          // 0xD5
   UNDEFINED,                                                          // 0xD6
   UNDEFINED,                                                          // 0xD7
   UNDEFINED,                                                          // 0xD8
   UNDEFINED,                                                          // 0xD9
   UNDEFINED,                                                          // 0xDA
   UNDEFINED,                                                          // 0xDB
   UNDEFINED,                                                          // 0xDC
   UNDEFINED,                                                          // 0xDD
   UNDEFINED,                                                          // 0xDE
   UNDEFINED,                                                          // 0xDF

   UNDEFINED,                                                          // 0xE0
   {0, 0, 2, 0, False, TYPE_1, "POP %s",            NULL            }, // 0xE1
   UNDEFINED,                                                          // 0xE2
   {0, 0, 2, 2, False, TYPE_1, "EX (SP),%s",        NULL            }, // 0xE3
   UNDEFINED,                                                          // 0xE4
   {0, 0, 0,-2, False, TYPE_1, "PUSH %s",           NULL            }, // 0xE5
   UNDEFINED,                                                          // 0xE6
   UNDEFINED,                                                          // 0xE7
   UNDEFINED,                                                          // 0xE8
   {0, 0, 0, 0, False, TYPE_1, "JP (%s)",           NULL            }, // 0xE9
   UNDEFINED,                                                          // 0xEA
   UNDEFINED,                                                          // 0xEB
   UNDEFINED,                                                          // 0xEC
   UNDEFINED,                                                          // 0xED
   UNDEFINED,                                                          // 0xEE
   UNDEFINED,                                                          // 0xEF

   UNDEFINED,                                                          // 0xF0
   UNDEFINED,                                                          // 0xF1
   UNDEFINED,                                                          // 0xF2
   UNDEFINED,                                                          // 0xF3
   UNDEFINED,                                                          // 0xF4
   UNDEFINED,                                                          // 0xF5
   UNDEFINED,                                                          // 0xF7
   UNDEFINED,                                                          // 0xF7
   UNDEFINED,                                                          // 0xF8
   {0, 0, 0, 0, False, TYPE_1, "LD SP,%s",          NULL            }, // 0xF9
   UNDEFINED,                                                          // 0xFA
   UNDEFINED,                                                          // 0xFB
   UNDEFINED,                                                          // 0xFC
   UNDEFINED,                                                          // 0xFD
   UNDEFINED,                                                          // 0xFE
   UNDEFINED                                                           // 0xFF
};

// Instructions with DD CB or FD CB prefix.
// For these instructions, the displacement precedes the opcode byte.
// This is handled as a special case in the code, and thus the entries
// in this table specify 0 for the displacement length.
InstrType index_bit_instructions[256] = {
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),B",     NULL            }, // 0x00
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),C",     NULL            }, // 0x01
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),D",     NULL            }, // 0x02
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),E",     NULL            }, // 0x03
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),H",     NULL            }, // 0x04
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),L",     NULL            }, // 0x05
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d)",       NULL            }, // 0x06
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),A",     NULL            }, // 0x07
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),B",     NULL            }, // 0x08
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),C",     NULL            }, // 0x09
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),D",     NULL            }, // 0x0A
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),E",     NULL            }, // 0x0B
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),H",     NULL            }, // 0x0C
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),L",     NULL            }, // 0x0D
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d)",       NULL            }, // 0x0E
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),A",     NULL            }, // 0x0F

   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),B",      NULL            }, // 0x10
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),C",      NULL            }, // 0x11
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),D",      NULL            }, // 0x12
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),E",      NULL            }, // 0x13
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),H",      NULL            }, // 0x14
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),L",      NULL            }, // 0x15
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d)",        NULL            }, // 0x16
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),A",      NULL            }, // 0x17
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),B",      NULL            }, // 0x18
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),C",      NULL            }, // 0x19
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),D",      NULL            }, // 0x1A
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),E",      NULL            }, // 0x1B
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),H",      NULL            }, // 0x1C
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),L",      NULL            }, // 0x1D
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d)",        NULL            }, // 0x1E
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),A",      NULL            }, // 0x1F

   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),B",     NULL            }, // 0x20
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),C",     NULL            }, // 0x21
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),D",     NULL            }, // 0x22
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),E",     NULL            }, // 0x23
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),H",     NULL            }, // 0x24
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),L",     NULL            }, // 0x25
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d)",       NULL            }, // 0x26
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),A",     NULL            }, // 0x27
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),B",     NULL            }, // 0x28
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),C",     NULL            }, // 0x29
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),D",     NULL            }, // 0x2A
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),E",     NULL            }, // 0x2B
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),H",     NULL            }, // 0x2C
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),L",     NULL            }, // 0x2D
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d)",       NULL            }, // 0x2E
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),A",     NULL            }, // 0x2F

   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),B",     NULL            }, // 0x30
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),C",     NULL            }, // 0x31
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),D",     NULL            }, // 0x32
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),E",     NULL            }, // 0x33
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),H",     NULL            }, // 0x34
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),L",     NULL            }, // 0x35
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d)",       NULL            }, // 0x36
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),A",     NULL            }, // 0x37
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),B",     NULL            }, // 0x38
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),C",     NULL            }, // 0x39
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),D",     NULL            }, // 0x3A
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),E",     NULL            }, // 0x3B
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),H",     NULL            }, // 0x3C
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),L",     NULL            }, // 0x3D
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d)",       NULL            }, // 0x3E
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),A",     NULL            }, // 0x3F

   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     NULL            }, // 0x40
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     NULL            }, // 0x41
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     NULL            }, // 0x42
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     NULL            }, // 0x43
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     NULL            }, // 0x44
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     NULL            }, // 0x45
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     NULL            }, // 0x46
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)",     NULL            }, // 0x47
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     NULL            }, // 0x48
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     NULL            }, // 0x49
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     NULL            }, // 0x4A
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     NULL            }, // 0x4B
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     NULL            }, // 0x4C
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     NULL            }, // 0x4D
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     NULL            }, // 0x4E
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)",     NULL            }, // 0x4F

   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     NULL            }, // 0x50
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     NULL            }, // 0x51
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     NULL            }, // 0x52
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     NULL            }, // 0x53
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     NULL            }, // 0x54
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     NULL            }, // 0x55
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     NULL            }, // 0x56
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)",     NULL            }, // 0x57
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     NULL            }, // 0x58
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     NULL            }, // 0x59
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     NULL            }, // 0x5A
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     NULL            }, // 0x5B
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     NULL            }, // 0x5C
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     NULL            }, // 0x5D
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     NULL            }, // 0x5E
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)",     NULL            }, // 0x5F

   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     NULL            }, // 0x60
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     NULL            }, // 0x61
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     NULL            }, // 0x62
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     NULL            }, // 0x63
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     NULL            }, // 0x64
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     NULL            }, // 0x65
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     NULL            }, // 0x66
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)",     NULL            }, // 0x67
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     NULL            }, // 0x68
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     NULL            }, // 0x69
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     NULL            }, // 0x6A
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     NULL            }, // 0x6B
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     NULL            }, // 0x6C
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     NULL            }, // 0x6D
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     NULL            }, // 0x6E
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)",     NULL            }, // 0x6F

   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     NULL            }, // 0x70
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     NULL            }, // 0x71
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     NULL            }, // 0x72
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     NULL            }, // 0x73
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     NULL            }, // 0x74
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     NULL            }, // 0x75
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     NULL            }, // 0x76
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)",     NULL            }, // 0x77
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     NULL            }, // 0x78
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     NULL            }, // 0x79
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     NULL            }, // 0x7A
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     NULL            }, // 0x7B
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     NULL            }, // 0x7C
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     NULL            }, // 0x7D
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     NULL            }, // 0x7E
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)",     NULL            }, // 0x7F

   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),B",   NULL            }, // 0x80
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),C",   NULL            }, // 0x81
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),D",   NULL            }, // 0x82
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),E",   NULL            }, // 0x83
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),H",   NULL            }, // 0x84
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),L",   NULL            }, // 0x85
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d)",     NULL            }, // 0x86
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),A",   NULL            }, // 0x87
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),B",   NULL            }, // 0x88
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),C",   NULL            }, // 0x89
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),D",   NULL            }, // 0x8A
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),E",   NULL            }, // 0x8B
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),H",   NULL            }, // 0x8C
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),L",   NULL            }, // 0x8D
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d)",     NULL            }, // 0x8E
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),A",   NULL            }, // 0x8F

   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),B",   NULL            }, // 0x90
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),C",   NULL            }, // 0x91
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),D",   NULL            }, // 0x92
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),E",   NULL            }, // 0x93
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),H",   NULL            }, // 0x94
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),L",   NULL            }, // 0x95
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d)",     NULL            }, // 0x96
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),A",   NULL            }, // 0x97
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),B",   NULL            }, // 0x98
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),C",   NULL            }, // 0x99
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),D",   NULL            }, // 0x9A
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),E",   NULL            }, // 0x9B
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),H",   NULL            }, // 0x9C
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),L",   NULL            }, // 0x9D
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d)",     NULL            }, // 0x9E
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),A",   NULL            }, // 0x9F

   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),B",   NULL            }, // 0xA0
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),C",   NULL            }, // 0xA1
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),D",   NULL            }, // 0xA2
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),E",   NULL            }, // 0xA3
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),H",   NULL            }, // 0xA4
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),L",   NULL            }, // 0xA5
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d)",     NULL            }, // 0xA6
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),A",   NULL            }, // 0xA7
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),B",   NULL            }, // 0xA8
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),C",   NULL            }, // 0xA9
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),D",   NULL            }, // 0xAA
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),E",   NULL            }, // 0xAB
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),H",   NULL            }, // 0xAC
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),L",   NULL            }, // 0xAD
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d)",     NULL            }, // 0xAE
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),A",   NULL            }, // 0xAF

   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),B",   NULL            }, // 0xB0
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),C",   NULL            }, // 0xB1
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),D",   NULL            }, // 0xB2
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),E",   NULL            }, // 0xB3
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),H",   NULL            }, // 0xB4
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),L",   NULL            }, // 0xB5
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d)",     NULL            }, // 0xB6
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),A",   NULL            }, // 0xB7
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),B",   NULL            }, // 0xB8
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),C",   NULL            }, // 0xB9
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),D",   NULL            }, // 0xBA
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),E",   NULL            }, // 0xBB
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),H",   NULL            }, // 0xBC
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),L",   NULL            }, // 0xBD
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d)",     NULL            }, // 0xBE
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),A",   NULL            }, // 0xBF

   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),B",   NULL            }, // 0xC0
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),C",   NULL            }, // 0xC1
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),D",   NULL            }, // 0xC2
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),E",   NULL            }, // 0xC3
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),H",   NULL            }, // 0xC4
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),L",   NULL            }, // 0xC5
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d)",     NULL            }, // 0xC6
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),A",   NULL            }, // 0xC7
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),B",   NULL            }, // 0xC8
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),C",   NULL            }, // 0xC9
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),D",   NULL            }, // 0xCA
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),E",   NULL            }, // 0xCB
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),H",   NULL            }, // 0xCC
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),L",   NULL            }, // 0xCD
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d)",     NULL            }, // 0xCE
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),A",   NULL            }, // 0xCF

   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),B",   NULL            }, // 0xD0
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),C",   NULL            }, // 0xD1
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),D",   NULL            }, // 0xD2
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),E",   NULL            }, // 0xD3
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),H",   NULL            }, // 0xD4
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),L",   NULL            }, // 0xD5
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d)",     NULL            }, // 0xD6
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),A",   NULL            }, // 0xD7
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),B",   NULL            }, // 0xD8
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),C",   NULL            }, // 0xD9
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),D",   NULL            }, // 0xDA
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),E",   NULL            }, // 0xDB
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),H",   NULL            }, // 0xDC
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),L",   NULL            }, // 0xDD
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d)",     NULL            }, // 0xDE
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),A",   NULL            }, // 0xDF

   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),B",   NULL            }, // 0xE0
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),C",   NULL            }, // 0xE1
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),D",   NULL            }, // 0xE2
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),E",   NULL            }, // 0xE3
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),H",   NULL            }, // 0xE4
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),L",   NULL            }, // 0xE5
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d)",     NULL            }, // 0xE6
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),A",   NULL            }, // 0xE7
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),B",   NULL            }, // 0xE8
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),C",   NULL            }, // 0xE9
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),D",   NULL            }, // 0xEA
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),E",   NULL            }, // 0xEB
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),H",   NULL            }, // 0xEC
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),L",   NULL            }, // 0xED
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d)",     NULL            }, // 0xEE
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),A",   NULL            }, // 0xEF

   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),B",   NULL            }, // 0xF0
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),C",   NULL            }, // 0xF1
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),D",   NULL            }, // 0xF2
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),E",   NULL            }, // 0xF3
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),H",   NULL            }, // 0xF4
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),L",   NULL            }, // 0xF5
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d)",     NULL            }, // 0xF6
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),A",   NULL            }, // 0xF7
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),B",   NULL            }, // 0xF8
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),C",   NULL            }, // 0xF9
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),D",   NULL            }, // 0xFA
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),E",   NULL            }, // 0xFB
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),H",   NULL            }, // 0xFC
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),L",   NULL            }, // 0xFD
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d)",     NULL            }, // 0xFE
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),A",   NULL            }  // 0xFF
};


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
