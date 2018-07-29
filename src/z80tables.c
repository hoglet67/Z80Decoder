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
#include "z80tables.h"

#define UNDEFINED {-1, -1, -1, -1, False, TYPE_0, "???"}

// Instructions without a prefix
InstrType main_instructions[256] = {
    {0, 0, 0, 0, False, TYPE_0, "NOP"},               // 0x00
    {0, 2, 0, 0, False, TYPE_8, "LD BC,%04Xh"},       // 0x01
    {0, 0, 0, 1, False, TYPE_0, "LD (BC),A"},         // 0x02
    {0, 0, 0, 0, False, TYPE_0, "INC BC"},            // 0x03
    {0, 0, 0, 0, False, TYPE_0, "INC B"},             // 0x04
    {0, 0, 0, 0, False, TYPE_0, "DEC B"},             // 0x05
    {0, 1, 0, 0, False, TYPE_8, "LD B,%02Xh"},        // 0x06
    {0, 0, 0, 0, False, TYPE_0, "RLCA"},              // 0x07
    {0, 0, 0, 0, False, TYPE_0, "EX AF,AF'"},         // 0x08
    {0, 0, 0, 0, False, TYPE_0, "ADD HL,BC"},         // 0x09
    {0, 0, 1, 0, False, TYPE_0, "LD A,(BC)"},         // 0x0A
    {0, 0, 0, 0, False, TYPE_0, "DEC BC"},            // 0x0B
    {0, 0, 0, 0, False, TYPE_0, "INC C"},             // 0x0C
    {0, 0, 0, 0, False, TYPE_0, "DEC C"},             // 0x0D
    {0, 1, 0, 0, False, TYPE_8, "LD C,%02Xh"},        // 0x0E
    {0, 0, 0, 0, False, TYPE_0, "RRCA"},              // 0x0F

    {1, 0, 0, 0, False, TYPE_7, "DJNZ $%+d"},         // 0x10
    {0, 2, 0, 0, False, TYPE_8, "LD DE,%04Xh"},       // 0x11
    {0, 0, 0, 1, False, TYPE_0, "LD (DE),A"},         // 0x12
    {0, 0, 0, 0, False, TYPE_0, "INC DE"},            // 0x13
    {0, 0, 0, 0, False, TYPE_0, "INC D"},             // 0x14
    {0, 0, 0, 0, False, TYPE_0, "DEC D"},             // 0x15
    {0, 1, 0, 0, False, TYPE_8, "LD D,%02Xh"},        // 0x16
    {0, 0, 0, 0, False, TYPE_0, "RLA"},               // 0x17
    {1, 0, 0, 0, False, TYPE_7, "JR $%+d"},           // 0x18
    {0, 0, 0, 0, False, TYPE_0, "ADD HL,DE"},         // 0x19
    {0, 0, 1, 0, False, TYPE_0, "LD A,(DE)"},         // 0x1A
    {0, 0, 0, 0, False, TYPE_0, "DEC DE"},            // 0x1B
    {0, 0, 0, 0, False, TYPE_0, "INC E"},             // 0x1C
    {0, 0, 0, 0, False, TYPE_0, "DEC E"},             // 0x1D
    {0, 1, 0, 0, False, TYPE_8, "LD E,%02Xh"},        // 0x1E
    {0, 0, 0, 0, False, TYPE_0, "RRA"},               // 0x1F

    {1, 0, 0, 0, False, TYPE_7, "JR NZ,$%+d"},        // 0x20
    {0, 2, 0, 0, False, TYPE_8, "LD HL,%04Xh"},       // 0x21
    {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),HL"},     // 0x22
    {0, 0, 0, 0, False, TYPE_0, "INC HL"},            // 0x23
    {0, 0, 0, 0, False, TYPE_0, "INC H"},             // 0x24
    {0, 0, 0, 0, False, TYPE_0, "DEC H"},             // 0x25
    {0, 1, 0, 0, False, TYPE_8, "LD H,%02Xh"},        // 0x26
    {0, 0, 0, 0, False, TYPE_0, "DAA"},               // 0x27
    {1, 0, 0, 0, False, TYPE_7, "JR Z,$%+d"},         // 0x28
    {0, 0, 0, 0, False, TYPE_0, "ADD HL,HL"},         // 0x29
    {0, 2, 2, 0, False, TYPE_8, "LD HL,(%04Xh)"},     // 0x2A
    {0, 0, 0, 0, False, TYPE_0, "DEC HL"},            // 0x2B
    {0, 0, 0, 0, False, TYPE_0, "INC L"},             // 0x2C
    {0, 0, 0, 0, False, TYPE_0, "DEC L"},             // 0x2D
    {0, 1, 0, 0, False, TYPE_8, "LD L,%02Xh"},        // 0x2E
    {0, 0, 0, 0, False, TYPE_0, "CPL"},               // 0x2F

    {1, 0, 0, 0, False, TYPE_7, "JR NC,$%+d"},        // 0x30
    {0, 2, 0, 0, False, TYPE_8, "LD SP,%04Xh"},       // 0x31
    {0, 2, 0, 1, False, TYPE_8, "LD (%04Xh),A"},      // 0x32
    {0, 0, 0, 0, False, TYPE_0, "INC SP"},            // 0x33
    {0, 0, 1, 1, False, TYPE_0, "INC (HL)"},          // 0x34
    {0, 0, 1, 1, False, TYPE_0, "DEC (HL)"},          // 0x35
    {0, 1, 0, 1, False, TYPE_8, "LD (HL),%02Xh"},     // 0x36
    {0, 0, 0, 0, False, TYPE_0, "SCF"},               // 0x37
    {1, 0, 0, 0, False, TYPE_7, "JR C,$%+d"},         // 0x38
    {0, 0, 0, 0, False, TYPE_0, "ADD HL,SP"},         // 0x39
    {0, 2, 1, 0, False, TYPE_8, "LD A,(%04Xh)"},      // 0x3A
    {0, 0, 0, 0, False, TYPE_0, "DEC SP"},            // 0x3B
    {0, 0, 0, 0, False, TYPE_0, "INC A"},             // 0x3C
    {0, 0, 0, 0, False, TYPE_0, "DEC A"},             // 0x3D
    {0, 1, 0, 0, False, TYPE_8, "LD A,%02Xh"},        // 0x3E
    {0, 0, 0, 0, False, TYPE_0, "CCF"},               // 0x3F

    {0, 0, 0, 0, False, TYPE_0, "LD B,B"},            // 0x40
    {0, 0, 0, 0, False, TYPE_0, "LD B,C"},            // 0x41
    {0, 0, 0, 0, False, TYPE_0, "LD B,D"},            // 0x42
    {0, 0, 0, 0, False, TYPE_0, "LD B,E"},            // 0x43
    {0, 0, 0, 0, False, TYPE_0, "LD B,H"},            // 0x44
    {0, 0, 0, 0, False, TYPE_0, "LD B,L"},            // 0x45
    {0, 0, 1, 0, False, TYPE_0, "LD B,(HL)"},         // 0x46
    {0, 0, 0, 0, False, TYPE_0, "LD B,A"},            // 0x47
    {0, 0, 0, 0, False, TYPE_0, "LD C,B"},            // 0x48
    {0, 0, 0, 0, False, TYPE_0, "LD C,C"},            // 0x49
    {0, 0, 0, 0, False, TYPE_0, "LD C,D"},            // 0x4A
    {0, 0, 0, 0, False, TYPE_0, "LD C,E"},            // 0x4B
    {0, 0, 0, 0, False, TYPE_0, "LD C,H"},            // 0x4C
    {0, 0, 0, 0, False, TYPE_0, "LD C,L"},            // 0x4D
    {0, 0, 1, 0, False, TYPE_0, "LD C,(HL)"},         // 0x4E
    {0, 0, 0, 0, False, TYPE_0, "LD C,A"},            // 0x4F

    {0, 0, 0, 0, False, TYPE_0, "LD D,B"},            // 0x50
    {0, 0, 0, 0, False, TYPE_0, "LD D,C"},            // 0x51
    {0, 0, 0, 0, False, TYPE_0, "LD D,D"},            // 0x52
    {0, 0, 0, 0, False, TYPE_0, "LD D,E"},            // 0x53
    {0, 0, 0, 0, False, TYPE_0, "LD D,H"},            // 0x54
    {0, 0, 0, 0, False, TYPE_0, "LD D,L"},            // 0x55
    {0, 0, 1, 0, False, TYPE_0, "LD D,(HL)"},         // 0x56
    {0, 0, 0, 0, False, TYPE_0, "LD D,A"},            // 0x57
    {0, 0, 0, 0, False, TYPE_0, "LD E,B"},            // 0x58
    {0, 0, 0, 0, False, TYPE_0, "LD E,C"},            // 0x59
    {0, 0, 0, 0, False, TYPE_0, "LD E,D"},            // 0x5A
    {0, 0, 0, 0, False, TYPE_0, "LD E,E"},            // 0x5B
    {0, 0, 0, 0, False, TYPE_0, "LD E,H"},            // 0x5C
    {0, 0, 0, 0, False, TYPE_0, "LD E,L"},            // 0x5D
    {0, 0, 1, 0, False, TYPE_0, "LD E,(HL)"},         // 0x5E
    {0, 0, 0, 0, False, TYPE_0, "LD E,A"},            // 0x5F

    {0, 0, 0, 0, False, TYPE_0, "LD H,B"},            // 0x60
    {0, 0, 0, 0, False, TYPE_0, "LD H,C"},            // 0x61
    {0, 0, 0, 0, False, TYPE_0, "LD H,D"},            // 0x62
    {0, 0, 0, 0, False, TYPE_0, "LD H,E"},            // 0x63
    {0, 0, 0, 0, False, TYPE_0, "LD H,H"},            // 0x64
    {0, 0, 0, 0, False, TYPE_0, "LD H,L"},            // 0x65
    {0, 0, 1, 0, False, TYPE_0, "LD H,(HL)"},         // 0x66
    {0, 0, 0, 0, False, TYPE_0, "LD H,A"},            // 0x67
    {0, 0, 0, 0, False, TYPE_0, "LD L,B"},            // 0x68
    {0, 0, 0, 0, False, TYPE_0, "LD L,C"},            // 0x69
    {0, 0, 0, 0, False, TYPE_0, "LD L,D"},            // 0x6A
    {0, 0, 0, 0, False, TYPE_0, "LD L,E"},            // 0x6B
    {0, 0, 0, 0, False, TYPE_0, "LD L,H"},            // 0x6C
    {0, 0, 0, 0, False, TYPE_0, "LD L,L"},            // 0x6D
    {0, 0, 1, 0, False, TYPE_0, "LD L,(HL)"},         // 0x6E
    {0, 0, 0, 0, False, TYPE_0, "LD L,A"},            // 0x6F

    {0, 0, 0, 1, False, TYPE_0, "LD (HL),B"},         // 0x70
    {0, 0, 0, 1, False, TYPE_0, "LD (HL),C"},         // 0x71
    {0, 0, 0, 1, False, TYPE_0, "LD (HL),D"},         // 0x72
    {0, 0, 0, 1, False, TYPE_0, "LD (HL),E"},         // 0x73
    {0, 0, 0, 1, False, TYPE_0, "LD (HL),H"},         // 0x74
    {0, 0, 0, 1, False, TYPE_0, "LD (HL),L"},         // 0x75
    {0, 0, 0, 0, False, TYPE_0, "HALT"},              // 0x76
    {0, 0, 0, 1, False, TYPE_0, "LD (HL),A"},         // 0x77
    {0, 0, 0, 0, False, TYPE_0, "LD A,B"},            // 0x78
    {0, 0, 0, 0, False, TYPE_0, "LD A,C"},            // 0x79
    {0, 0, 0, 0, False, TYPE_0, "LD A,D"},            // 0x7A
    {0, 0, 0, 0, False, TYPE_0, "LD A,E"},            // 0x7B
    {0, 0, 0, 0, False, TYPE_0, "LD A,H"},            // 0x7C
    {0, 0, 0, 0, False, TYPE_0, "LD A,L"},            // 0x7D
    {0, 0, 1, 0, False, TYPE_0, "LD A,(HL)"},         // 0x7E
    {0, 0, 0, 0, False, TYPE_0, "LD A,A"},            // 0x7F

    {0, 0, 0, 0, False, TYPE_0, "ADD A,B"},           // 0x80
    {0, 0, 0, 0, False, TYPE_0, "ADD A,C"},           // 0x81
    {0, 0, 0, 0, False, TYPE_0, "ADD A,D"},           // 0x82
    {0, 0, 0, 0, False, TYPE_0, "ADD A,E"},           // 0x83
    {0, 0, 0, 0, False, TYPE_0, "ADD A,H"},           // 0x84
    {0, 0, 0, 0, False, TYPE_0, "ADD A,L"},           // 0x85
    {0, 0, 1, 0, False, TYPE_0, "ADD A,(HL)"},        // 0x86
    {0, 0, 0, 0, False, TYPE_0, "ADD A,A"},           // 0x87
    {0, 0, 0, 0, False, TYPE_0, "ADC A,B"},           // 0x88
    {0, 0, 0, 0, False, TYPE_0, "ADC A,C"},           // 0x89
    {0, 0, 0, 0, False, TYPE_0, "ADC A,D"},           // 0x8A
    {0, 0, 0, 0, False, TYPE_0, "ADC A,E"},           // 0x8B
    {0, 0, 0, 0, False, TYPE_0, "ADC A,H"},           // 0x8C
    {0, 0, 0, 0, False, TYPE_0, "ADC A,L"},           // 0x8D
    {0, 0, 1, 0, False, TYPE_0, "ADC A,(HL)"},        // 0x8E
    {0, 0, 0, 0, False, TYPE_0, "ADC A,A"},           // 0x8F

    {0, 0, 0, 0, False, TYPE_0, "SUB B"},             // 0x90
    {0, 0, 0, 0, False, TYPE_0, "SUB C"},             // 0x91
    {0, 0, 0, 0, False, TYPE_0, "SUB D"},             // 0x92
    {0, 0, 0, 0, False, TYPE_0, "SUB E"},             // 0x93
    {0, 0, 0, 0, False, TYPE_0, "SUB H"},             // 0x94
    {0, 0, 0, 0, False, TYPE_0, "SUB L"},             // 0x95
    {0, 0, 1, 0, False, TYPE_0, "SUB (HL)"},          // 0x96
    {0, 0, 0, 0, False, TYPE_0, "SUB A"},             // 0x97
    {0, 0, 0, 0, False, TYPE_0, "SBC A,B"},           // 0x98
    {0, 0, 0, 0, False, TYPE_0, "SBC A,C"},           // 0x99
    {0, 0, 0, 0, False, TYPE_0, "SBC A,D"},           // 0x9A
    {0, 0, 0, 0, False, TYPE_0, "SBC A,E"},           // 0x9B
    {0, 0, 0, 0, False, TYPE_0, "SBC A,H"},           // 0x9C
    {0, 0, 0, 0, False, TYPE_0, "SBC A,L"},           // 0x9D
    {0, 0, 1, 0, False, TYPE_0, "SBC A,(HL)"},        // 0x9E
    {0, 0, 0, 0, False, TYPE_0, "SBC A,A"},           // 0x9F

    {0, 0, 0, 0, False, TYPE_0, "AND B"},             // 0xA0
    {0, 0, 0, 0, False, TYPE_0, "AND C"},             // 0xA1
    {0, 0, 0, 0, False, TYPE_0, "AND D"},             // 0xA2
    {0, 0, 0, 0, False, TYPE_0, "AND E"},             // 0xA3
    {0, 0, 0, 0, False, TYPE_0, "AND H"},             // 0xA4
    {0, 0, 0, 0, False, TYPE_0, "AND L"},             // 0xA5
    {0, 0, 1, 0, False, TYPE_0, "AND (HL)"},          // 0xA6
    {0, 0, 0, 0, False, TYPE_0, "AND A"},             // 0xA7
    {0, 0, 0, 0, False, TYPE_0, "XOR B"},             // 0xA8
    {0, 0, 0, 0, False, TYPE_0, "XOR C"},             // 0xA9
    {0, 0, 0, 0, False, TYPE_0, "XOR D"},             // 0xAA
    {0, 0, 0, 0, False, TYPE_0, "XOR E"},             // 0xAB
    {0, 0, 0, 0, False, TYPE_0, "XOR H"},             // 0xAC
    {0, 0, 0, 0, False, TYPE_0, "XOR L"},             // 0xAD
    {0, 0, 1, 0, False, TYPE_0, "XOR (HL)"},          // 0xAE
    {0, 0, 0, 0, False, TYPE_0, "XOR A"},             // 0xAF

    {0, 0, 0, 0, False, TYPE_0, "OR B"},              // 0xB0
    {0, 0, 0, 0, False, TYPE_0, "OR C"},              // 0xB1
    {0, 0, 0, 0, False, TYPE_0, "OR D"},              // 0xB2
    {0, 0, 0, 0, False, TYPE_0, "OR E"},              // 0xB3
    {0, 0, 0, 0, False, TYPE_0, "OR H"},              // 0xB4
    {0, 0, 0, 0, False, TYPE_0, "OR L"},              // 0xB5
    {0, 0, 1, 0, False, TYPE_0, "OR (HL)"},           // 0xB6
    {0, 0, 0, 0, False, TYPE_0, "OR A"},              // 0xB7
    {0, 0, 0, 0, False, TYPE_0, "CP B"},              // 0xB8
    {0, 0, 0, 0, False, TYPE_0, "CP C"},              // 0xB9
    {0, 0, 0, 0, False, TYPE_0, "CP D"},              // 0xBA
    {0, 0, 0, 0, False, TYPE_0, "CP E"},              // 0xBB
    {0, 0, 0, 0, False, TYPE_0, "CP H"},              // 0xBC
    {0, 0, 0, 0, False, TYPE_0, "CP L"},              // 0xBD
    {0, 0, 1, 0, False, TYPE_0, "CP (HL)"},           // 0xBE
    {0, 0, 0, 0, False, TYPE_0, "CP A"},              // 0xBF

    {0, 0, 2, 0, False, TYPE_0, "RET NZ"},            // 0xC0
    {0, 0, 2, 0, False, TYPE_0, "POP BC"},            // 0xC1
    {0, 2, 0, 0, False, TYPE_8, "JP NZ,%04Xh"},       // 0xC2
    {0, 2, 0, 0, False, TYPE_8, "JP %04Xh"},          // 0xC3
    {0, 2, 0,-2, False, TYPE_8, "CALL NZ,%04Xh"},     // 0xC4
    {0, 0, 0,-2, False, TYPE_0, "PUSH BC"},           // 0xC5
    {0, 1, 0, 0, False, TYPE_8, "ADD A,%02Xh"},       // 0xC6
    {0, 0, 0,-2, False, TYPE_0, "RST 00h"},           // 0xC7
    {0, 0, 2, 0, False, TYPE_0, "RET Z"},             // 0xC8
    {0, 0, 2, 0, False, TYPE_0, "RET"},               // 0xC9
    {0, 2, 0, 0, False, TYPE_8, "JP Z,%04Xh"},        // 0xCA
    UNDEFINED,                                        // 0xCB
    {0, 2, 0,-2, False, TYPE_8, "CALL Z,%04Xh"},      // 0xCC
    {0, 2, 0,-2, False, TYPE_8, "CALL %04Xh"},        // 0xCD
    {0, 1, 0, 0, False, TYPE_8, "ADC A,%02Xh"},       // 0xCE
    {0, 0, 0,-2, False, TYPE_0, "RST 08h"},           // 0xCF

    {0, 0, 2, 0, False, TYPE_0, "RET NC"},            // 0xD0
    {0, 0, 2, 0, False, TYPE_0, "POP DE"},            // 0xD1
    {0, 2, 0, 0, False, TYPE_8, "JP NC,%04Xh"},       // 0xD2
    {0, 1, 0, 1, False, TYPE_8, "OUT (%02Xh),A"},     // 0xD3
    {0, 2, 0,-2, False, TYPE_8, "CALL NC,%04Xh"},     // 0xD4
    {0, 0, 0,-2, False, TYPE_0, "PUSH DE"},           // 0xD5
    {0, 1, 0, 0, False, TYPE_8, "SUB %02Xh"},         // 0xD6
    {0, 0, 0,-2, False, TYPE_0, "RST 10h"},           // 0xD7
    {0, 0, 2, 0, False, TYPE_0, "RET C"},             // 0xD8
    {0, 0, 0, 0, False, TYPE_0, "EXX"},               // 0xD9
    {0, 2, 0, 0, False, TYPE_8, "JP C,%04Xh"},        // 0xDA
    {0, 1, 1, 0, False, TYPE_8, "IN A,(%02Xh)"},      // 0xDB
    {0, 2, 0,-2, False, TYPE_8, "CALL C,%04Xh"},      // 0xDC
    UNDEFINED,                                        // 0xDD
    {0, 1, 0, 0, False, TYPE_8, "SBC A,%02Xh"},       // 0xDE
    {0, 0, 0,-2, False, TYPE_0, "RST 18h"},           // 0xDF

    {0, 0, 2, 0, False, TYPE_0, "RET PO"},            // 0xE0
    {0, 0, 2, 0, False, TYPE_0, "POP HL"},            // 0xE1
    {0, 2, 0, 0, False, TYPE_8, "JP PO,%04Xh"},       // 0xE2
    {0, 0, 2, 2, False, TYPE_0, "EX (SP),HL"},        // 0xE3
    {0, 2, 0,-2, False, TYPE_8, "CALL PO,%04Xh"},     // 0xE4
    {0, 0, 0,-2, False, TYPE_0, "PUSH HL"},           // 0xE5
    {0, 1, 0, 0, False, TYPE_8, "AND %02Xh"},         // 0xE6
    {0, 0, 0,-2, False, TYPE_0, "RST 20h"},           // 0xE7
    {0, 0, 2, 0, False, TYPE_0, "RET PE"},            // 0xE8
    {0, 0, 0, 0, False, TYPE_0, "JP (HL)"},           // 0xE9
    {0, 2, 0, 0, False, TYPE_8, "JP PE,%04Xh"},       // 0xEA
    {0, 0, 0, 0, False, TYPE_0, "EX DE,HL"},          // 0xEB
    {0, 2, 0,-2, False, TYPE_8, "CALL PE,%04Xh"},     // 0xEC
    UNDEFINED,                                        // 0xED
    {0, 1, 0, 0, False, TYPE_8, "XOR %02Xh"},         // 0xEE
    {0, 0, 0,-2, False, TYPE_0, "RST 28h"},           // 0xEF

    {0, 0, 2, 0, False, TYPE_0, "RET P"},             // 0xF0
    {0, 0, 2, 0, False, TYPE_0, "POP AF"},            // 0xF1
    {0, 2, 0, 0, False, TYPE_8, "JP P,%04Xh"},        // 0xF2
    {0, 0, 0, 0, False, TYPE_0, "DI"},                // 0xF3
    {0, 2, 0,-2, False, TYPE_8, "CALL P,%04Xh"},      // 0xF4
    {0, 0, 0,-2, False, TYPE_0, "PUSH AF"},           // 0xF5
    {0, 1, 0, 0, False, TYPE_8, "OR %02Xh"},          // 0xF6
    {0, 0, 0,-2, False, TYPE_0, "RST 30h"},           // 0xF7
    {0, 0, 2, 0, False, TYPE_0, "RET M"},             // 0xF8
    {0, 0, 0, 0, False, TYPE_0, "LD SP,HL"},          // 0xF9
    {0, 2, 0, 0, False, TYPE_8, "JP M,%04Xh"},        // 0xFA
    {0, 0, 0, 0, False, TYPE_0, "EI"},                // 0xFB
    {0, 2, 0,-2, False, TYPE_8, "CALL M,%04Xh"},      // 0xFC
    UNDEFINED,                                        // 0xFD
    {0, 1, 0, 0, False, TYPE_8, "CP %02Xh"},          // 0xFE
    {0, 0, 0,-2, False, TYPE_0, "RST 38h"}            // 0xFF
};

// Instructions with ED prefix
InstrType extended_instructions[256] = {
    UNDEFINED,                                        // 0x00
    UNDEFINED,                                        // 0x01
    UNDEFINED,                                        // 0x02
    UNDEFINED,                                        // 0x03
    UNDEFINED,                                        // 0x04
    UNDEFINED,                                        // 0x05
    UNDEFINED,                                        // 0x06
    UNDEFINED,                                        // 0x07
    UNDEFINED,                                        // 0x08
    UNDEFINED,                                        // 0x09
    UNDEFINED,                                        // 0x0A
    UNDEFINED,                                        // 0x0B
    UNDEFINED,                                        // 0x0C
    UNDEFINED,                                        // 0x0D
    UNDEFINED,                                        // 0x0E
    UNDEFINED,                                        // 0x0F

    UNDEFINED,                                        // 0x10
    UNDEFINED,                                        // 0x11
    UNDEFINED,                                        // 0x12
    UNDEFINED,                                        // 0x13
    UNDEFINED,                                        // 0x14
    UNDEFINED,                                        // 0x15
    UNDEFINED,                                        // 0x16
    UNDEFINED,                                        // 0x17
    UNDEFINED,                                        // 0x18
    UNDEFINED,                                        // 0x19
    UNDEFINED,                                        // 0x1A
    UNDEFINED,                                        // 0x1B
    UNDEFINED,                                        // 0x1C
    UNDEFINED,                                        // 0x1D
    UNDEFINED,                                        // 0x1E
    UNDEFINED,                                        // 0x1F

    UNDEFINED,                                        // 0x20
    UNDEFINED,                                        // 0x21
    UNDEFINED,                                        // 0x22
    UNDEFINED,                                        // 0x23
    UNDEFINED,                                        // 0x24
    UNDEFINED,                                        // 0x25
    UNDEFINED,                                        // 0x26
    UNDEFINED,                                        // 0x27
    UNDEFINED,                                        // 0x28
    UNDEFINED,                                        // 0x29
    UNDEFINED,                                        // 0x2A
    UNDEFINED,                                        // 0x2B
    UNDEFINED,                                        // 0x2C
    UNDEFINED,                                        // 0x2D
    UNDEFINED,                                        // 0x2E
    UNDEFINED,                                        // 0x2F

    UNDEFINED,                                        // 0x30
    UNDEFINED,                                        // 0x31
    UNDEFINED,                                        // 0x32
    UNDEFINED,                                        // 0x33
    UNDEFINED,                                        // 0x34
    UNDEFINED,                                        // 0x35
    UNDEFINED,                                        // 0x36
    UNDEFINED,                                        // 0x37
    UNDEFINED,                                        // 0x38
    UNDEFINED,                                        // 0x39
    UNDEFINED,                                        // 0x3A
    UNDEFINED,                                        // 0x3B
    UNDEFINED,                                        // 0x3C
    UNDEFINED,                                        // 0x3D
    UNDEFINED,                                        // 0x3E
    UNDEFINED,                                        // 0x3F

    {0, 0, 1, 0, False, TYPE_0, "IN B,(C)"},          // 0x40
    {0, 0, 0, 1, False, TYPE_0, "OUT (C),B"},         // 0x41
    {0, 0, 0, 0, False, TYPE_0, "SBC HL,BC"},         // 0x42
    {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),BC"},     // 0x43
    {0, 0, 0, 0, False, TYPE_0, "NEG"},               // 0x44
    {0, 0, 2, 0, False, TYPE_0, "RETN"},              // 0x45
    {0, 0, 0, 0, False, TYPE_0, "IM 0"},              // 0x46
    {0, 0, 0, 0, False, TYPE_0, "LD I,A"},            // 0x47
    {0, 0, 1, 0, False, TYPE_0, "IN C,(C)"},          // 0x48
    {0, 0, 0, 1, False, TYPE_0, "OUT (C),C"},         // 0x49
    {0, 0, 0, 0, False, TYPE_0, "ADC HL,BC"},         // 0x4A
    {0, 2, 2, 0, False, TYPE_8, "LD BC,(%04Xh)"},     // 0x4B
    {0, 0, 0, 0, False, TYPE_0, "NEG"},               // 0x4C
    {0, 0, 2, 0, False, TYPE_0, "RETI"},              // 0x4D
    {0, 0, 0, 0, False, TYPE_0, "IM 0/1"},            // 0x4E
    {0, 0, 0, 0, False, TYPE_0, "LD R,A"},            // 0x4F

    {0, 0, 1, 0, False, TYPE_0, "IN D,(C)"},          // 0x50
    {0, 0, 0, 1, False, TYPE_0, "OUT (C),D"},         // 0x51
    {0, 0, 0, 0, False, TYPE_0, "SBC HL,DE"},         // 0x52
    {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),DE"},     // 0x53
    {0, 0, 0, 0, False, TYPE_0, "NEG"},               // 0x54
    {0, 0, 2, 0, False, TYPE_0, "RETN"},              // 0x55
    {0, 0, 0, 0, False, TYPE_0, "IM 1"},              // 0x56
    {0, 0, 0, 0, False, TYPE_0, "LD A,I"},            // 0x57
    {0, 0, 1, 0, False, TYPE_0, "IN E,(C)"},          // 0x58
    {0, 0, 0, 1, False, TYPE_0, "OUT (C),E"},         // 0x59
    {0, 0, 0, 0, False, TYPE_0, "ADC HL,DE"},         // 0x5A
    {0, 2, 2, 0, False, TYPE_8, "LD DE,(%04Xh)"},     // 0x5B
    {0, 0, 0, 0, False, TYPE_0, "NEG"},               // 0x5C
    {0, 0, 2, 0, False, TYPE_0, "RETN"},              // 0x5D
    {0, 0, 0, 0, False, TYPE_0, "IM 2"},              // 0x5E
    {0, 0, 0, 0, False, TYPE_0, "LD A,R"},            // 0x5F

    {0, 0, 1, 0, False, TYPE_0, "IN H,(C)"},          // 0x60
    {0, 0, 0, 1, False, TYPE_0, "OUT (C),H"},         // 0x61
    {0, 0, 0, 0, False, TYPE_0, "SBC HL,HL"},         // 0x62
    {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),HL"},     // 0x63
    {0, 0, 0, 0, False, TYPE_0, "NEG"},               // 0x64
    {0, 0, 2, 0, False, TYPE_0, "RETN"},              // 0x65
    {0, 0, 0, 0, False, TYPE_0, "IM 0"},              // 0x66
    {0, 0, 1, 1, False, TYPE_0, "RRD"},               // 0x67
    {0, 0, 1, 0, False, TYPE_0, "IN L,(C)"},          // 0x68
    {0, 0, 0, 1, False, TYPE_0, "OUT (C),L"},         // 0x69
    {0, 0, 0, 0, False, TYPE_0, "ADC HL,HL"},         // 0x6A
    {0, 2, 2, 0, False, TYPE_8, "LD HL,(%04Xh)"},     // 0x6B
    {0, 0, 0, 0, False, TYPE_0, "NEG"},               // 0x6C
    {0, 0, 2, 0, False, TYPE_0, "RETN"},              // 0x6D
    {0, 0, 0, 0, False, TYPE_0, "IM 0/1"},            // 0x6E
    {0, 0, 1, 1, False, TYPE_0, "RLD"},               // 0x6F

    {0, 0, 1, 0, False, TYPE_0, "IN (C)"},            // 0x70
    {0, 0, 0, 1, False, TYPE_0, "OUT (C),0"},         // 0x71
    {0, 0, 0, 0, False, TYPE_0, "SBC HL,SP"},         // 0x72
    {0, 2, 0, 2, False, TYPE_8, "LD (%04Xh),SP"},     // 0x73
    {0, 0, 0, 0, False, TYPE_0, "NEG"},               // 0x74
    {0, 0, 2, 0, False, TYPE_0, "RETN"},              // 0x75
    {0, 0, 0, 0, False, TYPE_0, "IM 1"},              // 0x76
    UNDEFINED,                                        // 0x77
    {0, 0, 1, 0, False, TYPE_0, "IN A,(C)"},          // 0x78
    {0, 0, 0, 1, False, TYPE_0, "OUT (C),A"},         // 0x79
    {0, 0, 0, 0, False, TYPE_0, "ADC HL,SP"},         // 0x7A
    {0, 2, 2, 0, False, TYPE_8, "LD SP,(%04Xh)"},     // 0x7B
    {0, 0, 0, 0, False, TYPE_0, "NEG"},               // 0x7C
    {0, 0, 2, 0, False, TYPE_0, "RETN"},              // 0x7D
    {0, 0, 0, 0, False, TYPE_0, "IM 2"},              // 0x7E
    UNDEFINED,                                        // 0x7F

    UNDEFINED,                                        // 0x80
    UNDEFINED,                                        // 0x81
    UNDEFINED,                                        // 0x82
    UNDEFINED,                                        // 0x83
    UNDEFINED,                                        // 0x84
    UNDEFINED,                                        // 0x85
    UNDEFINED,                                        // 0x86
    UNDEFINED,                                        // 0x87
    UNDEFINED,                                        // 0x88
    UNDEFINED,                                        // 0x89
    UNDEFINED,                                        // 0x8A
    UNDEFINED,                                        // 0x8B
    UNDEFINED,                                        // 0x8C
    UNDEFINED,                                        // 0x8D
    UNDEFINED,                                        // 0x8E
    UNDEFINED,                                        // 0x8F

    UNDEFINED,                                        // 0x90
    UNDEFINED,                                        // 0x91
    UNDEFINED,                                        // 0x92
    UNDEFINED,                                        // 0x93
    UNDEFINED,                                        // 0x94
    UNDEFINED,                                        // 0x95
    UNDEFINED,                                        // 0x96
    UNDEFINED,                                        // 0x97
    UNDEFINED,                                        // 0x98
    UNDEFINED,                                        // 0x99
    UNDEFINED,                                        // 0x9A
    UNDEFINED,                                        // 0x9B
    UNDEFINED,                                        // 0x9C
    UNDEFINED,                                        // 0x9D
    UNDEFINED,                                        // 0x9E
    UNDEFINED,                                        // 0x9F

    {0, 0, 1, 1, False, TYPE_0, "LDI"},               // 0xA0
    {0, 0, 1, 0, False, TYPE_0, "CPI"},               // 0xA1
    {0, 0, 1, 1, False, TYPE_0, "INI"},               // 0xA2
    {0, 0, 1, 1, False, TYPE_0, "OUTI"},              // 0xA3
    UNDEFINED,                                        // 0xA4
    UNDEFINED,                                        // 0xA5
    UNDEFINED,                                        // 0xA6
    UNDEFINED,                                        // 0xA7
    {0, 0, 1, 1, False, TYPE_0, "LDD"},               // 0xA8
    {0, 0, 1, 0, False, TYPE_0, "CPD"},               // 0xA9
    {0, 0, 1, 1, False, TYPE_0, "IND"},               // 0xAA
    {0, 0, 1, 1, False, TYPE_0, "OUTD"},              // 0xAB
    UNDEFINED,                                        // 0xAC
    UNDEFINED,                                        // 0xAD
    UNDEFINED,                                        // 0xAE
    UNDEFINED,                                        // 0xAF

    {0, 0, 1, 1, True,  TYPE_0, "LDIR"},              // 0xB0
    {0, 0, 1, 0, True,  TYPE_0, "CPIR"},              // 0xB1
    {0, 0, 1, 1, True,  TYPE_0, "INIR"},              // 0xB2
    {0, 0, 1, 1, True,  TYPE_0, "OTIR"},              // 0xB3
    UNDEFINED,                                        // 0xB4
    UNDEFINED,                                        // 0xB5
    UNDEFINED,                                        // 0xB6
    UNDEFINED,                                        // 0xB7
    {0, 0, 1, 1, True,  TYPE_0, "LDDR"},              // 0xB8
    {0, 0, 1, 0, True,  TYPE_0, "CPDR"},              // 0xB9
    {0, 0, 1, 1, True,  TYPE_0, "INDR"},              // 0xBA
    {0, 0, 1, 1, True,  TYPE_0, "OTDR"},              // 0xBB
    UNDEFINED,                                        // 0xBC
    UNDEFINED,                                        // 0xBD
    UNDEFINED,                                        // 0xBE
    UNDEFINED,                                        // 0xBF

    UNDEFINED,                                        // 0xC0
    UNDEFINED,                                        // 0xC1
    UNDEFINED,                                        // 0xC2
    UNDEFINED,                                        // 0xC3
    UNDEFINED,                                        // 0xC4
    UNDEFINED,                                        // 0xC5
    UNDEFINED,                                        // 0xC6
    UNDEFINED,                                        // 0xC7
    UNDEFINED,                                        // 0xC8
    UNDEFINED,                                        // 0xC9
    UNDEFINED,                                        // 0xCA
    UNDEFINED,                                        // 0xCB
    UNDEFINED,                                        // 0xCC
    UNDEFINED,                                        // 0xCD
    UNDEFINED,                                        // 0xCE
    UNDEFINED,                                        // 0xCF

    UNDEFINED,                                        // 0xD0
    UNDEFINED,                                        // 0xD1
    UNDEFINED,                                        // 0xD2
    UNDEFINED,                                        // 0xD3
    UNDEFINED,                                        // 0xD4
    UNDEFINED,                                        // 0xD5
    UNDEFINED,                                        // 0xD6
    UNDEFINED,                                        // 0xD7
    UNDEFINED,                                        // 0xD8
    UNDEFINED,                                        // 0xD9
    UNDEFINED,                                        // 0xDA
    UNDEFINED,                                        // 0xDB
    UNDEFINED,                                        // 0xDC
    UNDEFINED,                                        // 0xDD
    UNDEFINED,                                        // 0xDE
    UNDEFINED,                                        // 0xDF

    UNDEFINED,                                        // 0xE0
    UNDEFINED,                                        // 0xE1
    UNDEFINED,                                        // 0xE2
    UNDEFINED,                                        // 0xE3
    UNDEFINED,                                        // 0xE4
    UNDEFINED,                                        // 0xE5
    UNDEFINED,                                        // 0xE6
    UNDEFINED,                                        // 0xE7
    UNDEFINED,                                        // 0xE8
    UNDEFINED,                                        // 0xE9
    UNDEFINED,                                        // 0xEA
    UNDEFINED,                                        // 0xEB
    UNDEFINED,                                        // 0xEC
    UNDEFINED,                                        // 0xED
    UNDEFINED,                                        // 0xEE
    UNDEFINED,                                        // 0xEF

    UNDEFINED,                                        // 0xF0
    UNDEFINED,                                        // 0xF1
    UNDEFINED,                                        // 0xF2
    UNDEFINED,                                        // 0xF3
    UNDEFINED,                                        // 0xF4
    UNDEFINED,                                        // 0xF5
    UNDEFINED,                                        // 0xF6
    UNDEFINED,                                        // 0xF7
    UNDEFINED,                                        // 0xF8
    UNDEFINED,                                        // 0xF9
    UNDEFINED,                                        // 0xFA
    UNDEFINED,                                        // 0xFB
    UNDEFINED,                                        // 0xFC
    UNDEFINED,                                        // 0xFD
    UNDEFINED,                                        // 0xFE
    UNDEFINED                                         // 0xFF
};

// Instructions with CB prefix
InstrType bit_instructions[256] = {
    {0, 0, 0, 0, False, TYPE_0, "RLC B"},             // 0x00
    {0, 0, 0, 0, False, TYPE_0, "RLC C"},             // 0x01
    {0, 0, 0, 0, False, TYPE_0, "RLC D"},             // 0x02
    {0, 0, 0, 0, False, TYPE_0, "RLC E"},             // 0x03
    {0, 0, 0, 0, False, TYPE_0, "RLC H"},             // 0x04
    {0, 0, 0, 0, False, TYPE_0, "RLC L"},             // 0x05
    {0, 0, 1, 1, False, TYPE_0, "RLC (HL)"},          // 0x06
    {0, 0, 0, 0, False, TYPE_0, "RLC A"},             // 0x07
    {0, 0, 0, 0, False, TYPE_0, "RRC B"},             // 0x08
    {0, 0, 0, 0, False, TYPE_0, "RRC C"},             // 0x09
    {0, 0, 0, 0, False, TYPE_0, "RRC D"},             // 0x0A
    {0, 0, 0, 0, False, TYPE_0, "RRC E"},             // 0x0B
    {0, 0, 0, 0, False, TYPE_0, "RRC H"},             // 0x0C
    {0, 0, 0, 0, False, TYPE_0, "RRC L"},             // 0x0D
    {0, 0, 1, 1, False, TYPE_0, "RRC (HL)"},          // 0x0E
    {0, 0, 0, 0, False, TYPE_0, "RRC A"},             // 0x0F

    {0, 0, 0, 0, False, TYPE_0, "RL B"},              // 0x10
    {0, 0, 0, 0, False, TYPE_0, "RL C"},              // 0x11
    {0, 0, 0, 0, False, TYPE_0, "RL D"},              // 0x12
    {0, 0, 0, 0, False, TYPE_0, "RL E"},              // 0x13
    {0, 0, 0, 0, False, TYPE_0, "RL H"},              // 0x14
    {0, 0, 0, 0, False, TYPE_0, "RL L"},              // 0x15
    {0, 0, 1, 1, False, TYPE_0, "RL (HL)"},           // 0x16
    {0, 0, 0, 0, False, TYPE_0, "RL A"},              // 0x17
    {0, 0, 0, 0, False, TYPE_0, "RR B"},              // 0x18
    {0, 0, 0, 0, False, TYPE_0, "RR C"},              // 0x19
    {0, 0, 0, 0, False, TYPE_0, "RR D"},              // 0x1A
    {0, 0, 0, 0, False, TYPE_0, "RR E"},              // 0x1B
    {0, 0, 0, 0, False, TYPE_0, "RR H"},              // 0x1C
    {0, 0, 0, 0, False, TYPE_0, "RR L"},              // 0x1D
    {0, 0, 1, 1, False, TYPE_0, "RR (HL)"},           // 0x1E
    {0, 0, 0, 0, False, TYPE_0, "RR A"},              // 0x1F

    {0, 0, 0, 0, False, TYPE_0, "SLA B"},             // 0x20
    {0, 0, 0, 0, False, TYPE_0, "SLA C"},             // 0x21
    {0, 0, 0, 0, False, TYPE_0, "SLA D"},             // 0x22
    {0, 0, 0, 0, False, TYPE_0, "SLA E"},             // 0x23
    {0, 0, 0, 0, False, TYPE_0, "SLA H"},             // 0x24
    {0, 0, 0, 0, False, TYPE_0, "SLA L"},             // 0x25
    {0, 0, 1, 1, False, TYPE_0, "SLA (HL)"},          // 0x26
    {0, 0, 0, 0, False, TYPE_0, "SLA A"},             // 0x27
    {0, 0, 0, 0, False, TYPE_0, "SRA B"},             // 0x28
    {0, 0, 0, 0, False, TYPE_0, "SRA C"},             // 0x29
    {0, 0, 0, 0, False, TYPE_0, "SRA D"},             // 0x2A
    {0, 0, 0, 0, False, TYPE_0, "SRA E"},             // 0x2B
    {0, 0, 0, 0, False, TYPE_0, "SRA H"},             // 0x2C
    {0, 0, 0, 0, False, TYPE_0, "SRA L"},             // 0x2D
    {0, 0, 1, 1, False, TYPE_0, "SRA (HL)"},          // 0x2E
    {0, 0, 0, 0, False, TYPE_0, "SRA A"},             // 0x2F

    {0, 0, 0, 0, False, TYPE_0, "SLL B"},             // 0x30
    {0, 0, 0, 0, False, TYPE_0, "SLL C"},             // 0x31
    {0, 0, 0, 0, False, TYPE_0, "SLL D"},             // 0x32
    {0, 0, 0, 0, False, TYPE_0, "SLL E"},             // 0x33
    {0, 0, 0, 0, False, TYPE_0, "SLL H"},             // 0x34
    {0, 0, 0, 0, False, TYPE_0, "SLL L"},             // 0x35
    {0, 0, 1, 1, False, TYPE_0, "SLL (HL)"},          // 0x36
    {0, 0, 0, 0, False, TYPE_0, "SLL A"},             // 0x37
    {0, 0, 0, 0, False, TYPE_0, "SRL B"},             // 0x38
    {0, 0, 0, 0, False, TYPE_0, "SRL C"},             // 0x39
    {0, 0, 0, 0, False, TYPE_0, "SRL D"},             // 0x3A
    {0, 0, 0, 0, False, TYPE_0, "SRL E"},             // 0x3B
    {0, 0, 0, 0, False, TYPE_0, "SRL H"},             // 0x3C
    {0, 0, 0, 0, False, TYPE_0, "SRL L"},             // 0x3D
    {0, 0, 1, 1, False, TYPE_0, "SRL (HL)"},          // 0x3E
    {0, 0, 0, 0, False, TYPE_0, "SRL A"},             // 0x3F

    {0, 0, 0, 0, False, TYPE_0, "BIT 0,B"},           // 0x40
    {0, 0, 0, 0, False, TYPE_0, "BIT 0,C"},           // 0x41
    {0, 0, 0, 0, False, TYPE_0, "BIT 0,D"},           // 0x42
    {0, 0, 0, 0, False, TYPE_0, "BIT 0,E"},           // 0x43
    {0, 0, 0, 0, False, TYPE_0, "BIT 0,H"},           // 0x44
    {0, 0, 0, 0, False, TYPE_0, "BIT 0,L"},           // 0x45
    {0, 0, 1, 0, False, TYPE_0, "BIT 0,(HL)"},        // 0x46
    {0, 0, 0, 0, False, TYPE_0, "BIT 0,A"},           // 0x47
    {0, 0, 0, 0, False, TYPE_0, "BIT 1,B"},           // 0x48
    {0, 0, 0, 0, False, TYPE_0, "BIT 1,C"},           // 0x49
    {0, 0, 0, 0, False, TYPE_0, "BIT 1,D"},           // 0x4A
    {0, 0, 0, 0, False, TYPE_0, "BIT 1,E"},           // 0x4B
    {0, 0, 0, 0, False, TYPE_0, "BIT 1,H"},           // 0x4C
    {0, 0, 0, 0, False, TYPE_0, "BIT 1,L"},           // 0x4D
    {0, 0, 1, 0, False, TYPE_0, "BIT 1,(HL)"},        // 0x4E
    {0, 0, 0, 0, False, TYPE_0, "BIT 1,A"},           // 0x4F

    {0, 0, 0, 0, False, TYPE_0, "BIT 2,B"},           // 0x50
    {0, 0, 0, 0, False, TYPE_0, "BIT 2,C"},           // 0x51
    {0, 0, 0, 0, False, TYPE_0, "BIT 2,D"},           // 0x52
    {0, 0, 0, 0, False, TYPE_0, "BIT 2,E"},           // 0x53
    {0, 0, 0, 0, False, TYPE_0, "BIT 2,H"},           // 0x54
    {0, 0, 0, 0, False, TYPE_0, "BIT 2,L"},           // 0x55
    {0, 0, 1, 0, False, TYPE_0, "BIT 2,(HL)"},        // 0x56
    {0, 0, 0, 0, False, TYPE_0, "BIT 2,A"},           // 0x57
    {0, 0, 0, 0, False, TYPE_0, "BIT 3,B"},           // 0x58
    {0, 0, 0, 0, False, TYPE_0, "BIT 3,C"},           // 0x59
    {0, 0, 0, 0, False, TYPE_0, "BIT 3,D"},           // 0x5A
    {0, 0, 0, 0, False, TYPE_0, "BIT 3,E"},           // 0x5B
    {0, 0, 0, 0, False, TYPE_0, "BIT 3,H"},           // 0x5C
    {0, 0, 0, 0, False, TYPE_0, "BIT 3,L"},           // 0x5D
    {0, 0, 1, 0, False, TYPE_0, "BIT 3,(HL)"},        // 0x5E
    {0, 0, 0, 0, False, TYPE_0, "BIT 3,A"},           // 0x5F

    {0, 0, 0, 0, False, TYPE_0, "BIT 4,B"},           // 0x60
    {0, 0, 0, 0, False, TYPE_0, "BIT 4,C"},           // 0x61
    {0, 0, 0, 0, False, TYPE_0, "BIT 4,D"},           // 0x62
    {0, 0, 0, 0, False, TYPE_0, "BIT 4,E"},           // 0x63
    {0, 0, 0, 0, False, TYPE_0, "BIT 4,H"},           // 0x64
    {0, 0, 0, 0, False, TYPE_0, "BIT 4,L"},           // 0x65
    {0, 0, 1, 0, False, TYPE_0, "BIT 4,(HL)"},        // 0x66
    {0, 0, 0, 0, False, TYPE_0, "BIT 4,A"},           // 0x67
    {0, 0, 0, 0, False, TYPE_0, "BIT 5,B"},           // 0x68
    {0, 0, 0, 0, False, TYPE_0, "BIT 5,C"},           // 0x69
    {0, 0, 0, 0, False, TYPE_0, "BIT 5,D"},           // 0x6A
    {0, 0, 0, 0, False, TYPE_0, "BIT 5,E"},           // 0x6B
    {0, 0, 0, 0, False, TYPE_0, "BIT 5,H"},           // 0x6C
    {0, 0, 0, 0, False, TYPE_0, "BIT 5,L"},           // 0x6D
    {0, 0, 1, 0, False, TYPE_0, "BIT 5,(HL)"},        // 0x6E
    {0, 0, 0, 0, False, TYPE_0, "BIT 5,A"},           // 0x6F

    {0, 0, 0, 0, False, TYPE_0, "BIT 6,B"},           // 0x70
    {0, 0, 0, 0, False, TYPE_0, "BIT 6,C"},           // 0x71
    {0, 0, 0, 0, False, TYPE_0, "BIT 6,D"},           // 0x72
    {0, 0, 0, 0, False, TYPE_0, "BIT 6,E"},           // 0x73
    {0, 0, 0, 0, False, TYPE_0, "BIT 6,H"},           // 0x74
    {0, 0, 0, 0, False, TYPE_0, "BIT 6,L"},           // 0x75
    {0, 0, 1, 0, False, TYPE_0, "BIT 6,(HL)"},        // 0x76
    {0, 0, 0, 0, False, TYPE_0, "BIT 6,A"},           // 0x77
    {0, 0, 0, 0, False, TYPE_0, "BIT 7,B"},           // 0x78
    {0, 0, 0, 0, False, TYPE_0, "BIT 7,C"},           // 0x79
    {0, 0, 0, 0, False, TYPE_0, "BIT 7,D"},           // 0x7A
    {0, 0, 0, 0, False, TYPE_0, "BIT 7,E"},           // 0x7B
    {0, 0, 0, 0, False, TYPE_0, "BIT 7,H"},           // 0x7C
    {0, 0, 0, 0, False, TYPE_0, "BIT 7,L"},           // 0x7D
    {0, 0, 1, 0, False, TYPE_0, "BIT 7,(HL)"},        // 0x7E
    {0, 0, 0, 0, False, TYPE_0, "BIT 7,A"},           // 0x7F

    {0, 0, 0, 0, False, TYPE_0, "RES 0,B"},           // 0x80
    {0, 0, 0, 0, False, TYPE_0, "RES 0,C"},           // 0x81
    {0, 0, 0, 0, False, TYPE_0, "RES 0,D"},           // 0x82
    {0, 0, 0, 0, False, TYPE_0, "RES 0,E"},           // 0x83
    {0, 0, 0, 0, False, TYPE_0, "RES 0,H"},           // 0x84
    {0, 0, 0, 0, False, TYPE_0, "RES 0,L"},           // 0x85
    {0, 0, 1, 1, False, TYPE_0, "RES 0,(HL)"},        // 0x86
    {0, 0, 0, 0, False, TYPE_0, "RES 0,A"},           // 0x87
    {0, 0, 0, 0, False, TYPE_0, "RES 1,B"},           // 0x88
    {0, 0, 0, 0, False, TYPE_0, "RES 1,C"},           // 0x89
    {0, 0, 0, 0, False, TYPE_0, "RES 1,D"},           // 0x8A
    {0, 0, 0, 0, False, TYPE_0, "RES 1,E"},           // 0x8B
    {0, 0, 0, 0, False, TYPE_0, "RES 1,H"},           // 0x8C
    {0, 0, 0, 0, False, TYPE_0, "RES 1,L"},           // 0x8D
    {0, 0, 1, 1, False, TYPE_0, "RES 1,(HL)"},        // 0x8E
    {0, 0, 0, 0, False, TYPE_0, "RES 1,A"},           // 0x8F

    {0, 0, 0, 0, False, TYPE_0, "RES 2,B"},           // 0x90
    {0, 0, 0, 0, False, TYPE_0, "RES 2,C"},           // 0x91
    {0, 0, 0, 0, False, TYPE_0, "RES 2,D"},           // 0x92
    {0, 0, 0, 0, False, TYPE_0, "RES 2,E"},           // 0x93
    {0, 0, 0, 0, False, TYPE_0, "RES 2,H"},           // 0x94
    {0, 0, 0, 0, False, TYPE_0, "RES 2,L"},           // 0x95
    {0, 0, 1, 1, False, TYPE_0, "RES 2,(HL)"},        // 0x96
    {0, 0, 0, 0, False, TYPE_0, "RES 2,A"},           // 0x97
    {0, 0, 0, 0, False, TYPE_0, "RES 3,B"},           // 0x98
    {0, 0, 0, 0, False, TYPE_0, "RES 3,C"},           // 0x99
    {0, 0, 0, 0, False, TYPE_0, "RES 3,D"},           // 0x9A
    {0, 0, 0, 0, False, TYPE_0, "RES 3,E"},           // 0x9B
    {0, 0, 0, 0, False, TYPE_0, "RES 3,H"},           // 0x9C
    {0, 0, 0, 0, False, TYPE_0, "RES 3,L"},           // 0x9D
    {0, 0, 1, 1, False, TYPE_0, "RES 3,(HL)"},        // 0x9E
    {0, 0, 0, 0, False, TYPE_0, "RES 3,A"},           // 0x9F

    {0, 0, 0, 0, False, TYPE_0, "RES 4,B"},           // 0xA0
    {0, 0, 0, 0, False, TYPE_0, "RES 4,C"},           // 0xA1
    {0, 0, 0, 0, False, TYPE_0, "RES 4,D"},           // 0xA2
    {0, 0, 0, 0, False, TYPE_0, "RES 4,E"},           // 0xA3
    {0, 0, 0, 0, False, TYPE_0, "RES 4,H"},           // 0xA4
    {0, 0, 0, 0, False, TYPE_0, "RES 4,L"},           // 0xA5
    {0, 0, 1, 1, False, TYPE_0, "RES 4,(HL)"},        // 0xA6
    {0, 0, 0, 0, False, TYPE_0, "RES 4,A"},           // 0xA7
    {0, 0, 0, 0, False, TYPE_0, "RES 5,B"},           // 0xA8
    {0, 0, 0, 0, False, TYPE_0, "RES 5,C"},           // 0xA9
    {0, 0, 0, 0, False, TYPE_0, "RES 5,D"},           // 0xAA
    {0, 0, 0, 0, False, TYPE_0, "RES 5,E"},           // 0xAB
    {0, 0, 0, 0, False, TYPE_0, "RES 5,H"},           // 0xAC
    {0, 0, 0, 0, False, TYPE_0, "RES 5,L"},           // 0xAD
    {0, 0, 1, 1, False, TYPE_0, "RES 5,(HL)"},        // 0xAE
    {0, 0, 0, 0, False, TYPE_0, "RES 5,A"},           // 0xAF

    {0, 0, 0, 0, False, TYPE_0, "RES 6,B"},           // 0xB0
    {0, 0, 0, 0, False, TYPE_0, "RES 6,C"},           // 0xB1
    {0, 0, 0, 0, False, TYPE_0, "RES 6,D"},           // 0xB2
    {0, 0, 0, 0, False, TYPE_0, "RES 6,E"},           // 0xB3
    {0, 0, 0, 0, False, TYPE_0, "RES 6,H"},           // 0xB4
    {0, 0, 0, 0, False, TYPE_0, "RES 6,L"},           // 0xB5
    {0, 0, 1, 1, False, TYPE_0, "RES 6,(HL)"},        // 0xB6
    {0, 0, 0, 0, False, TYPE_0, "RES 6,A"},           // 0xB7
    {0, 0, 0, 0, False, TYPE_0, "RES 7,B"},           // 0xB8
    {0, 0, 0, 0, False, TYPE_0, "RES 7,C"},           // 0xB9
    {0, 0, 0, 0, False, TYPE_0, "RES 7,D"},           // 0xBA
    {0, 0, 0, 0, False, TYPE_0, "RES 7,E"},           // 0xBB
    {0, 0, 0, 0, False, TYPE_0, "RES 7,H"},           // 0xBC
    {0, 0, 0, 0, False, TYPE_0, "RES 7,L"},           // 0xBD
    {0, 0, 1, 1, False, TYPE_0, "RES 7,(HL)"},        // 0xBE
    {0, 0, 0, 0, False, TYPE_0, "RES 7,A"},           // 0xBF

    {0, 0, 0, 0, False, TYPE_0, "SET 0,B"},           // 0xC0
    {0, 0, 0, 0, False, TYPE_0, "SET 0,C"},           // 0xC1
    {0, 0, 0, 0, False, TYPE_0, "SET 0,D"},           // 0xC2
    {0, 0, 0, 0, False, TYPE_0, "SET 0,E"},           // 0xC3
    {0, 0, 0, 0, False, TYPE_0, "SET 0,H"},           // 0xC4
    {0, 0, 0, 0, False, TYPE_0, "SET 0,L"},           // 0xC5
    {0, 0, 1, 1, False, TYPE_0, "SET 0,(HL)"},        // 0xC6
    {0, 0, 0, 0, False, TYPE_0, "SET 0,A"},           // 0xC7
    {0, 0, 0, 0, False, TYPE_0, "SET 1,B"},           // 0xC8
    {0, 0, 0, 0, False, TYPE_0, "SET 1,C"},           // 0xC9
    {0, 0, 0, 0, False, TYPE_0, "SET 1,D"},           // 0xCA
    {0, 0, 0, 0, False, TYPE_0, "SET 1,E"},           // 0xCB
    {0, 0, 0, 0, False, TYPE_0, "SET 1,H"},           // 0xCC
    {0, 0, 0, 0, False, TYPE_0, "SET 1,L"},           // 0xCD
    {0, 0, 1, 1, False, TYPE_0, "SET 1,(HL)"},        // 0xCE
    {0, 0, 0, 0, False, TYPE_0, "SET 1,A"},           // 0xCF

    {0, 0, 0, 0, False, TYPE_0, "SET 2,B"},           // 0xD0
    {0, 0, 0, 0, False, TYPE_0, "SET 2,C"},           // 0xD1
    {0, 0, 0, 0, False, TYPE_0, "SET 2,D"},           // 0xD2
    {0, 0, 0, 0, False, TYPE_0, "SET 2,E"},           // 0xD3
    {0, 0, 0, 0, False, TYPE_0, "SET 2,H"},           // 0xD4
    {0, 0, 0, 0, False, TYPE_0, "SET 2,L"},           // 0xD5
    {0, 0, 1, 1, False, TYPE_0, "SET 2,(HL)"},        // 0xD6
    {0, 0, 0, 0, False, TYPE_0, "SET 2,A"},           // 0xD7
    {0, 0, 0, 0, False, TYPE_0, "SET 3,B"},           // 0xD8
    {0, 0, 0, 0, False, TYPE_0, "SET 3,C"},           // 0xD9
    {0, 0, 0, 0, False, TYPE_0, "SET 3,D"},           // 0xDA
    {0, 0, 0, 0, False, TYPE_0, "SET 3,E"},           // 0xDB
    {0, 0, 0, 0, False, TYPE_0, "SET 3,H"},           // 0xDC
    {0, 0, 0, 0, False, TYPE_0, "SET 3,L"},           // 0xDD
    {0, 0, 1, 1, False, TYPE_0, "SET 3,(HL)"},        // 0xDE
    {0, 0, 0, 0, False, TYPE_0, "SET 3,A"},           // 0xDF

    {0, 0, 0, 0, False, TYPE_0, "SET 4,B"},           // 0xE0
    {0, 0, 0, 0, False, TYPE_0, "SET 4,C"},           // 0xE1
    {0, 0, 0, 0, False, TYPE_0, "SET 4,D"},           // 0xE2
    {0, 0, 0, 0, False, TYPE_0, "SET 4,E"},           // 0xE3
    {0, 0, 0, 0, False, TYPE_0, "SET 4,H"},           // 0xE4
    {0, 0, 0, 0, False, TYPE_0, "SET 4,L"},           // 0xE5
    {0, 0, 1, 1, False, TYPE_0, "SET 4,(HL)"},        // 0xE6
    {0, 0, 0, 0, False, TYPE_0, "SET 4,A"},           // 0xE7
    {0, 0, 0, 0, False, TYPE_0, "SET 5,B"},           // 0xE8
    {0, 0, 0, 0, False, TYPE_0, "SET 5,C"},           // 0xE9
    {0, 0, 0, 0, False, TYPE_0, "SET 5,D"},           // 0xEA
    {0, 0, 0, 0, False, TYPE_0, "SET 5,E"},           // 0xEB
    {0, 0, 0, 0, False, TYPE_0, "SET 5,H"},           // 0xEC
    {0, 0, 0, 0, False, TYPE_0, "SET 5,L"},           // 0xED
    {0, 0, 1, 1, False, TYPE_0, "SET 5,(HL)"},        // 0xEE
    {0, 0, 0, 0, False, TYPE_0, "SET 5,A"},           // 0xEF

    {0, 0, 0, 0, False, TYPE_0, "SET 6,B"},           // 0xF0
    {0, 0, 0, 0, False, TYPE_0, "SET 6,C"},           // 0xF1
    {0, 0, 0, 0, False, TYPE_0, "SET 6,D"},           // 0xF2
    {0, 0, 0, 0, False, TYPE_0, "SET 6,E"},           // 0xF3
    {0, 0, 0, 0, False, TYPE_0, "SET 6,H"},           // 0xF4
    {0, 0, 0, 0, False, TYPE_0, "SET 6,L"},           // 0xF5
    {0, 0, 1, 1, False, TYPE_0, "SET 6,(HL)"},        // 0xF6
    {0, 0, 0, 0, False, TYPE_0, "SET 6,A"},           // 0xF7
    {0, 0, 0, 0, False, TYPE_0, "SET 7,B"},           // 0xF8
    {0, 0, 0, 0, False, TYPE_0, "SET 7,C"},           // 0xF9
    {0, 0, 0, 0, False, TYPE_0, "SET 7,D"},           // 0xFA
    {0, 0, 0, 0, False, TYPE_0, "SET 7,E"},           // 0xFB
    {0, 0, 0, 0, False, TYPE_0, "SET 7,H"},           // 0xFC
    {0, 0, 0, 0, False, TYPE_0, "SET 7,L"},           // 0xFD
    {0, 0, 1, 1, False, TYPE_0, "SET 7,(HL)"},        // 0xFE
    {0, 0, 0, 0, False, TYPE_0, "SET 7,A"}            // 0xFF
};

// Instructions with DD or FD prefix
InstrType index_instructions[256] = {
    UNDEFINED,                                        // 0x00
    UNDEFINED,                                        // 0x01
    UNDEFINED,                                        // 0x02
    UNDEFINED,                                        // 0x03
    UNDEFINED,                                        // 0x04
    UNDEFINED,                                        // 0x05
    UNDEFINED,                                        // 0x06
    UNDEFINED,                                        // 0x07
    UNDEFINED,                                        // 0x08
    {0, 0, 0, 0, False, TYPE_1, "ADD %s,BC"},         // 0x09
    UNDEFINED,                                        // 0x0A
    UNDEFINED,                                        // 0x0B
    UNDEFINED,                                        // 0x0C
    UNDEFINED,                                        // 0x0D
    UNDEFINED,                                        // 0x0E
    UNDEFINED,                                        // 0x0F

    UNDEFINED,                                        // 0x10
    UNDEFINED,                                        // 0x11
    UNDEFINED,                                        // 0x12
    UNDEFINED,                                        // 0x13
    UNDEFINED,                                        // 0x14
    UNDEFINED,                                        // 0x15
    UNDEFINED,                                        // 0x16
    UNDEFINED,                                        // 0x17
    UNDEFINED,                                        // 0x18
    {0, 0, 0, 0, False, TYPE_1, "ADD %s,DE"},         // 0x19
    UNDEFINED,                                        // 0x1A
    UNDEFINED,                                        // 0x1B
    UNDEFINED,                                        // 0x1C
    UNDEFINED,                                        // 0x1D
    UNDEFINED,                                        // 0x1E
    UNDEFINED,                                        // 0x1F

    UNDEFINED,                                        // 0x20
    {0, 2, 0, 0, False, TYPE_4, "LD %s,%04Xh"},       // 0x21
    {0, 2, 0, 2, False, TYPE_3, "LD (%04Xh),%s"},     // 0x22
    {0, 0, 0, 0, False, TYPE_1, "INC %s"},            // 0x23
    {0, 0, 0, 0, False, TYPE_1, "INC %sh"},           // 0x24
    {0, 0, 0, 0, False, TYPE_1, "DEC %sh"},           // 0x25
    {0, 1, 0, 0, False, TYPE_4, "LD %sh,%02Xh"},      // 0x26
    UNDEFINED,                                        // 0x27
    UNDEFINED,                                        // 0x28
    {0, 0, 0, 0, False, TYPE_2, "ADD %s,%s"},         // 0x29
    {0, 2, 2, 0, False, TYPE_4, "LD %s,(%04Xh)"},     // 0x2A
    {0, 0, 0, 0, False, TYPE_1, "DEC %s"},            // 0x2B
    {0, 0, 0, 0, False, TYPE_1, "INC %sl"},           // 0x2C
    {0, 0, 0, 0, False, TYPE_1, "DEC %sl"},           // 0x2D
    {0, 1, 0, 0, False, TYPE_4, "LD %sl,%02Xh"},      // 0x2E
    UNDEFINED,                                        // 0x2F

    UNDEFINED,                                        // 0x30
    UNDEFINED,                                        // 0x31
    UNDEFINED,                                        // 0x32
    UNDEFINED,                                        // 0x33
    {1, 0, 1, 1, False, TYPE_5, "INC (%s%+d)"},       // 0x34
    {1, 0, 1, 1, False, TYPE_5, "DEC (%s%+d)"},       // 0x35
    {1, 1, 0, 1, False, TYPE_6, "LD (%s%+d),%02xh"},  // 0x36
    UNDEFINED,                                        // 0x37
    UNDEFINED,                                        // 0x38
    {0, 0, 0, 0, False, TYPE_1, "ADD %s,SP"},         // 0x39
    UNDEFINED,                                        // 0x3A
    UNDEFINED,                                        // 0x3B
    UNDEFINED,                                        // 0x3C
    UNDEFINED,                                        // 0x3D
    UNDEFINED,                                        // 0x3D
    UNDEFINED,                                        // 0x3F

    UNDEFINED,                                        // 0x40
    UNDEFINED,                                        // 0x41
    UNDEFINED,                                        // 0x42
    UNDEFINED,                                        // 0x44
    {0, 0, 0, 0, False, TYPE_1, "LD B,%sh"},          // 0x44
    {0, 0, 0, 0, False, TYPE_1, "LD B,%sl"},          // 0x45
    {1, 0, 1, 0, False, TYPE_5, "LD B,(%s%+d)"},      // 0x46
    UNDEFINED,                                        // 0x47
    UNDEFINED,                                        // 0x48
    UNDEFINED,                                        // 0x49
    UNDEFINED,                                        // 0x4A
    UNDEFINED,                                        // 0x4B
    {0, 0, 0, 0, False, TYPE_1, "LD C,%sh"},          // 0x4C
    {0, 0, 0, 0, False, TYPE_1, "LD C,%sl"},          // 0x4D
    {1, 0, 1, 0, False, TYPE_5, "LD C,(%s%+d)"},      // 0x4E
    UNDEFINED,                                        // 0x4F

    UNDEFINED,                                        // 0x50
    UNDEFINED,                                        // 0x51
    UNDEFINED,                                        // 0x52
    UNDEFINED,                                        // 0x52
    {0, 0, 0, 0, False, TYPE_1, "LD D,%sh"},          // 0x54
    {0, 0, 0, 0, False, TYPE_1, "LD D,%sl"},          // 0x55
    {1, 0, 1, 0, False, TYPE_5, "LD D,(%s%+d)"},      // 0x56
    UNDEFINED,                                        // 0x57
    UNDEFINED,                                        // 0x58
    UNDEFINED,                                        // 0x59
    UNDEFINED,                                        // 0x5A
    UNDEFINED,                                        // 0x5B
    {0, 0, 0, 0, False, TYPE_1, "LD E,%sh"},          // 0x5C
    {0, 0, 0, 0, False, TYPE_1, "LD E,%sl"},          // 0x5D
    {1, 0, 1, 0, False, TYPE_5, "LD E,(%s%+d)"},      // 0x5E
    UNDEFINED,                                        // 0x5F

    {0, 0, 0, 0, False, TYPE_1, "LD %sh,B"},          // 0x60
    {0, 0, 0, 0, False, TYPE_1, "LD %sh,C"},          // 0x61
    {0, 0, 0, 0, False, TYPE_1, "LD %sh,D"},          // 0x62
    {0, 0, 0, 0, False, TYPE_1, "LD %sh,E"},          // 0x63
    {0, 0, 0, 0, False, TYPE_2, "LD %sh,%sh"},        // 0x64
    {0, 0, 0, 0, False, TYPE_2, "LD %sh,%sl"},        // 0x65
    {1, 0, 1, 0, False, TYPE_5, "LD H,(%s%+d)"},      // 0x66
    {0, 0, 0, 0, False, TYPE_1, "LD %sh,A"},          // 0x67
    {0, 0, 0, 0, False, TYPE_1, "LD %sl,B"},          // 0x68
    {0, 0, 0, 0, False, TYPE_1, "LD %sl,C"},          // 0x69
    {0, 0, 0, 0, False, TYPE_1, "LD %sl,D"},          // 0x6A
    {0, 0, 0, 0, False, TYPE_1, "LD %sl,E"},          // 0x6B
    {0, 0, 0, 0, False, TYPE_2, "LD %sl,%sh"},        // 0x6C
    {0, 0, 0, 0, False, TYPE_2, "LD %sl,%sl"},        // 0x6D
    {1, 0, 1, 0, False, TYPE_5, "LD L,(%s%+d)"},      // 0x6E
    {0, 0, 0, 0, False, TYPE_1, "LD %sl,A"},          // 0x6F

    {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),B"},      // 0x70
    {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),C"},      // 0x71
    {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),D"},      // 0x72
    {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),E"},      // 0x73
    {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),H"},      // 0x74
    {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),L"},      // 0x75
    UNDEFINED,                                        // 0x76
    {1, 0, 0, 1, False, TYPE_5, "LD (%s%+d),A"},      // 0x77
    UNDEFINED,                                        // 0x78
    UNDEFINED,                                        // 0x79
    UNDEFINED,                                        // 0x7A
    UNDEFINED,                                        // 0x7B
    {0, 0, 0, 0, False, TYPE_1, "LD A,%sh"},          // 0x7C
    {0, 0, 0, 0, False, TYPE_1, "LD A,%sl"},          // 0x7D
    {1, 0, 1, 0, False, TYPE_5, "LD A,(%s%+d)"},      // 0x7E
    UNDEFINED,                                        // 0x7F

    UNDEFINED,                                        // 0x80
    UNDEFINED,                                        // 0x81
    UNDEFINED,                                        // 0x82
    UNDEFINED,                                        // 0x83
    {0, 0, 0, 0, False, TYPE_1, "ADD A,%sh"},         // 0x84
    {0, 0, 0, 0, False, TYPE_1, "ADD A,%sl"},         // 0x85
    {1, 0, 1, 0, False, TYPE_5, "ADD A,(%s%+d)"},     // 0x86
    UNDEFINED,                                        // 0x87
    UNDEFINED,                                        // 0x88
    UNDEFINED,                                        // 0x89
    UNDEFINED,                                        // 0x8A
    UNDEFINED,                                        // 0x8B
    {0, 0, 0, 0, False, TYPE_1, "ADC A,%sh"},         // 0x8C
    {0, 0, 0, 0, False, TYPE_1, "ADC A,%sl"},         // 0x8D
    {1, 0, 1, 0, False, TYPE_5, "ADC A,(%s%+d)"},     // 0x8E
    UNDEFINED,                                        // 0x8F

    UNDEFINED,                                        // 0x90
    UNDEFINED,                                        // 0x91
    UNDEFINED,                                        // 0x92
    UNDEFINED,                                        // 0x93
    {0, 0, 0, 0, False, TYPE_1, "SUB %sh"},           // 0x94
    {0, 0, 0, 0, False, TYPE_1, "SUB %sl"},           // 0x95
    {1, 0, 1, 0, False, TYPE_5, "SUB (%s%+d)"},       // 0x96
    UNDEFINED,                                        // 0x97
    UNDEFINED,                                        // 0x98
    UNDEFINED,                                        // 0x99
    UNDEFINED,                                        // 0x9A
    UNDEFINED,                                        // 0x9B
    {0, 0, 0, 0, False, TYPE_1, "SBC A,%sh"},         // 0x9C
    {0, 0, 0, 0, False, TYPE_1, "SBC A,%sl"},         // 0x9D
    {1, 0, 1, 0, False, TYPE_5, "SBC A,(%s%+d)"},     // 0x9E
    UNDEFINED,                                        // 0x9F

    UNDEFINED,                                        // 0xA0
    UNDEFINED,                                        // 0xA1
    UNDEFINED,                                        // 0xA2
    UNDEFINED,                                        // 0xA3
    {0, 0, 0, 0, False, TYPE_1, "AND %sh"},           // 0xA4
    {0, 0, 0, 0, False, TYPE_1, "AND %sl"},           // 0xA5
    {1, 0, 1, 0, False, TYPE_5, "AND (%s%+d)"},       // 0xA6
    UNDEFINED,                                        // 0xA7
    UNDEFINED,                                        // 0xA8
    UNDEFINED,                                        // 0xA9
    UNDEFINED,                                        // 0xAA
    UNDEFINED,                                        // 0xAB
    {0, 0, 0, 0, False, TYPE_1, "XOR %sh"},           // 0xAC
    {0, 0, 0, 0, False, TYPE_1, "XOR %sl"},           // 0xAD
    {1, 0, 1, 0, False, TYPE_5, "XOR (%s%+d)"},       // 0xAE
    UNDEFINED,                                        // 0xEF

    UNDEFINED,                                        // 0xB0
    UNDEFINED,                                        // 0xB1
    UNDEFINED,                                        // 0xB2
    UNDEFINED,                                        // 0xB3
    {0, 0, 0, 0, False, TYPE_1, "OR %sh"},            // 0xB4
    {0, 0, 0, 0, False, TYPE_1, "OR %sl"},            // 0xB5
    {1, 0, 1, 0, False, TYPE_5, "OR (%s%+d)"},        // 0xB6
    UNDEFINED,                                        // 0xB7
    UNDEFINED,                                        // 0xB8
    UNDEFINED,                                        // 0xB9
    UNDEFINED,                                        // 0xBA
    UNDEFINED,                                        // 0xBB
    {0, 0, 0, 0, False, TYPE_1, "CP %sh"},            // 0xBC
    {0, 0, 0, 0, False, TYPE_1, "CP %sl"},            // 0xBD
    {1, 0, 1, 0, False, TYPE_5, "CP (%s%+d)"},        // 0xBE
    UNDEFINED,                                        // 0xBF

    UNDEFINED,                                        // 0xC0
    UNDEFINED,                                        // 0xC1
    UNDEFINED,                                        // 0xC2
    UNDEFINED,                                        // 0xC3
    UNDEFINED,                                        // 0xC4
    UNDEFINED,                                        // 0xC5
    UNDEFINED,                                        // 0xC6
    UNDEFINED,                                        // 0xC7
    UNDEFINED,                                        // 0xC8
    UNDEFINED,                                        // 0xC9
    UNDEFINED,                                        // 0xCA
    UNDEFINED,                                        // 0xCB
    UNDEFINED,                                        // 0xCC
    UNDEFINED,                                        // 0xCD
    UNDEFINED,                                        // 0xCE
    UNDEFINED,                                        // 0xCF

    UNDEFINED,                                        // 0xD0
    UNDEFINED,                                        // 0xD1
    UNDEFINED,                                        // 0xD2
    UNDEFINED,                                        // 0xD3
    UNDEFINED,                                        // 0xD4
    UNDEFINED,                                        // 0xD5
    UNDEFINED,                                        // 0xD6
    UNDEFINED,                                        // 0xD7
    UNDEFINED,                                        // 0xD8
    UNDEFINED,                                        // 0xD9
    UNDEFINED,                                        // 0xDA
    UNDEFINED,                                        // 0xDB
    UNDEFINED,                                        // 0xDC
    UNDEFINED,                                        // 0xDD
    UNDEFINED,                                        // 0xDE
    UNDEFINED,                                        // 0xDF

    UNDEFINED,                                        // 0xE0
    {0, 0, 2, 0, False, TYPE_1, "POP %s"},            // 0xE1
    UNDEFINED,                                        // 0xE2
    {0, 0, 2, 2, False, TYPE_1, "EX (SP),%s"},        // 0xE3
    UNDEFINED,                                        // 0xE4
    {0, 0, 0,-2, False, TYPE_1, "PUSH %s"},           // 0xE5
    UNDEFINED,                                        // 0xE6
    UNDEFINED,                                        // 0xE7
    UNDEFINED,                                        // 0xE8
    {0, 0, 0, 0, False, TYPE_1, "JP (%s)"},           // 0xE9
    UNDEFINED,                                        // 0xEA
    UNDEFINED,                                        // 0xEB
    UNDEFINED,                                        // 0xEC
    UNDEFINED,                                        // 0xED
    UNDEFINED,                                        // 0xEE
    UNDEFINED,                                        // 0xEF

    UNDEFINED,                                        // 0xF0
    UNDEFINED,                                        // 0xF1
    UNDEFINED,                                        // 0xF2
    UNDEFINED,                                        // 0xF3
    UNDEFINED,                                        // 0xF4
    UNDEFINED,                                        // 0xF5
    UNDEFINED,                                        // 0xF7
    UNDEFINED,                                        // 0xF7
    UNDEFINED,                                        // 0xF8
    {0, 0, 0, 0, False, TYPE_1, "LD SP,%s"},          // 0xF9
    UNDEFINED,                                        // 0xFA
    UNDEFINED,                                        // 0xFB
    UNDEFINED,                                        // 0xFC
    UNDEFINED,                                        // 0xFD
    UNDEFINED,                                        // 0xFE
    UNDEFINED                                         // 0xFF
};

// Instructions with DD CB or FD CB prefix.
// For these instructions, the displacement precedes the opcode byte.
// This is handled as a special case in the code, and thus the entries
// in this table specify 0 for the displacement length.
InstrType index_bit_instructions[256] = {
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),B"},     // 0x00
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),C"},     // 0x01
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),D"},     // 0x02
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),E"},     // 0x03
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),H"},     // 0x04
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),L"},     // 0x05
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d)"},       // 0x06
   {0, 0, 1, 1, False, TYPE_5, "RLC (%s%+d),A"},     // 0x07
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),B"},     // 0x08
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),C"},     // 0x09
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),D"},     // 0x0A
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),E"},     // 0x0B
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),H"},     // 0x0C
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),L"},     // 0x0D
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d)"},       // 0x0E
   {0, 0, 1, 1, False, TYPE_5, "RRC (%s%+d),A"},     // 0x0F

   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),B"},      // 0x10
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),C"},      // 0x11
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),D"},      // 0x12
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),E"},      // 0x13
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),H"},      // 0x14
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),L"},      // 0x15
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d)"},        // 0x16
   {0, 0, 1, 1, False, TYPE_5, "RL (%s%+d),A"},      // 0x17
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),B"},      // 0x18
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),C"},      // 0x19
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),D"},      // 0x1A
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),E"},      // 0x1B
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),H"},      // 0x1C
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),L"},      // 0x1D
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d)"},        // 0x1E
   {0, 0, 1, 1, False, TYPE_5, "RR (%s%+d),A"},      // 0x1F

   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),B"},     // 0x20
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),C"},     // 0x21
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),D"},     // 0x22
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),E"},     // 0x23
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),H"},     // 0x24
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),L"},     // 0x25
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d)"},       // 0x26
   {0, 0, 1, 1, False, TYPE_5, "SLA (%s%+d),A"},     // 0x27
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),B"},     // 0x28
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),C"},     // 0x29
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),D"},     // 0x2A
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),E"},     // 0x2B
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),H"},     // 0x2C
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),L"},     // 0x2D
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d)"},       // 0x2E
   {0, 0, 1, 1, False, TYPE_5, "SRA (%s%+d),A"},     // 0x2F

   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),B"},     // 0x30
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),C"},     // 0x31
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),D"},     // 0x32
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),E"},     // 0x33
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),H"},     // 0x34
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),L"},     // 0x35
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d)"},       // 0x36
   {0, 0, 1, 1, False, TYPE_5, "SLL (%s%+d),A"},     // 0x37
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),B"},     // 0x38
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),C"},     // 0x39
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),D"},     // 0x3A
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),E"},     // 0x3B
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),H"},     // 0x3C
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),L"},     // 0x3D
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d)"},       // 0x3E
   {0, 0, 1, 1, False, TYPE_5, "SRL (%s%+d),A"},     // 0x3F

   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)"},     // 0x40
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)"},     // 0x41
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)"},     // 0x42
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)"},     // 0x43
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)"},     // 0x44
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)"},     // 0x45
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)"},     // 0x46
   {0, 0, 1, 0, False, TYPE_5, "BIT 0,(%s%+d)"},     // 0x47
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)"},     // 0x48
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)"},     // 0x49
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)"},     // 0x4A
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)"},     // 0x4B
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)"},     // 0x4C
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)"},     // 0x4D
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)"},     // 0x4E
   {0, 0, 1, 0, False, TYPE_5, "BIT 1,(%s%+d)"},     // 0x4F

   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)"},     // 0x50
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)"},     // 0x51
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)"},     // 0x52
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)"},     // 0x53
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)"},     // 0x54
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)"},     // 0x55
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)"},     // 0x56
   {0, 0, 1, 0, False, TYPE_5, "BIT 2,(%s%+d)"},     // 0x57
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)"},     // 0x58
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)"},     // 0x59
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)"},     // 0x5A
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)"},     // 0x5B
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)"},     // 0x5C
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)"},     // 0x5D
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)"},     // 0x5E
   {0, 0, 1, 0, False, TYPE_5, "BIT 3,(%s%+d)"},     // 0x5F

   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)"},     // 0x60
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)"},     // 0x61
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)"},     // 0x62
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)"},     // 0x63
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)"},     // 0x64
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)"},     // 0x65
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)"},     // 0x66
   {0, 0, 1, 0, False, TYPE_5, "BIT 4,(%s%+d)"},     // 0x67
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)"},     // 0x68
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)"},     // 0x69
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)"},     // 0x6A
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)"},     // 0x6B
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)"},     // 0x6C
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)"},     // 0x6D
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)"},     // 0x6E
   {0, 0, 1, 0, False, TYPE_5, "BIT 5,(%s%+d)"},     // 0x6F

   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)"},     // 0x70
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)"},     // 0x71
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)"},     // 0x72
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)"},     // 0x73
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)"},     // 0x74
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)"},     // 0x75
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)"},     // 0x76
   {0, 0, 1, 0, False, TYPE_5, "BIT 6,(%s%+d)"},     // 0x77
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)"},     // 0x78
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)"},     // 0x79
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)"},     // 0x7A
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)"},     // 0x7B
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)"},     // 0x7C
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)"},     // 0x7D
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)"},     // 0x7E
   {0, 0, 1, 0, False, TYPE_5, "BIT 7,(%s%+d)"},     // 0x7F

   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),B"},   // 0x80
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),C"},   // 0x81
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),D"},   // 0x82
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),E"},   // 0x83
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),H"},   // 0x84
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),L"},   // 0x85
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d)"},     // 0x86
   {0, 0, 1, 1, False, TYPE_5, "RES 0,(%s%+d),A"},   // 0x87
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),B"},   // 0x88
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),C"},   // 0x89
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),D"},   // 0x8A
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),E"},   // 0x8B
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),H"},   // 0x8C
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),L"},   // 0x8D
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d)"},     // 0x8E
   {0, 0, 1, 1, False, TYPE_5, "RES 1,(%s%+d),A"},   // 0x8F

   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),B"},   // 0x90
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),C"},   // 0x91
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),D"},   // 0x92
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),E"},   // 0x93
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),H"},   // 0x94
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),L"},   // 0x95
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d)"},     // 0x96
   {0, 0, 1, 1, False, TYPE_5, "RES 2,(%s%+d),A"},   // 0x97
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),B"},   // 0x98
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),C"},   // 0x99
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),D"},   // 0x9A
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),E"},   // 0x9B
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),H"},   // 0x9C
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),L"},   // 0x9D
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d)"},     // 0x9E
   {0, 0, 1, 1, False, TYPE_5, "RES 3,(%s%+d),A"},   // 0x9F

   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),B"},   // 0xA0
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),C"},   // 0xA1
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),D"},   // 0xA2
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),E"},   // 0xA3
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),H"},   // 0xA4
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),L"},   // 0xA5
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d)"},     // 0xA6
   {0, 0, 1, 1, False, TYPE_5, "RES 4,(%s%+d),A"},   // 0xA7
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),B"},   // 0xA8
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),C"},   // 0xA9
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),D"},   // 0xAA
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),E"},   // 0xAB
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),H"},   // 0xAC
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),L"},   // 0xAD
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d)"},     // 0xAE
   {0, 0, 1, 1, False, TYPE_5, "RES 5,(%s%+d),A"},   // 0xAF

   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),B"},   // 0xB0
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),C"},   // 0xB1
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),D"},   // 0xB2
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),E"},   // 0xB3
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),H"},   // 0xB4
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),L"},   // 0xB5
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d)"},     // 0xB6
   {0, 0, 1, 1, False, TYPE_5, "RES 6,(%s%+d),A"},   // 0xB7
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),B"},   // 0xB8
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),C"},   // 0xB9
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),D"},   // 0xBA
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),E"},   // 0xBB
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),H"},   // 0xBC
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),L"},   // 0xBD
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d)"},     // 0xBE
   {0, 0, 1, 1, False, TYPE_5, "RES 7,(%s%+d),A"},   // 0xBF

   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),B"},   // 0xC0
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),C"},   // 0xC1
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),D"},   // 0xC2
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),E"},   // 0xC3
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),H"},   // 0xC4
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),L"},   // 0xC5
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d)"},     // 0xC6
   {0, 0, 1, 1, False, TYPE_5, "SET 0,(%s%+d),A"},   // 0xC7
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),B"},   // 0xC8
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),C"},   // 0xC9
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),D"},   // 0xCA
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),E"},   // 0xCB
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),H"},   // 0xCC
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),L"},   // 0xCD
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d)"},     // 0xCE
   {0, 0, 1, 1, False, TYPE_5, "SET 1,(%s%+d),A"},   // 0xCF

   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),B"},   // 0xD0
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),C"},   // 0xD1
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),D"},   // 0xD2
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),E"},   // 0xD3
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),H"},   // 0xD4
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),L"},   // 0xD5
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d)"},     // 0xD6
   {0, 0, 1, 1, False, TYPE_5, "SET 2,(%s%+d),A"},   // 0xD7
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),B"},   // 0xD8
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),C"},   // 0xD9
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),D"},   // 0xDA
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),E"},   // 0xDB
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),H"},   // 0xDC
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),L"},   // 0xDD
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d)"},     // 0xDE
   {0, 0, 1, 1, False, TYPE_5, "SET 3,(%s%+d),A"},   // 0xDF

   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),B"},   // 0xE0
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),C"},   // 0xE1
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),D"},   // 0xE2
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),E"},   // 0xE3
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),H"},   // 0xE4
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),L"},   // 0xE5
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d)"},     // 0xE6
   {0, 0, 1, 1, False, TYPE_5, "SET 4,(%s%+d),A"},   // 0xE7
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),B"},   // 0xE8
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),C"},   // 0xE9
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),D"},   // 0xEA
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),E"},   // 0xEB
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),H"},   // 0xEC
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),L"},   // 0xED
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d)"},     // 0xEE
   {0, 0, 1, 1, False, TYPE_5, "SET 5,(%s%+d),A"},   // 0xEF

   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),B"},   // 0xF0
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),C"},   // 0xF1
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),D"},   // 0xF2
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),E"},   // 0xF3
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),H"},   // 0xF4
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),L"},   // 0xF5
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d)"},     // 0xF6
   {0, 0, 1, 1, False, TYPE_5, "SET 6,(%s%+d),A"},   // 0xF7
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),B"},   // 0xF8
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),C"},   // 0xF9
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),D"},   // 0xFA
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),E"},   // 0xFB
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),H"},   // 0xFC
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),L"},   // 0xFD
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d)"},     // 0xFE
   {0, 0, 1, 1, False, TYPE_5, "SET 7,(%s%+d),A"}    // 0xFF
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
