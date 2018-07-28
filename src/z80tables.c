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

#include "z80tables.h"

#define False 0
#define True  1

#define UNDEFINED {0, 0, 0, 0, False, "???"}

// Instructions without a prefix
InstrType main_instructions[256] = {
    {0, 0, 0, 0, False, "NOP"},                       // 0x00
    {0, 2, 0, 0, False, "LD BC,{i:04H}h"},            // 0x01
    {0, 0, 0, 1, False, "LD (BC),A"},                 // 0x02
    {0, 0, 0, 0, False, "INC BC"},                    // 0x03
    {0, 0, 0, 0, False, "INC B"},                     // 0x04
    {0, 0, 0, 0, False, "DEC B"},                     // 0x05
    {0, 1, 0, 0, False, "LD B,{i:02H}h"},             // 0x06
    {0, 0, 0, 0, False, "RLCA"},                      // 0x07
    {0, 0, 0, 0, False, "EX AF,AF'"},                 // 0x08
    {0, 0, 0, 0, False, "ADD HL,BC"},                 // 0x09
    {0, 0, 1, 0, False, "LD A,(BC)"},                 // 0x0A
    {0, 0, 0, 0, False, "DEC BC"},                    // 0x0B
    {0, 0, 0, 0, False, "INC C"},                     // 0x0C
    {0, 0, 0, 0, False, "DEC C"},                     // 0x0D
    {0, 1, 0, 0, False, "LD C,{i:02H}h"},             // 0x0E
    {0, 0, 0, 0, False, "RRCA"},                      // 0x0F

    {1, 0, 0, 0, False, "DJNZ ${j:+d}"},              // 0x10
    {0, 2, 0, 0, False, "LD DE,{i:04H}h"},            // 0x11
    {0, 0, 0, 1, False, "LD (DE),A"},                 // 0x12
    {0, 0, 0, 0, False, "INC DE"},                    // 0x13
    {0, 0, 0, 0, False, "INC D"},                     // 0x14
    {0, 0, 0, 0, False, "DEC D"},                     // 0x15
    {0, 1, 0, 0, False, "LD D,{i:02H}h"},             // 0x16
    {0, 0, 0, 0, False, "RLA"},                       // 0x17
    {1, 0, 0, 0, False, "JR ${j:+d}"},                // 0x18
    {0, 0, 0, 0, False, "ADD HL,DE"},                 // 0x19
    {0, 0, 1, 0, False, "LD A,(DE)"},                 // 0x1A
    {0, 0, 0, 0, False, "DEC DE"},                    // 0x1B
    {0, 0, 0, 0, False, "INC E"},                     // 0x1C
    {0, 0, 0, 0, False, "DEC E"},                     // 0x1D
    {0, 1, 0, 0, False, "LD E,{i:02H}h"},             // 0x1E
    {0, 0, 0, 0, False, "RRA"},                       // 0x1F

    {1, 0, 0, 0, False, "JR NZ,${j:+d}"},             // 0x20
    {0, 2, 0, 0, False, "LD HL,{i:04H}h"},            // 0x21
    {0, 2, 0, 2, False, "LD ({i:04H}h),HL"},          // 0x22
    {0, 0, 0, 0, False, "INC HL"},                    // 0x23
    {0, 0, 0, 0, False, "INC H"},                     // 0x24
    {0, 0, 0, 0, False, "DEC H"},                     // 0x25
    {0, 1, 0, 0, False, "LD H,{i:02H}h"},             // 0x26
    {0, 0, 0, 0, False, "DAA"},                       // 0x27
    {1, 0, 0, 0, False, "JR Z,${j:+d}"},              // 0x28
    {0, 0, 0, 0, False, "ADD HL,HL"},                 // 0x29
    {0, 2, 2, 0, False, "LD HL,({i:04H}h)"},          // 0x2A
    {0, 0, 0, 0, False, "DEC HL"},                    // 0x2B
    {0, 0, 0, 0, False, "INC L"},                     // 0x2C
    {0, 0, 0, 0, False, "DEC L"},                     // 0x2D
    {0, 1, 0, 0, False, "LD L,{i:02H}h"},             // 0x2E
    {0, 0, 0, 0, False, "CPL"},                       // 0x2F

    {1, 0, 0, 0, False, "JR NC,${j:+d}"},             // 0x30
    {0, 2, 0, 0, False, "LD SP,{i:04H}h"},            // 0x31
    {0, 2, 0, 1, False, "LD ({i:04H}h),A"},           // 0x32
    {0, 0, 0, 0, False, "INC SP"},                    // 0x33
    {0, 0, 1, 1, False, "INC (HL)"},                  // 0x34
    {0, 0, 1, 1, False, "DEC (HL)"},                  // 0x35
    {0, 1, 0, 1, False, "LD (HL),{i:02H}h"},          // 0x36
    {0, 0, 0, 0, False, "SCF"},                       // 0x37
    {1, 0, 0, 0, False, "JR C,${j:+d}"},              // 0x38
    {0, 0, 0, 0, False, "ADD HL,SP"},                 // 0x39
    {0, 2, 1, 0, False, "LD A,({i:04H}h)"},           // 0x3A
    {0, 0, 0, 0, False, "DEC SP"},                    // 0x3B
    {0, 0, 0, 0, False, "INC A"},                     // 0x3C
    {0, 0, 0, 0, False, "DEC A"},                     // 0x3D
    {0, 1, 0, 0, False, "LD A,{i:02H}h"},             // 0x3E
    {0, 0, 0, 0, False, "CCF"},                       // 0x3F

    {0, 0, 0, 0, False, "LD B,B"},                    // 0x40
    {0, 0, 0, 0, False, "LD B,C"},                    // 0x41
    {0, 0, 0, 0, False, "LD B,D"},                    // 0x42
    {0, 0, 0, 0, False, "LD B,E"},                    // 0x43
    {0, 0, 0, 0, False, "LD B,H"},                    // 0x44
    {0, 0, 0, 0, False, "LD B,L"},                    // 0x45
    {0, 0, 1, 0, False, "LD B,(HL)"},                 // 0x46
    {0, 0, 0, 0, False, "LD B,A"},                    // 0x47
    {0, 0, 0, 0, False, "LD C,B"},                    // 0x48
    {0, 0, 0, 0, False, "LD C,C"},                    // 0x49
    {0, 0, 0, 0, False, "LD C,D"},                    // 0x4A
    {0, 0, 0, 0, False, "LD C,E"},                    // 0x4B
    {0, 0, 0, 0, False, "LD C,H"},                    // 0x4C
    {0, 0, 0, 0, False, "LD C,L"},                    // 0x4D
    {0, 0, 1, 0, False, "LD C,(HL)"},                 // 0x4E
    {0, 0, 0, 0, False, "LD C,A"},                    // 0x4F

    {0, 0, 0, 0, False, "LD D,B"},                    // 0x50
    {0, 0, 0, 0, False, "LD D,C"},                    // 0x51
    {0, 0, 0, 0, False, "LD D,D"},                    // 0x52
    {0, 0, 0, 0, False, "LD D,E"},                    // 0x53
    {0, 0, 0, 0, False, "LD D,H"},                    // 0x54
    {0, 0, 0, 0, False, "LD D,L"},                    // 0x55
    {0, 0, 1, 0, False, "LD D,(HL)"},                 // 0x56
    {0, 0, 0, 0, False, "LD D,A"},                    // 0x57
    {0, 0, 0, 0, False, "LD E,B"},                    // 0x58
    {0, 0, 0, 0, False, "LD E,C"},                    // 0x59
    {0, 0, 0, 0, False, "LD E,D"},                    // 0x5A
    {0, 0, 0, 0, False, "LD E,E"},                    // 0x5B
    {0, 0, 0, 0, False, "LD E,H"},                    // 0x5C
    {0, 0, 0, 0, False, "LD E,L"},                    // 0x5D
    {0, 0, 1, 0, False, "LD E,(HL)"},                 // 0x5E
    {0, 0, 0, 0, False, "LD E,A"},                    // 0x5F

    {0, 0, 0, 0, False, "LD H,B"},                    // 0x60
    {0, 0, 0, 0, False, "LD H,C"},                    // 0x61
    {0, 0, 0, 0, False, "LD H,D"},                    // 0x62
    {0, 0, 0, 0, False, "LD H,E"},                    // 0x63
    {0, 0, 0, 0, False, "LD H,H"},                    // 0x64
    {0, 0, 0, 0, False, "LD H,L"},                    // 0x65
    {0, 0, 1, 0, False, "LD H,(HL)"},                 // 0x66
    {0, 0, 0, 0, False, "LD H,A"},                    // 0x67
    {0, 0, 0, 0, False, "LD L,B"},                    // 0x68
    {0, 0, 0, 0, False, "LD L,C"},                    // 0x69
    {0, 0, 0, 0, False, "LD L,D"},                    // 0x6A
    {0, 0, 0, 0, False, "LD L,E"},                    // 0x6B
    {0, 0, 0, 0, False, "LD L,H"},                    // 0x6C
    {0, 0, 0, 0, False, "LD L,L"},                    // 0x6D
    {0, 0, 1, 0, False, "LD L,(HL)"},                 // 0x6E
    {0, 0, 0, 0, False, "LD L,A"},                    // 0x6F

    {0, 0, 0, 1, False, "LD (HL),B"},                 // 0x70
    {0, 0, 0, 1, False, "LD (HL),C"},                 // 0x71
    {0, 0, 0, 1, False, "LD (HL),D"},                 // 0x72
    {0, 0, 0, 1, False, "LD (HL),E"},                 // 0x73
    {0, 0, 0, 1, False, "LD (HL),H"},                 // 0x74
    {0, 0, 0, 1, False, "LD (HL),L"},                 // 0x75
    {0, 0, 0, 0, False, "HALT"},                      // 0x76
    {0, 0, 0, 1, False, "LD (HL),A"},                 // 0x77
    {0, 0, 0, 0, False, "LD A,B"},                    // 0x78
    {0, 0, 0, 0, False, "LD A,C"},                    // 0x79
    {0, 0, 0, 0, False, "LD A,D"},                    // 0x7A
    {0, 0, 0, 0, False, "LD A,E"},                    // 0x7B
    {0, 0, 0, 0, False, "LD A,H"},                    // 0x7C
    {0, 0, 0, 0, False, "LD A,L"},                    // 0x7D
    {0, 0, 1, 0, False, "LD A,(HL)"},                 // 0x7E
    {0, 0, 0, 0, False, "LD A,A"},                    // 0x7F

    {0, 0, 0, 0, False, "ADD A,B"},                   // 0x80
    {0, 0, 0, 0, False, "ADD A,C"},                   // 0x81
    {0, 0, 0, 0, False, "ADD A,D"},                   // 0x82
    {0, 0, 0, 0, False, "ADD A,E"},                   // 0x83
    {0, 0, 0, 0, False, "ADD A,H"},                   // 0x84
    {0, 0, 0, 0, False, "ADD A,L"},                   // 0x85
    {0, 0, 1, 0, False, "ADD A,(HL)"},                // 0x86
    {0, 0, 0, 0, False, "ADD A,A"},                   // 0x87
    {0, 0, 0, 0, False, "ADC A,B"},                   // 0x88
    {0, 0, 0, 0, False, "ADC A,C"},                   // 0x89
    {0, 0, 0, 0, False, "ADC A,D"},                   // 0x8A
    {0, 0, 0, 0, False, "ADC A,E"},                   // 0x8B
    {0, 0, 0, 0, False, "ADC A,H"},                   // 0x8C
    {0, 0, 0, 0, False, "ADC A,L"},                   // 0x8D
    {0, 0, 1, 0, False, "ADC A,(HL)"},                // 0x8E
    {0, 0, 0, 0, False, "ADC A,A"},                   // 0x8F

    {0, 0, 0, 0, False, "SUB B"},                     // 0x90
    {0, 0, 0, 0, False, "SUB C"},                     // 0x91
    {0, 0, 0, 0, False, "SUB D"},                     // 0x92
    {0, 0, 0, 0, False, "SUB E"},                     // 0x93
    {0, 0, 0, 0, False, "SUB H"},                     // 0x94
    {0, 0, 0, 0, False, "SUB L"},                     // 0x95
    {0, 0, 1, 0, False, "SUB (HL)"},                  // 0x96
    {0, 0, 0, 0, False, "SUB A"},                     // 0x97
    {0, 0, 0, 0, False, "SBC A,B"},                   // 0x98
    {0, 0, 0, 0, False, "SBC A,C"},                   // 0x99
    {0, 0, 0, 0, False, "SBC A,D"},                   // 0x9A
    {0, 0, 0, 0, False, "SBC A,E"},                   // 0x9B
    {0, 0, 0, 0, False, "SBC A,H"},                   // 0x9C
    {0, 0, 0, 0, False, "SBC A,L"},                   // 0x9D
    {0, 0, 1, 0, False, "SBC A,(HL)"},                // 0x9E
    {0, 0, 0, 0, False, "SBC A,A"},                   // 0x9F

    {0, 0, 0, 0, False, "AND B"},                     // 0xA0
    {0, 0, 0, 0, False, "AND C"},                     // 0xA1
    {0, 0, 0, 0, False, "AND D"},                     // 0xA2
    {0, 0, 0, 0, False, "AND E"},                     // 0xA3
    {0, 0, 0, 0, False, "AND H"},                     // 0xA4
    {0, 0, 0, 0, False, "AND L"},                     // 0xA5
    {0, 0, 1, 0, False, "AND (HL)"},                  // 0xA6
    {0, 0, 0, 0, False, "AND A"},                     // 0xA7
    {0, 0, 0, 0, False, "XOR B"},                     // 0xA8
    {0, 0, 0, 0, False, "XOR C"},                     // 0xA9
    {0, 0, 0, 0, False, "XOR D"},                     // 0xAA
    {0, 0, 0, 0, False, "XOR E"},                     // 0xAB
    {0, 0, 0, 0, False, "XOR H"},                     // 0xAC
    {0, 0, 0, 0, False, "XOR L"},                     // 0xAD
    {0, 0, 1, 0, False, "XOR (HL)"},                  // 0xAE
    {0, 0, 0, 0, False, "XOR A"},                     // 0xAF

    {0, 0, 0, 0, False, "OR B"},                      // 0xB0
    {0, 0, 0, 0, False, "OR C"},                      // 0xB1
    {0, 0, 0, 0, False, "OR D"},                      // 0xB2
    {0, 0, 0, 0, False, "OR E"},                      // 0xB3
    {0, 0, 0, 0, False, "OR H"},                      // 0xB4
    {0, 0, 0, 0, False, "OR L"},                      // 0xB5
    {0, 0, 1, 0, False, "OR (HL)"},                   // 0xB6
    {0, 0, 0, 0, False, "OR A"},                      // 0xB7
    {0, 0, 0, 0, False, "CP B"},                      // 0xB8
    {0, 0, 0, 0, False, "CP C"},                      // 0xB9
    {0, 0, 0, 0, False, "CP D"},                      // 0xBA
    {0, 0, 0, 0, False, "CP E"},                      // 0xBB
    {0, 0, 0, 0, False, "CP H"},                      // 0xBC
    {0, 0, 0, 0, False, "CP L"},                      // 0xBD
    {0, 0, 1, 0, False, "CP (HL)"},                   // 0xBE
    {0, 0, 0, 0, False, "CP A"},                      // 0xBF

    {0, 0, 2, 0, False, "RET NZ"},                    // 0xC0
    {0, 0, 2, 0, False, "POP BC"},                    // 0xC1
    {0, 2, 0, 0, False, "JP NZ,{i:04H}h"},            // 0xC2
    {0, 2, 0, 0, False, "JP {i:04H}h"},               // 0xC3
    {0, 2, 0,-2, False, "CALL NZ,{i:04H}h"},          // 0xC4
    {0, 0, 0,-2, False, "PUSH BC"},                   // 0xC5
    {0, 1, 0, 0, False, "ADD A,{i:02H}h"},            // 0xC6
    {0, 0, 0,-2, False, "RST 00h"},                   // 0xC7
    {0, 0, 2, 0, False, "RET Z"},                     // 0xC8
    {0, 0, 2, 0, False, "RET"},                       // 0xC9
    {0, 2, 0, 0, False, "JP Z,{i:04H}h"},             // 0xCA
    UNDEFINED,                                        // 0xCB
    {0, 2, 0,-2, False, "CALL Z,{i:04H}h"},           // 0xCC
    {0, 2, 0,-2, False, "CALL {i:04H}h"},             // 0xCD
    {0, 1, 0, 0, False, "ADC A,{i:02H}h"},            // 0xCE
    {0, 0, 0,-2, False, "RST 08h"},                   // 0xCF

    {0, 0, 2, 0, False, "RET NC"},                    // 0xD0
    {0, 0, 2, 0, False, "POP DE"},                    // 0xD1
    {0, 2, 0, 0, False, "JP NC,{i:04H}h"},            // 0xD2
    {0, 1, 0, 1, False, "OUT ({i:02H}h),A"},          // 0xD3
    {0, 2, 0,-2, False, "CALL NC,{i:04H}h"},          // 0xD4
    {0, 0, 0,-2, False, "PUSH DE"},                   // 0xD5
    {0, 1, 0, 0, False, "SUB {i:02H}h"},              // 0xD6
    {0, 0, 0,-2, False, "RST 10h"},                   // 0xD7
    {0, 0, 2, 0, False, "RET C"},                     // 0xD8
    {0, 0, 0, 0, False, "EXX"},                       // 0xD9
    {0, 2, 0, 0, False, "JP C,{i:04H}h"},             // 0xDA
    {0, 1, 1, 0, False, "IN A,({i:02H}h)"},           // 0xDB
    {0, 2, 0,-2, False, "CALL C,{i:04H}h"},           // 0xDC
    UNDEFINED,                                        // 0xDD
    {0, 1, 0, 0, False, "SBC A,{i:02H}h"},            // 0xDE
    {0, 0, 0,-2, False, "RST 18h"},                   // 0xDF

    {0, 0, 2, 0, False, "RET PO"},                    // 0xE0
    {0, 0, 2, 0, False, "POP HL"},                    // 0xE1
    {0, 2, 0, 0, False, "JP PO,{i:04H}h"},            // 0xE2
    {0, 0, 2, 2, False, "EX (SP),HL"},                // 0xE3
    {0, 2, 0,-2, False, "CALL PO,{i:04H}h"},          // 0xE4
    {0, 0, 0,-2, False, "PUSH HL"},                   // 0xE5
    {0, 1, 0, 0, False, "AND {i:02H}h"},              // 0xE6
    {0, 0, 0,-2, False, "RST 20h"},                   // 0xE7
    {0, 0, 2, 0, False, "RET PE"},                    // 0xE8
    {0, 0, 0, 0, False, "JP (HL)"},                   // 0xE9
    {0, 2, 0, 0, False, "JP PE,{i:04H}h"},            // 0xEA
    {0, 0, 0, 0, False, "EX DE,HL"},                  // 0xEB
    {0, 2, 0,-2, False, "CALL PE,{i:04H}h"},          // 0xEC
    UNDEFINED,                                        // 0xED
    {0, 1, 0, 0, False, "XOR {i:02H}h"},              // 0xEE
    {0, 0, 0,-2, False, "RST 28h"},                   // 0xEF

    {0, 0, 2, 0, False, "RET P"},                     // 0xF0
    {0, 0, 2, 0, False, "POP AF"},                    // 0xF1
    {0, 2, 0, 0, False, "JP P,{i:04H}h"},             // 0xF2
    {0, 0, 0, 0, False, "DI"},                        // 0xF3
    {0, 2, 0,-2, False, "CALL P,{i:04H}h"},           // 0xF4
    {0, 0, 0,-2, False, "PUSH AF"},                   // 0xF5
    {0, 1, 0, 0, False, "OR {i:02H}h"},               // 0xF6
    {0, 0, 0,-2, False, "RST 30h"},                   // 0xF7
    {0, 0, 2, 0, False, "RET M"},                     // 0xF8
    {0, 0, 0, 0, False, "LD SP,HL"},                  // 0xF9
    {0, 2, 0, 0, False, "JP M,{i:04H}h"},             // 0xFA
    {0, 0, 0, 0, False, "EI"},                        // 0xFB
    {0, 2, 0,-2, False, "CALL M,{i:04H}h"},           // 0xFC
    UNDEFINED,                                        // 0xFD
    {0, 1, 0, 0, False, "CP {i:02H}h"},               // 0xFE
    {0, 0, 0,-2, False, "RST 38h"}                    // 0xFF
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

    {0, 0, 1, 0, False, "IN B,(C)"},                  // 0x40
    {0, 0, 0, 1, False, "OUT (C),B"},                 // 0x41
    {0, 0, 0, 0, False, "SBC HL,BC"},                 // 0x42
    {0, 2, 0, 2, False, "LD ({i:04H}h),BC"},          // 0x43
    {0, 0, 0, 0, False, "NEG"},                       // 0x44
    {0, 0, 2, 0, False, "RETN"},                      // 0x45
    {0, 0, 0, 0, False, "IM 0"},                      // 0x46
    {0, 0, 0, 0, False, "LD I,A"},                    // 0x47
    {0, 0, 1, 0, False, "IN C,(C)"},                  // 0x48
    {0, 0, 0, 1, False, "OUT (C),C"},                 // 0x49
    {0, 0, 0, 0, False, "ADC HL,BC"},                 // 0x4A
    {0, 2, 2, 0, False, "LD BC,({i:04H}h)"},          // 0x4B
    {0, 0, 0, 0, False, "NEG"},                       // 0x4C
    {0, 0, 2, 0, False, "RETI"},                      // 0x4D
    {0, 0, 0, 0, False, "IM 0/1"},                    // 0x4E
    {0, 0, 0, 0, False, "LD R,A"},                    // 0x4F

    {0, 0, 1, 0, False, "IN D,(C)"},                  // 0x50
    {0, 0, 0, 1, False, "OUT (C),D"},                 // 0x51
    {0, 0, 0, 0, False, "SBC HL,DE"},                 // 0x52
    {0, 2, 0, 2, False, "LD ({i:04H}h),DE"},          // 0x53
    {0, 0, 0, 0, False, "NEG"},                       // 0x54
    {0, 0, 2, 0, False, "RETN"},                      // 0x55
    {0, 0, 0, 0, False, "IM 1"},                      // 0x56
    {0, 0, 0, 0, False, "LD A,I"},                    // 0x57
    {0, 0, 1, 0, False, "IN E,(C)"},                  // 0x58
    {0, 0, 0, 1, False, "OUT (C),E"},                 // 0x59
    {0, 0, 0, 0, False, "ADC HL,DE"},                 // 0x5A
    {0, 2, 2, 0, False, "LD DE,({i:04H}h)"},          // 0x5B
    {0, 0, 0, 0, False, "NEG"},                       // 0x5C
    {0, 0, 2, 0, False, "RETN"},                      // 0x5D
    {0, 0, 0, 0, False, "IM 2"},                      // 0x5E
    {0, 0, 0, 0, False, "LD A,R"},                    // 0x5F

    {0, 0, 1, 0, False, "IN H,(C)"},                  // 0x60
    {0, 0, 0, 1, False, "OUT (C),H"},                 // 0x61
    {0, 0, 0, 0, False, "SBC HL,HL"},                 // 0x62
    {0, 2, 0, 2, False, "LD ({i:04H}h),HL"},          // 0x63
    {0, 0, 0, 0, False, "NEG"},                       // 0x64
    {0, 0, 2, 0, False, "RETN"},                      // 0x65
    {0, 0, 0, 0, False, "IM 0"},                      // 0x66
    {0, 0, 1, 1, False, "RRD"},                       // 0x67
    {0, 0, 1, 0, False, "IN L,(C)"},                  // 0x68
    {0, 0, 0, 1, False, "OUT (C),L"},                 // 0x69
    {0, 0, 0, 0, False, "ADC HL,HL"},                 // 0x6A
    {0, 2, 2, 0, False, "LD HL,({i:04H}h)"},          // 0x6B
    {0, 0, 0, 0, False, "NEG"},                       // 0x6C
    {0, 0, 2, 0, False, "RETN"},                      // 0x6D
    {0, 0, 0, 0, False, "IM 0/1"},                    // 0x6E
    {0, 0, 1, 1, False, "RLD"},                       // 0x6F

    {0, 0, 1, 0, False, "IN (C)"},                    // 0x70
    {0, 0, 0, 1, False, "OUT (C),0"},                 // 0x71
    {0, 0, 0, 0, False, "SBC HL,SP"},                 // 0x72
    {0, 2, 0, 2, False, "LD ({i:04H}h),SP"},          // 0x73
    {0, 0, 0, 0, False, "NEG"},                       // 0x74
    {0, 0, 2, 0, False, "RETN"},                      // 0x75
    {0, 0, 0, 0, False, "IM 1"},                      // 0x76
    UNDEFINED,                                        // 0x77
    {0, 0, 1, 0, False, "IN A,(C)"},                  // 0x78
    {0, 0, 0, 1, False, "OUT (C),A"},                 // 0x79
    {0, 0, 0, 0, False, "ADC HL,SP"},                 // 0x7A
    {0, 2, 2, 0, False, "LD SP,({i:04H}h)"},          // 0x7B
    {0, 0, 0, 0, False, "NEG"},                       // 0x7C
    {0, 0, 2, 0, False, "RETN"},                      // 0x7D
    {0, 0, 0, 0, False, "IM 2"},                      // 0x7E
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

    {0, 0, 1, 1, False, "LDI"},                       // 0xA0
    {0, 0, 1, 0, False, "CPI"},                       // 0xA1
    {0, 0, 1, 1, False, "INI"},                       // 0xA2
    {0, 0, 1, 1, False, "OUTI"},                      // 0xA3
    UNDEFINED,                                        // 0xA4
    UNDEFINED,                                        // 0xA5
    UNDEFINED,                                        // 0xA6
    UNDEFINED,                                        // 0xA7
    {0, 0, 1, 1, False, "LDD"},                       // 0xA8
    {0, 0, 1, 0, False, "CPD"},                       // 0xA9
    {0, 0, 1, 1, False, "IND"},                       // 0xAA
    {0, 0, 1, 1, False, "OUTD"},                      // 0xAB
    UNDEFINED,                                        // 0xAC
    UNDEFINED,                                        // 0xAD
    UNDEFINED,                                        // 0xAE
    UNDEFINED,                                        // 0xAF

    {0, 0, 1, 1, True,  "LDIR"},                      // 0xB0
    {0, 0, 1, 0, True,  "CPIR"},                      // 0xB1
    {0, 0, 1, 1, True,  "INIR"},                      // 0xB2
    {0, 0, 1, 1, True,  "OTIR"},                      // 0xB3
    UNDEFINED,                                        // 0xB4
    UNDEFINED,                                        // 0xB5
    UNDEFINED,                                        // 0xB6
    UNDEFINED,                                        // 0xB7
    {0, 0, 1, 1, True,  "LDDR"},                      // 0xB8
    {0, 0, 1, 0, True,  "CPDR"},                      // 0xB9
    {0, 0, 1, 1, True,  "INDR"},                      // 0xBA
    {0, 0, 1, 1, True,  "OTDR"},                      // 0xBB
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
    {}                                                // 0xFF
};

// Instructions with CB prefix
InstrType bit_instructions[256] = {
    {0, 0, 0, 0, False, "RLC B"},                     // 0x00
    {0, 0, 0, 0, False, "RLC C"},                     // 0x01
    {0, 0, 0, 0, False, "RLC D"},                     // 0x02
    {0, 0, 0, 0, False, "RLC E"},                     // 0x03
    {0, 0, 0, 0, False, "RLC H"},                     // 0x04
    {0, 0, 0, 0, False, "RLC L"},                     // 0x05
    {0, 0, 1, 1, False, "RLC (HL)"},                  // 0x06
    {0, 0, 0, 0, False, "RLC A"},                     // 0x07
    {0, 0, 0, 0, False, "RRC B"},                     // 0x08
    {0, 0, 0, 0, False, "RRC C"},                     // 0x09
    {0, 0, 0, 0, False, "RRC D"},                     // 0x0A
    {0, 0, 0, 0, False, "RRC E"},                     // 0x0B
    {0, 0, 0, 0, False, "RRC H"},                     // 0x0C
    {0, 0, 0, 0, False, "RRC L"},                     // 0x0D
    {0, 0, 1, 1, False, "RRC (HL)"},                  // 0x0E
    {0, 0, 0, 0, False, "RRC A"},                     // 0x0F

    {0, 0, 0, 0, False, "RL B"},                      // 0x10
    {0, 0, 0, 0, False, "RL C"},                      // 0x11
    {0, 0, 0, 0, False, "RL D"},                      // 0x12
    {0, 0, 0, 0, False, "RL E"},                      // 0x13
    {0, 0, 0, 0, False, "RL H"},                      // 0x14
    {0, 0, 0, 0, False, "RL L"},                      // 0x15
    {0, 0, 1, 1, False, "RL (HL)"},                   // 0x16
    {0, 0, 0, 0, False, "RL A"},                      // 0x17
    {0, 0, 0, 0, False, "RR B"},                      // 0x18
    {0, 0, 0, 0, False, "RR C"},                      // 0x19
    {0, 0, 0, 0, False, "RR D"},                      // 0x1A
    {0, 0, 0, 0, False, "RR E"},                      // 0x1B
    {0, 0, 0, 0, False, "RR H"},                      // 0x1C
    {0, 0, 0, 0, False, "RR L"},                      // 0x1D
    {0, 0, 1, 1, False, "RR (HL)"},                   // 0x1E
    {0, 0, 0, 0, False, "RR A"},                      // 0x1F

    {0, 0, 0, 0, False, "SLA B"},                     // 0x20
    {0, 0, 0, 0, False, "SLA C"},                     // 0x21
    {0, 0, 0, 0, False, "SLA D"},                     // 0x22
    {0, 0, 0, 0, False, "SLA E"},                     // 0x23
    {0, 0, 0, 0, False, "SLA H"},                     // 0x24
    {0, 0, 0, 0, False, "SLA L"},                     // 0x25
    {0, 0, 1, 1, False, "SLA (HL)"},                  // 0x26
    {0, 0, 0, 0, False, "SLA A"},                     // 0x27
    {0, 0, 0, 0, False, "SRA B"},                     // 0x28
    {0, 0, 0, 0, False, "SRA C"},                     // 0x29
    {0, 0, 0, 0, False, "SRA D"},                     // 0x2A
    {0, 0, 0, 0, False, "SRA E"},                     // 0x2B
    {0, 0, 0, 0, False, "SRA H"},                     // 0x2C
    {0, 0, 0, 0, False, "SRA L"},                     // 0x2D
    {0, 0, 1, 1, False, "SRA (HL)"},                  // 0x2E
    {0, 0, 0, 0, False, "SRA A"},                     // 0x2F

    {0, 0, 0, 0, False, "SLL B"},                     // 0x30
    {0, 0, 0, 0, False, "SLL C"},                     // 0x31
    {0, 0, 0, 0, False, "SLL D"},                     // 0x32
    {0, 0, 0, 0, False, "SLL E"},                     // 0x33
    {0, 0, 0, 0, False, "SLL H"},                     // 0x34
    {0, 0, 0, 0, False, "SLL L"},                     // 0x35
    {0, 0, 1, 1, False, "SLL (HL)"},                  // 0x36
    {0, 0, 0, 0, False, "SLL A"},                     // 0x37
    {0, 0, 0, 0, False, "SRL B"},                     // 0x38
    {0, 0, 0, 0, False, "SRL C"},                     // 0x39
    {0, 0, 0, 0, False, "SRL D"},                     // 0x3A
    {0, 0, 0, 0, False, "SRL E"},                     // 0x3B
    {0, 0, 0, 0, False, "SRL H"},                     // 0x3C
    {0, 0, 0, 0, False, "SRL L"},                     // 0x3D
    {0, 0, 1, 1, False, "SRL (HL)"},                  // 0x3E
    {0, 0, 0, 0, False, "SRL A"},                     // 0x3F

    {0, 0, 0, 0, False, "BIT 0,B"},                   // 0x40
    {0, 0, 0, 0, False, "BIT 0,C"},                   // 0x41
    {0, 0, 0, 0, False, "BIT 0,D"},                   // 0x42
    {0, 0, 0, 0, False, "BIT 0,E"},                   // 0x43
    {0, 0, 0, 0, False, "BIT 0,H"},                   // 0x44
    {0, 0, 0, 0, False, "BIT 0,L"},                   // 0x45
    {0, 0, 1, 0, False, "BIT 0,(HL)"},                // 0x46
    {0, 0, 0, 0, False, "BIT 0,A"},                   // 0x47
    {0, 0, 0, 0, False, "BIT 1,B"},                   // 0x48
    {0, 0, 0, 0, False, "BIT 1,C"},                   // 0x49
    {0, 0, 0, 0, False, "BIT 1,D"},                   // 0x4A
    {0, 0, 0, 0, False, "BIT 1,E"},                   // 0x4B
    {0, 0, 0, 0, False, "BIT 1,H"},                   // 0x4C
    {0, 0, 0, 0, False, "BIT 1,L"},                   // 0x4D
    {0, 0, 1, 0, False, "BIT 1,(HL)"},                // 0x4E
    {0, 0, 0, 0, False, "BIT 1,A"},                   // 0x4F

    {0, 0, 0, 0, False, "BIT 2,B"},                   // 0x50
    {0, 0, 0, 0, False, "BIT 2,C"},                   // 0x51
    {0, 0, 0, 0, False, "BIT 2,D"},                   // 0x52
    {0, 0, 0, 0, False, "BIT 2,E"},                   // 0x53
    {0, 0, 0, 0, False, "BIT 2,H"},                   // 0x54
    {0, 0, 0, 0, False, "BIT 2,L"},                   // 0x55
    {0, 0, 1, 0, False, "BIT 2,(HL)"},                // 0x56
    {0, 0, 0, 0, False, "BIT 2,A"},                   // 0x57
    {0, 0, 0, 0, False, "BIT 3,B"},                   // 0x58
    {0, 0, 0, 0, False, "BIT 3,C"},                   // 0x59
    {0, 0, 0, 0, False, "BIT 3,D"},                   // 0x5A
    {0, 0, 0, 0, False, "BIT 3,E"},                   // 0x5B
    {0, 0, 0, 0, False, "BIT 3,H"},                   // 0x5C
    {0, 0, 0, 0, False, "BIT 3,L"},                   // 0x5D
    {0, 0, 1, 0, False, "BIT 3,(HL)"},                // 0x5E
    {0, 0, 0, 0, False, "BIT 3,A"},                   // 0x5F

    {0, 0, 0, 0, False, "BIT 4,B"},                   // 0x60
    {0, 0, 0, 0, False, "BIT 4,C"},                   // 0x61
    {0, 0, 0, 0, False, "BIT 4,D"},                   // 0x62
    {0, 0, 0, 0, False, "BIT 4,E"},                   // 0x63
    {0, 0, 0, 0, False, "BIT 4,H"},                   // 0x64
    {0, 0, 0, 0, False, "BIT 4,L"},                   // 0x65
    {0, 0, 1, 0, False, "BIT 4,(HL)"},                // 0x66
    {0, 0, 0, 0, False, "BIT 4,A"},                   // 0x67
    {0, 0, 0, 0, False, "BIT 5,B"},                   // 0x68
    {0, 0, 0, 0, False, "BIT 5,C"},                   // 0x69
    {0, 0, 0, 0, False, "BIT 5,D"},                   // 0x6A
    {0, 0, 0, 0, False, "BIT 5,E"},                   // 0x6B
    {0, 0, 0, 0, False, "BIT 5,H"},                   // 0x6C
    {0, 0, 0, 0, False, "BIT 5,L"},                   // 0x6D
    {0, 0, 1, 0, False, "BIT 5,(HL)"},                // 0x6E
    {0, 0, 0, 0, False, "BIT 5,A"},                   // 0x6F

    {0, 0, 0, 0, False, "BIT 6,B"},                   // 0x70
    {0, 0, 0, 0, False, "BIT 6,C"},                   // 0x71
    {0, 0, 0, 0, False, "BIT 6,D"},                   // 0x72
    {0, 0, 0, 0, False, "BIT 6,E"},                   // 0x73
    {0, 0, 0, 0, False, "BIT 6,H"},                   // 0x74
    {0, 0, 0, 0, False, "BIT 6,L"},                   // 0x75
    {0, 0, 1, 0, False, "BIT 6,(HL)"},                // 0x76
    {0, 0, 0, 0, False, "BIT 6,A"},                   // 0x77
    {0, 0, 0, 0, False, "BIT 7,B"},                   // 0x78
    {0, 0, 0, 0, False, "BIT 7,C"},                   // 0x79
    {0, 0, 0, 0, False, "BIT 7,D"},                   // 0x7A
    {0, 0, 0, 0, False, "BIT 7,E"},                   // 0x7B
    {0, 0, 0, 0, False, "BIT 7,H"},                   // 0x7C
    {0, 0, 0, 0, False, "BIT 7,L"},                   // 0x7D
    {0, 0, 1, 0, False, "BIT 7,(HL)"},                // 0x7E
    {0, 0, 0, 0, False, "BIT 7,A"},                   // 0x7F

    {0, 0, 0, 0, False, "RES 0,B"},                   // 0x80
    {0, 0, 0, 0, False, "RES 0,C"},                   // 0x81
    {0, 0, 0, 0, False, "RES 0,D"},                   // 0x82
    {0, 0, 0, 0, False, "RES 0,E"},                   // 0x83
    {0, 0, 0, 0, False, "RES 0,H"},                   // 0x84
    {0, 0, 0, 0, False, "RES 0,L"},                   // 0x85
    {0, 0, 1, 1, False, "RES 0,(HL)"},                // 0x86
    {0, 0, 0, 0, False, "RES 0,A"},                   // 0x87
    {0, 0, 0, 0, False, "RES 1,B"},                   // 0x88
    {0, 0, 0, 0, False, "RES 1,C"},                   // 0x89
    {0, 0, 0, 0, False, "RES 1,D"},                   // 0x8A
    {0, 0, 0, 0, False, "RES 1,E"},                   // 0x8B
    {0, 0, 0, 0, False, "RES 1,H"},                   // 0x8C
    {0, 0, 0, 0, False, "RES 1,L"},                   // 0x8D
    {0, 0, 1, 1, False, "RES 1,(HL)"},                // 0x8E
    {0, 0, 0, 0, False, "RES 1,A"},                   // 0x8F

    {0, 0, 0, 0, False, "RES 2,B"},                   // 0x90
    {0, 0, 0, 0, False, "RES 2,C"},                   // 0x91
    {0, 0, 0, 0, False, "RES 2,D"},                   // 0x92
    {0, 0, 0, 0, False, "RES 2,E"},                   // 0x93
    {0, 0, 0, 0, False, "RES 2,H"},                   // 0x94
    {0, 0, 0, 0, False, "RES 2,L"},                   // 0x95
    {0, 0, 1, 1, False, "RES 2,(HL)"},                // 0x96
    {0, 0, 0, 0, False, "RES 2,A"},                   // 0x97
    {0, 0, 0, 0, False, "RES 3,B"},                   // 0x98
    {0, 0, 0, 0, False, "RES 3,C"},                   // 0x99
    {0, 0, 0, 0, False, "RES 3,D"},                   // 0x9A
    {0, 0, 0, 0, False, "RES 3,E"},                   // 0x9B
    {0, 0, 0, 0, False, "RES 3,H"},                   // 0x9C
    {0, 0, 0, 0, False, "RES 3,L"},                   // 0x9D
    {0, 0, 1, 1, False, "RES 3,(HL)"},                // 0x9E
    {0, 0, 0, 0, False, "RES 3,A"},                   // 0x9F

    {0, 0, 0, 0, False, "RES 4,B"},                   // 0xA0
    {0, 0, 0, 0, False, "RES 4,C"},                   // 0xA1
    {0, 0, 0, 0, False, "RES 4,D"},                   // 0xA2
    {0, 0, 0, 0, False, "RES 4,E"},                   // 0xA3
    {0, 0, 0, 0, False, "RES 4,H"},                   // 0xA4
    {0, 0, 0, 0, False, "RES 4,L"},                   // 0xA5
    {0, 0, 1, 1, False, "RES 4,(HL)"},                // 0xA6
    {0, 0, 0, 0, False, "RES 4,A"},                   // 0xA7
    {0, 0, 0, 0, False, "RES 5,B"},                   // 0xA8
    {0, 0, 0, 0, False, "RES 5,C"},                   // 0xA9
    {0, 0, 0, 0, False, "RES 5,D"},                   // 0xAA
    {0, 0, 0, 0, False, "RES 5,E"},                   // 0xAB
    {0, 0, 0, 0, False, "RES 5,H"},                   // 0xAC
    {0, 0, 0, 0, False, "RES 5,L"},                   // 0xAD
    {0, 0, 1, 1, False, "RES 5,(HL)"},                // 0xAE
    {0, 0, 0, 0, False, "RES 5,A"},                   // 0xAF

    {0, 0, 0, 0, False, "RES 6,B"},                   // 0xB0
    {0, 0, 0, 0, False, "RES 6,C"},                   // 0xB1
    {0, 0, 0, 0, False, "RES 6,D"},                   // 0xB2
    {0, 0, 0, 0, False, "RES 6,E"},                   // 0xB3
    {0, 0, 0, 0, False, "RES 6,H"},                   // 0xB4
    {0, 0, 0, 0, False, "RES 6,L"},                   // 0xB5
    {0, 0, 1, 1, False, "RES 6,(HL)"},                // 0xB6
    {0, 0, 0, 0, False, "RES 6,A"},                   // 0xB7
    {0, 0, 0, 0, False, "RES 7,B"},                   // 0xB8
    {0, 0, 0, 0, False, "RES 7,C"},                   // 0xB9
    {0, 0, 0, 0, False, "RES 7,D"},                   // 0xBA
    {0, 0, 0, 0, False, "RES 7,E"},                   // 0xBB
    {0, 0, 0, 0, False, "RES 7,H"},                   // 0xBC
    {0, 0, 0, 0, False, "RES 7,L"},                   // 0xBD
    {0, 0, 1, 1, False, "RES 7,(HL)"},                // 0xBE
    {0, 0, 0, 0, False, "RES 7,A"},                   // 0xBF

    {0, 0, 0, 0, False, "SET 0,B"},                   // 0xC0
    {0, 0, 0, 0, False, "SET 0,C"},                   // 0xC1
    {0, 0, 0, 0, False, "SET 0,D"},                   // 0xC2
    {0, 0, 0, 0, False, "SET 0,E"},                   // 0xC3
    {0, 0, 0, 0, False, "SET 0,H"},                   // 0xC4
    {0, 0, 0, 0, False, "SET 0,L"},                   // 0xC5
    {0, 0, 1, 1, False, "SET 0,(HL)"},                // 0xC6
    {0, 0, 0, 0, False, "SET 0,A"},                   // 0xC7
    {0, 0, 0, 0, False, "SET 1,B"},                   // 0xC8
    {0, 0, 0, 0, False, "SET 1,C"},                   // 0xC9
    {0, 0, 0, 0, False, "SET 1,D"},                   // 0xCA
    {0, 0, 0, 0, False, "SET 1,E"},                   // 0xCB
    {0, 0, 0, 0, False, "SET 1,H"},                   // 0xCC
    {0, 0, 0, 0, False, "SET 1,L"},                   // 0xCD
    {0, 0, 1, 1, False, "SET 1,(HL)"},                // 0xCE
    {0, 0, 0, 0, False, "SET 1,A"},                   // 0xCF

    {0, 0, 0, 0, False, "SET 2,B"},                   // 0xD0
    {0, 0, 0, 0, False, "SET 2,C"},                   // 0xD1
    {0, 0, 0, 0, False, "SET 2,D"},                   // 0xD2
    {0, 0, 0, 0, False, "SET 2,E"},                   // 0xD3
    {0, 0, 0, 0, False, "SET 2,H"},                   // 0xD4
    {0, 0, 0, 0, False, "SET 2,L"},                   // 0xD5
    {0, 0, 1, 1, False, "SET 2,(HL)"},                // 0xD6
    {0, 0, 0, 0, False, "SET 2,A"},                   // 0xD7
    {0, 0, 0, 0, False, "SET 3,B"},                   // 0xD8
    {0, 0, 0, 0, False, "SET 3,C"},                   // 0xD9
    {0, 0, 0, 0, False, "SET 3,D"},                   // 0xDA
    {0, 0, 0, 0, False, "SET 3,E"},                   // 0xDB
    {0, 0, 0, 0, False, "SET 3,H"},                   // 0xDC
    {0, 0, 0, 0, False, "SET 3,L"},                   // 0xDD
    {0, 0, 1, 1, False, "SET 3,(HL)"},                // 0xDE
    {0, 0, 0, 0, False, "SET 3,A"},                   // 0xDF

    {0, 0, 0, 0, False, "SET 4,B"},                   // 0xE0
    {0, 0, 0, 0, False, "SET 4,C"},                   // 0xE1
    {0, 0, 0, 0, False, "SET 4,D"},                   // 0xE2
    {0, 0, 0, 0, False, "SET 4,E"},                   // 0xE3
    {0, 0, 0, 0, False, "SET 4,H"},                   // 0xE4
    {0, 0, 0, 0, False, "SET 4,L"},                   // 0xE5
    {0, 0, 1, 1, False, "SET 4,(HL)"},                // 0xE6
    {0, 0, 0, 0, False, "SET 4,A"},                   // 0xE7
    {0, 0, 0, 0, False, "SET 5,B"},                   // 0xE8
    {0, 0, 0, 0, False, "SET 5,C"},                   // 0xE9
    {0, 0, 0, 0, False, "SET 5,D"},                   // 0xEA
    {0, 0, 0, 0, False, "SET 5,E"},                   // 0xEB
    {0, 0, 0, 0, False, "SET 5,H"},                   // 0xEC
    {0, 0, 0, 0, False, "SET 5,L"},                   // 0xED
    {0, 0, 1, 1, False, "SET 5,(HL)"},                // 0xEE
    {0, 0, 0, 0, False, "SET 5,A"},                   // 0xEF

    {0, 0, 0, 0, False, "SET 6,B"},                   // 0xF0
    {0, 0, 0, 0, False, "SET 6,C"},                   // 0xF1
    {0, 0, 0, 0, False, "SET 6,D"},                   // 0xF2
    {0, 0, 0, 0, False, "SET 6,E"},                   // 0xF3
    {0, 0, 0, 0, False, "SET 6,H"},                   // 0xF4
    {0, 0, 0, 0, False, "SET 6,L"},                   // 0xF5
    {0, 0, 1, 1, False, "SET 6,(HL)"},                // 0xF6
    {0, 0, 0, 0, False, "SET 6,A"},                   // 0xF7
    {0, 0, 0, 0, False, "SET 7,B"},                   // 0xF8
    {0, 0, 0, 0, False, "SET 7,C"},                   // 0xF9
    {0, 0, 0, 0, False, "SET 7,D"},                   // 0xFA
    {0, 0, 0, 0, False, "SET 7,E"},                   // 0xFB
    {0, 0, 0, 0, False, "SET 7,H"},                   // 0xFC
    {0, 0, 0, 0, False, "SET 7,L"},                   // 0xFD
    {0, 0, 1, 1, False, "SET 7,(HL)"},                // 0xFE
    {0, 0, 0, 0, False, "SET 7,A"}                    // 0xFF
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
    {0, 0, 0, 0, False, "ADD {r},BC"},                // 0x09
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
    {0, 0, 0, 0, False, "ADD {r},DE"},                // 0x19
    UNDEFINED,                                        // 0x1A
    UNDEFINED,                                        // 0x1B
    UNDEFINED,                                        // 0x1C
    UNDEFINED,                                        // 0x1D
    UNDEFINED,                                        // 0x1E
    UNDEFINED,                                        // 0x1F

    UNDEFINED,                                        // 0x20
    {0, 2, 0, 0, False, "LD {r},{i:04H}h"},           // 0x21
    {0, 2, 0, 2, False, "LD ({i:04H}h),{r}"},         // 0x22
    {0, 0, 0, 0, False, "INC {r}"},                   // 0x23
    {0, 0, 0, 0, False, "INC {r}h"},                  // 0x24
    {0, 0, 0, 0, False, "DEC {r}h"},                  // 0x25
    {0, 1, 0, 0, False, "LD {r}h,{i:02H}h"},          // 0x26
    UNDEFINED,                                        // 0x27
    UNDEFINED,                                        // 0x28
    {0, 0, 0, 0, False, "ADD {r},{r}"},               // 0x29
    {0, 2, 2, 0, False, "LD {r},({i:04H}h)"},         // 0x2A
    {0, 0, 0, 0, False, "DEC {r}"},                   // 0x2B
    {0, 0, 0, 0, False, "INC {r}l"},                  // 0x2C
    {0, 0, 0, 0, False, "DEC {r}l"},                  // 0x2D
    {0, 1, 0, 0, False, "LD {r}l,{i:02H}h"},          // 0x2E
    UNDEFINED,                                        // 0x2F

    UNDEFINED,                                        // 0x30
    UNDEFINED,                                        // 0x31
    UNDEFINED,                                        // 0x32
    UNDEFINED,                                        // 0x33
    {1, 0, 1, 1, False, "INC ({r}{d:+d})"},           // 0x34
    {1, 0, 1, 1, False, "DEC ({r}{d:+d})"},           // 0x35
    {1, 1, 0, 1, False, "LD ({r}{d:+d}),{i:02H}h"},   // 0x36
    UNDEFINED,                                        // 0x37
    UNDEFINED,                                        // 0x38
    {0, 0, 0, 0, False, "ADD {r},SP"},                // 0x39
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
    {0, 0, 0, 0, False, "LD B,{r}h"},                 // 0x44
    {0, 0, 0, 0, False, "LD B,{r}l"},                 // 0x45
    {1, 0, 1, 0, False, "LD B,({r}{d:+d})"},          // 0x46
    UNDEFINED,                                        // 0x47
    UNDEFINED,                                        // 0x48
    UNDEFINED,                                        // 0x49
    UNDEFINED,                                        // 0x4A
    UNDEFINED,                                        // 0x4B
    {0, 0, 0, 0, False, "LD C,{r}h"},                 // 0x4C
    {0, 0, 0, 0, False, "LD C,{r}l"},                 // 0x4D
    {1, 0, 1, 0, False, "LD C,({r}{d:+d})"},          // 0x4E
    UNDEFINED,                                        // 0x4F

    UNDEFINED,                                        // 0x50
    UNDEFINED,                                        // 0x51
    UNDEFINED,                                        // 0x52
    UNDEFINED,                                        // 0x52
    {0, 0, 0, 0, False, "LD D,{r}h"},                 // 0x54
    {0, 0, 0, 0, False, "LD D,{r}l"},                 // 0x55
    {1, 0, 1, 0, False, "LD D,({r}{d:+d})"},          // 0x56
    UNDEFINED,                                        // 0x57
    UNDEFINED,                                        // 0x58
    UNDEFINED,                                        // 0x59
    UNDEFINED,                                        // 0x5A
    UNDEFINED,                                        // 0x5B
    {0, 0, 0, 0, False, "LD E,{r}h"},                 // 0x5C
    {0, 0, 0, 0, False, "LD E,{r}l"},                 // 0x5D
    {1, 0, 1, 0, False, "LD E,({r}{d:+d})"},          // 0x5E
    UNDEFINED,                                        // 0x5F

    {0, 0, 0, 0, False, "LD {r}h,B"},                 // 0x60
    {0, 0, 0, 0, False, "LD {r}h,C"},                 // 0x61
    {0, 0, 0, 0, False, "LD {r}h,D"},                 // 0x62
    {0, 0, 0, 0, False, "LD {r}h,E"},                 // 0x63
    {0, 0, 0, 0, False, "LD {r}h,{r}h"},              // 0x64
    {0, 0, 0, 0, False, "LD {r}h,{r}l"},              // 0x65
    {1, 0, 1, 0, False, "LD H,({r}{d:+d})"},          // 0x66
    {0, 0, 0, 0, False, "LD {r}h,A"},                 // 0x67
    {0, 0, 0, 0, False, "LD {r}l,B"},                 // 0x68
    {0, 0, 0, 0, False, "LD {r}l,C"},                 // 0x69
    {0, 0, 0, 0, False, "LD {r}l,D"},                 // 0x6A
    {0, 0, 0, 0, False, "LD {r}l,E"},                 // 0x6B
    {0, 0, 0, 0, False, "LD {r}l,{r}h"},              // 0x6C
    {0, 0, 0, 0, False, "LD {r}l,{r}l"},              // 0x6D
    {1, 0, 1, 0, False, "LD L,({r}{d:+d})"},          // 0x6E
    {0, 0, 0, 0, False, "LD {r}l,A"},                 // 0x6F

    {1, 0, 0, 1, False, "LD ({r}{d:+d}),B"},          // 0x70
    {1, 0, 0, 1, False, "LD ({r}{d:+d}),C"},          // 0x71
    {1, 0, 0, 1, False, "LD ({r}{d:+d}),D"},          // 0x72
    {1, 0, 0, 1, False, "LD ({r}{d:+d}),E"},          // 0x73
    {1, 0, 0, 1, False, "LD ({r}{d:+d}),H"},          // 0x74
    {1, 0, 0, 1, False, "LD ({r}{d:+d}),L"},          // 0x75
    UNDEFINED,                                        // 0x76
    {1, 0, 0, 1, False, "LD ({r}{d:+d}),A"},          // 0x77
    UNDEFINED,                                        // 0x78
    UNDEFINED,                                        // 0x79
    UNDEFINED,                                        // 0x7A
    UNDEFINED,                                        // 0x7B
    {0, 0, 0, 0, False, "LD A,{r}h"},                 // 0x7C
    {0, 0, 0, 0, False, "LD A,{r}l"},                 // 0x7D
    {1, 0, 1, 0, False, "LD A,({r}{d:+d})"},          // 0x7E
    UNDEFINED,                                        // 0x7F

    UNDEFINED,                                        // 0x80
    UNDEFINED,                                        // 0x81
    UNDEFINED,                                        // 0x82
    UNDEFINED,                                        // 0x83
    {0, 0, 0, 0, False, "ADD A,{r}h"},                // 0x84
    {0, 0, 0, 0, False, "ADD A,{r}l"},                // 0x85
    {1, 0, 1, 0, False, "ADD A,({r}{d:+d})"},         // 0x86
    UNDEFINED,                                        // 0x87
    UNDEFINED,                                        // 0x88
    UNDEFINED,                                        // 0x89
    UNDEFINED,                                        // 0x8A
    UNDEFINED,                                        // 0x8B
    {0, 0, 0, 0, False, "ADC A,{r}h"},                // 0x8C
    {0, 0, 0, 0, False, "ADC A,{r}l"},                // 0x8D
    {1, 0, 1, 0, False, "ADC A,({r}{d:+d})"},         // 0x8E
    UNDEFINED,                                        // 0x8F

    UNDEFINED,                                        // 0x90
    UNDEFINED,                                        // 0x91
    UNDEFINED,                                        // 0x92
    UNDEFINED,                                        // 0x93
    {0, 0, 0, 0, False, "SUB {r}h"},                  // 0x94
    {0, 0, 0, 0, False, "SUB {r}l"},                  // 0x95
    {1, 0, 1, 0, False, "SUB ({r}{d:+d})"},           // 0x96
    UNDEFINED,                                        // 0x97
    UNDEFINED,                                        // 0x98
    UNDEFINED,                                        // 0x99
    UNDEFINED,                                        // 0x9A
    UNDEFINED,                                        // 0x9B
    {0, 0, 0, 0, False, "SBC A,{r}h"},                // 0x9C
    {0, 0, 0, 0, False, "SBC A,{r}l"},                // 0x9D
    {1, 0, 1, 0, False, "SBC A,({r}{d:+d})"},         // 0x9E
    UNDEFINED,                                        // 0x9F

    UNDEFINED,                                        // 0xA0
    UNDEFINED,                                        // 0xA1
    UNDEFINED,                                        // 0xA2
    UNDEFINED,                                        // 0xA3
    {0, 0, 0, 0, False, "AND {r}h"},                  // 0xA4
    {0, 0, 0, 0, False, "AND {r}l"},                  // 0xA5
    {1, 0, 1, 0, False, "AND ({r}{d:+d})"},           // 0xA6
    UNDEFINED,                                        // 0xA7
    UNDEFINED,                                        // 0xA8
    UNDEFINED,                                        // 0xA9
    UNDEFINED,                                        // 0xAA
    UNDEFINED,                                        // 0xAB
    {0, 0, 0, 0, False, "XOR {r}h"},                  // 0xAC
    {0, 0, 0, 0, False, "XOR {r}l"},                  // 0xAD
    {1, 0, 1, 0, False, "XOR ({r}{d:+d})"},           // 0xAE
    UNDEFINED,                                        // 0xEF

    UNDEFINED,                                        // 0xB0
    UNDEFINED,                                        // 0xB1
    UNDEFINED,                                        // 0xB2
    UNDEFINED,                                        // 0xB3
    {0, 0, 0, 0, False, "OR {r}h"},                   // 0xB4
    {0, 0, 0, 0, False, "OR {r}l"},                   // 0xB5
    {1, 0, 1, 0, False, "OR ({r}{d:+d})"},            // 0xB6
    UNDEFINED,                                        // 0xB7
    UNDEFINED,                                        // 0xB8
    UNDEFINED,                                        // 0xB9
    UNDEFINED,                                        // 0xBA
    UNDEFINED,                                        // 0xBB
    {0, 0, 0, 0, False, "CP {r}h"},                   // 0xBC
    {0, 0, 0, 0, False, "CP {r}l"},                   // 0xBD
    {1, 0, 1, 0, False, "CP ({r}{d:+d})"},            // 0xBE
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
    {0, 0, 2, 0, False, "POP {r}"},                   // 0xE1
    UNDEFINED,                                        // 0xE2
    {0, 0, 2, 2, False, "EX (SP),{r}"},               // 0xE3
    UNDEFINED,                                        // 0xE4
    {0, 0, 0,-2, False, "PUSH {r}"},                  // 0xE5
    UNDEFINED,                                        // 0xE6
    UNDEFINED,                                        // 0xE7
    UNDEFINED,                                        // 0xE8
    {0, 0, 0, 0, False, "JP ({r})"},                  // 0xE9
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
    {0, 0, 0, 0, False, "LD SP,{r}"},                 // 0xF9
    UNDEFINED,                                        // 0xFA
    UNDEFINED,                                        // 0xFB
    UNDEFINED,                                        // 0xFC
    UNDEFINED,                                        // 0xFD
    UNDEFINED,                                        // 0xFE
    {}                                                // 0xFF
};

// Instructions with DD CB or FD CB prefix.
// For these instructions, the displacement precedes the opcode byte.
// This is handled as a special case in the code, and thus the entries
// in this table specify 0 for the displacement length.
InstrType index_bit_instructions[256] = {
    {0, 0, 1, 1, False, "RLC ({r}{d:+d}),B"},         // 0x00
    {0, 0, 1, 1, False, "RLC ({r}{d:+d}),C"},         // 0x01
    {0, 0, 1, 1, False, "RLC ({r}{d:+d}),D"},         // 0x02
    {0, 0, 1, 1, False, "RLC ({r}{d:+d}),E"},         // 0x03
    {0, 0, 1, 1, False, "RLC ({r}{d:+d}),H"},         // 0x04
    {0, 0, 1, 1, False, "RLC ({r}{d:+d}),L"},         // 0x05
    {0, 0, 1, 1, False, "RLC ({r}{d:+d})"},           // 0x06
    {0, 0, 1, 1, False, "RLC ({r}{d:+d}),A"},         // 0x07
    {0, 0, 1, 1, False, "RRC ({r}{d:+d}),B"},         // 0x08
    {0, 0, 1, 1, False, "RRC ({r}{d:+d}),C"},         // 0x09
    {0, 0, 1, 1, False, "RRC ({r}{d:+d}),D"},         // 0x0A
    {0, 0, 1, 1, False, "RRC ({r}{d:+d}),E"},         // 0x0B
    {0, 0, 1, 1, False, "RRC ({r}{d:+d}),H"},         // 0x0C
    {0, 0, 1, 1, False, "RRC ({r}{d:+d}),L"},         // 0x0D
    {0, 0, 1, 1, False, "RRC ({r}{d:+d})"},           // 0x0E
    {0, 0, 1, 1, False, "RRC ({r}{d:+d}),A"},         // 0x0F

    {0, 0, 1, 1, False, "RL ({r}{d:+d}),B"},          // 0x10
    {0, 0, 1, 1, False, "RL ({r}{d:+d}),C"},          // 0x11
    {0, 0, 1, 1, False, "RL ({r}{d:+d}),D"},          // 0x12
    {0, 0, 1, 1, False, "RL ({r}{d:+d}),E"},          // 0x13
    {0, 0, 1, 1, False, "RL ({r}{d:+d}),H"},          // 0x14
    {0, 0, 1, 1, False, "RL ({r}{d:+d}),L"},          // 0x15
    {0, 0, 1, 1, False, "RL ({r}{d:+d})"},            // 0x16
    {0, 0, 1, 1, False, "RL ({r}{d:+d}),A"},          // 0x17
    {0, 0, 1, 1, False, "RR ({r}{d:+d}),B"},          // 0x18
    {0, 0, 1, 1, False, "RR ({r}{d:+d}),C"},          // 0x19
    {0, 0, 1, 1, False, "RR ({r}{d:+d}),D"},          // 0x1A
    {0, 0, 1, 1, False, "RR ({r}{d:+d}),E"},          // 0x1B
    {0, 0, 1, 1, False, "RR ({r}{d:+d}),H"},          // 0x1C
    {0, 0, 1, 1, False, "RR ({r}{d:+d}),L"},          // 0x1D
    {0, 0, 1, 1, False, "RR ({r}{d:+d})"},            // 0x1E
    {0, 0, 1, 1, False, "RR ({r}{d:+d}),A"},          // 0x1F

    {0, 0, 1, 1, False, "SLA ({r}{d:+d}),B"},         // 0x20
    {0, 0, 1, 1, False, "SLA ({r}{d:+d}),C"},         // 0x21
    {0, 0, 1, 1, False, "SLA ({r}{d:+d}),D"},         // 0x22
    {0, 0, 1, 1, False, "SLA ({r}{d:+d}),E"},         // 0x23
    {0, 0, 1, 1, False, "SLA ({r}{d:+d}),H"},         // 0x24
    {0, 0, 1, 1, False, "SLA ({r}{d:+d}),L"},         // 0x25
    {0, 0, 1, 1, False, "SLA ({r}{d:+d})"},           // 0x26
    {0, 0, 1, 1, False, "SLA ({r}{d:+d}),A"},         // 0x27
    {0, 0, 1, 1, False, "SRA ({r}{d:+d}),B"},         // 0x28
    {0, 0, 1, 1, False, "SRA ({r}{d:+d}),C"},         // 0x29
    {0, 0, 1, 1, False, "SRA ({r}{d:+d}),D"},         // 0x2A
    {0, 0, 1, 1, False, "SRA ({r}{d:+d}),E"},         // 0x2B
    {0, 0, 1, 1, False, "SRA ({r}{d:+d}),H"},         // 0x2C
    {0, 0, 1, 1, False, "SRA ({r}{d:+d}),L"},         // 0x2D
    {0, 0, 1, 1, False, "SRA ({r}{d:+d})"},           // 0x2E
    {0, 0, 1, 1, False, "SRA ({r}{d:+d}),A"},         // 0x2F

    {0, 0, 1, 1, False, "SLL ({r}{d:+d}),B"},         // 0x30
    {0, 0, 1, 1, False, "SLL ({r}{d:+d}),C"},         // 0x31
    {0, 0, 1, 1, False, "SLL ({r}{d:+d}),D"},         // 0x32
    {0, 0, 1, 1, False, "SLL ({r}{d:+d}),E"},         // 0x33
    {0, 0, 1, 1, False, "SLL ({r}{d:+d}),H"},         // 0x34
    {0, 0, 1, 1, False, "SLL ({r}{d:+d}),L"},         // 0x35
    {0, 0, 1, 1, False, "SLL ({r}{d:+d})"},           // 0x36
    {0, 0, 1, 1, False, "SLL ({r}{d:+d}),A"},         // 0x37
    {0, 0, 1, 1, False, "SRL ({r}{d:+d}),B"},         // 0x38
    {0, 0, 1, 1, False, "SRL ({r}{d:+d}),C"},         // 0x39
    {0, 0, 1, 1, False, "SRL ({r}{d:+d}),D"},         // 0x3A
    {0, 0, 1, 1, False, "SRL ({r}{d:+d}),E"},         // 0x3B
    {0, 0, 1, 1, False, "SRL ({r}{d:+d}),H"},         // 0x3C
    {0, 0, 1, 1, False, "SRL ({r}{d:+d}),L"},         // 0x3D
    {0, 0, 1, 1, False, "SRL ({r}{d:+d})"},           // 0x3E
    {0, 0, 1, 1, False, "SRL ({r}{d:+d}),A"},         // 0x3F

    {0, 0, 1, 0, False, "BIT 0,({r}{d:+d})"},         // 0x40
    {0, 0, 1, 0, False, "BIT 0,({r}{d:+d})"},         // 0x41
    {0, 0, 1, 0, False, "BIT 0,({r}{d:+d})"},         // 0x42
    {0, 0, 1, 0, False, "BIT 0,({r}{d:+d})"},         // 0x43
    {0, 0, 1, 0, False, "BIT 0,({r}{d:+d})"},         // 0x44
    {0, 0, 1, 0, False, "BIT 0,({r}{d:+d})"},         // 0x45
    {0, 0, 1, 0, False, "BIT 0,({r}{d:+d})"},         // 0x46
    {0, 0, 1, 0, False, "BIT 0,({r}{d:+d})"},         // 0x47
    {0, 0, 1, 0, False, "BIT 1,({r}{d:+d})"},         // 0x48
    {0, 0, 1, 0, False, "BIT 1,({r}{d:+d})"},         // 0x49
    {0, 0, 1, 0, False, "BIT 1,({r}{d:+d})"},         // 0x4A
    {0, 0, 1, 0, False, "BIT 1,({r}{d:+d})"},         // 0x4B
    {0, 0, 1, 0, False, "BIT 1,({r}{d:+d})"},         // 0x4C
    {0, 0, 1, 0, False, "BIT 1,({r}{d:+d})"},         // 0x4D
    {0, 0, 1, 0, False, "BIT 1,({r}{d:+d})"},         // 0x4E
    {0, 0, 1, 0, False, "BIT 1,({r}{d:+d})"},         // 0x4F

    {0, 0, 1, 0, False, "BIT 2,({r}{d:+d})"},         // 0x50
    {0, 0, 1, 0, False, "BIT 2,({r}{d:+d})"},         // 0x51
    {0, 0, 1, 0, False, "BIT 2,({r}{d:+d})"},         // 0x52
    {0, 0, 1, 0, False, "BIT 2,({r}{d:+d})"},         // 0x53
    {0, 0, 1, 0, False, "BIT 2,({r}{d:+d})"},         // 0x54
    {0, 0, 1, 0, False, "BIT 2,({r}{d:+d})"},         // 0x55
    {0, 0, 1, 0, False, "BIT 2,({r}{d:+d})"},         // 0x56
    {0, 0, 1, 0, False, "BIT 2,({r}{d:+d})"},         // 0x57
    {0, 0, 1, 0, False, "BIT 3,({r}{d:+d})"},         // 0x58
    {0, 0, 1, 0, False, "BIT 3,({r}{d:+d})"},         // 0x59
    {0, 0, 1, 0, False, "BIT 3,({r}{d:+d})"},         // 0x5A
    {0, 0, 1, 0, False, "BIT 3,({r}{d:+d})"},         // 0x5B
    {0, 0, 1, 0, False, "BIT 3,({r}{d:+d})"},         // 0x5C
    {0, 0, 1, 0, False, "BIT 3,({r}{d:+d})"},         // 0x5D
    {0, 0, 1, 0, False, "BIT 3,({r}{d:+d})"},         // 0x5E
    {0, 0, 1, 0, False, "BIT 3,({r}{d:+d})"},         // 0x5F

    {0, 0, 1, 0, False, "BIT 4,({r}{d:+d})"},         // 0x60
    {0, 0, 1, 0, False, "BIT 4,({r}{d:+d})"},         // 0x61
    {0, 0, 1, 0, False, "BIT 4,({r}{d:+d})"},         // 0x62
    {0, 0, 1, 0, False, "BIT 4,({r}{d:+d})"},         // 0x63
    {0, 0, 1, 0, False, "BIT 4,({r}{d:+d})"},         // 0x64
    {0, 0, 1, 0, False, "BIT 4,({r}{d:+d})"},         // 0x65
    {0, 0, 1, 0, False, "BIT 4,({r}{d:+d})"},         // 0x66
    {0, 0, 1, 0, False, "BIT 4,({r}{d:+d})"},         // 0x67
    {0, 0, 1, 0, False, "BIT 5,({r}{d:+d})"},         // 0x68
    {0, 0, 1, 0, False, "BIT 5,({r}{d:+d})"},         // 0x69
    {0, 0, 1, 0, False, "BIT 5,({r}{d:+d})"},         // 0x6A
    {0, 0, 1, 0, False, "BIT 5,({r}{d:+d})"},         // 0x6B
    {0, 0, 1, 0, False, "BIT 5,({r}{d:+d})"},         // 0x6C
    {0, 0, 1, 0, False, "BIT 5,({r}{d:+d})"},         // 0x6D
    {0, 0, 1, 0, False, "BIT 5,({r}{d:+d})"},         // 0x6E
    {0, 0, 1, 0, False, "BIT 5,({r}{d:+d})"},         // 0x6F

    {0, 0, 1, 0, False, "BIT 6,({r}{d:+d})"},         // 0x70
    {0, 0, 1, 0, False, "BIT 6,({r}{d:+d})"},         // 0x71
    {0, 0, 1, 0, False, "BIT 6,({r}{d:+d})"},         // 0x72
    {0, 0, 1, 0, False, "BIT 6,({r}{d:+d})"},         // 0x73
    {0, 0, 1, 0, False, "BIT 6,({r}{d:+d})"},         // 0x74
    {0, 0, 1, 0, False, "BIT 6,({r}{d:+d})"},         // 0x75
    {0, 0, 1, 0, False, "BIT 6,({r}{d:+d})"},         // 0x76
    {0, 0, 1, 0, False, "BIT 6,({r}{d:+d})"},         // 0x77
    {0, 0, 1, 0, False, "BIT 7,({r}{d:+d})"},         // 0x78
    {0, 0, 1, 0, False, "BIT 7,({r}{d:+d})"},         // 0x79
    {0, 0, 1, 0, False, "BIT 7,({r}{d:+d})"},         // 0x7A
    {0, 0, 1, 0, False, "BIT 7,({r}{d:+d})"},         // 0x7B
    {0, 0, 1, 0, False, "BIT 7,({r}{d:+d})"},         // 0x7C
    {0, 0, 1, 0, False, "BIT 7,({r}{d:+d})"},         // 0x7D
    {0, 0, 1, 0, False, "BIT 7,({r}{d:+d})"},         // 0x7E
    {0, 0, 1, 0, False, "BIT 7,({r}{d:+d})"},         // 0x7F

    {0, 0, 1, 1, False, "RES 0,({r}{d:+d}),B"},       // 0x80
    {0, 0, 1, 1, False, "RES 0,({r}{d:+d}),C"},       // 0x81
    {0, 0, 1, 1, False, "RES 0,({r}{d:+d}),D"},       // 0x82
    {0, 0, 1, 1, False, "RES 0,({r}{d:+d}),E"},       // 0x83
    {0, 0, 1, 1, False, "RES 0,({r}{d:+d}),H"},       // 0x84
    {0, 0, 1, 1, False, "RES 0,({r}{d:+d}),L"},       // 0x85
    {0, 0, 1, 1, False, "RES 0,({r}{d:+d})"},         // 0x86
    {0, 0, 1, 1, False, "RES 0,({r}{d:+d}),A"},       // 0x87
    {0, 0, 1, 1, False, "RES 1,({r}{d:+d}),B"},       // 0x88
    {0, 0, 1, 1, False, "RES 1,({r}{d:+d}),C"},       // 0x89
    {0, 0, 1, 1, False, "RES 1,({r}{d:+d}),D"},       // 0x8A
    {0, 0, 1, 1, False, "RES 1,({r}{d:+d}),E"},       // 0x8B
    {0, 0, 1, 1, False, "RES 1,({r}{d:+d}),H"},       // 0x8C
    {0, 0, 1, 1, False, "RES 1,({r}{d:+d}),L"},       // 0x8D
    {0, 0, 1, 1, False, "RES 1,({r}{d:+d})"},         // 0x8E
    {0, 0, 1, 1, False, "RES 1,({r}{d:+d}),A"},       // 0x8F

    {0, 0, 1, 1, False, "RES 2,({r}{d:+d}),B"},       // 0x90
    {0, 0, 1, 1, False, "RES 2,({r}{d:+d}),C"},       // 0x91
    {0, 0, 1, 1, False, "RES 2,({r}{d:+d}),D"},       // 0x92
    {0, 0, 1, 1, False, "RES 2,({r}{d:+d}),E"},       // 0x93
    {0, 0, 1, 1, False, "RES 2,({r}{d:+d}),H"},       // 0x94
    {0, 0, 1, 1, False, "RES 2,({r}{d:+d}),L"},       // 0x95
    {0, 0, 1, 1, False, "RES 2,({r}{d:+d})"},         // 0x96
    {0, 0, 1, 1, False, "RES 2,({r}{d:+d}),A"},       // 0x97
    {0, 0, 1, 1, False, "RES 3,({r}{d:+d}),B"},       // 0x98
    {0, 0, 1, 1, False, "RES 3,({r}{d:+d}),C"},       // 0x99
    {0, 0, 1, 1, False, "RES 3,({r}{d:+d}),D"},       // 0x9A
    {0, 0, 1, 1, False, "RES 3,({r}{d:+d}),E"},       // 0x9B
    {0, 0, 1, 1, False, "RES 3,({r}{d:+d}),H"},       // 0x9C
    {0, 0, 1, 1, False, "RES 3,({r}{d:+d}),L"},       // 0x9D
    {0, 0, 1, 1, False, "RES 3,({r}{d:+d})"},         // 0x9E
    {0, 0, 1, 1, False, "RES 3,({r}{d:+d}),A"},       // 0x9F

    {0, 0, 1, 1, False, "RES 4,({r}{d:+d}),B"},       // 0xA0
    {0, 0, 1, 1, False, "RES 4,({r}{d:+d}),C"},       // 0xA1
    {0, 0, 1, 1, False, "RES 4,({r}{d:+d}),D"},       // 0xA2
    {0, 0, 1, 1, False, "RES 4,({r}{d:+d}),E"},       // 0xA3
    {0, 0, 1, 1, False, "RES 4,({r}{d:+d}),H"},       // 0xA4
    {0, 0, 1, 1, False, "RES 4,({r}{d:+d}),L"},       // 0xA5
    {0, 0, 1, 1, False, "RES 4,({r}{d:+d})"},         // 0xA6
    {0, 0, 1, 1, False, "RES 4,({r}{d:+d}),A"},       // 0xA7
    {0, 0, 1, 1, False, "RES 5,({r}{d:+d}),B"},       // 0xA8
    {0, 0, 1, 1, False, "RES 5,({r}{d:+d}),C"},       // 0xA9
    {0, 0, 1, 1, False, "RES 5,({r}{d:+d}),D"},       // 0xAA
    {0, 0, 1, 1, False, "RES 5,({r}{d:+d}),E"},       // 0xAB
    {0, 0, 1, 1, False, "RES 5,({r}{d:+d}),H"},       // 0xAC
    {0, 0, 1, 1, False, "RES 5,({r}{d:+d}),L"},       // 0xAD
    {0, 0, 1, 1, False, "RES 5,({r}{d:+d})"},         // 0xAE
    {0, 0, 1, 1, False, "RES 5,({r}{d:+d}),A"},       // 0xAF

    {0, 0, 1, 1, False, "RES 6,({r}{d:+d}),B"},       // 0xB0
    {0, 0, 1, 1, False, "RES 6,({r}{d:+d}),C"},       // 0xB1
    {0, 0, 1, 1, False, "RES 6,({r}{d:+d}),D"},       // 0xB2
    {0, 0, 1, 1, False, "RES 6,({r}{d:+d}),E"},       // 0xB3
    {0, 0, 1, 1, False, "RES 6,({r}{d:+d}),H"},       // 0xB4
    {0, 0, 1, 1, False, "RES 6,({r}{d:+d}),L"},       // 0xB5
    {0, 0, 1, 1, False, "RES 6,({r}{d:+d})"},         // 0xB6
    {0, 0, 1, 1, False, "RES 6,({r}{d:+d}),A"},       // 0xB7
    {0, 0, 1, 1, False, "RES 7,({r}{d:+d}),B"},       // 0xB8
    {0, 0, 1, 1, False, "RES 7,({r}{d:+d}),C"},       // 0xB9
    {0, 0, 1, 1, False, "RES 7,({r}{d:+d}),D"},       // 0xBA
    {0, 0, 1, 1, False, "RES 7,({r}{d:+d}),E"},       // 0xBB
    {0, 0, 1, 1, False, "RES 7,({r}{d:+d}),H"},       // 0xBC
    {0, 0, 1, 1, False, "RES 7,({r}{d:+d}),L"},       // 0xBD
    {0, 0, 1, 1, False, "RES 7,({r}{d:+d})"},         // 0xBE
    {0, 0, 1, 1, False, "RES 7,({r}{d:+d}),A"},       // 0xBF

    {0, 0, 1, 1, False, "SET 0,({r}{d:+d}),B"},       // 0xC0
    {0, 0, 1, 1, False, "SET 0,({r}{d:+d}),C"},       // 0xC1
    {0, 0, 1, 1, False, "SET 0,({r}{d:+d}),D"},       // 0xC2
    {0, 0, 1, 1, False, "SET 0,({r}{d:+d}),E"},       // 0xC3
    {0, 0, 1, 1, False, "SET 0,({r}{d:+d}),H"},       // 0xC4
    {0, 0, 1, 1, False, "SET 0,({r}{d:+d}),L"},       // 0xC5
    {0, 0, 1, 1, False, "SET 0,({r}{d:+d})"},         // 0xC6
    {0, 0, 1, 1, False, "SET 0,({r}{d:+d}),A"},       // 0xC7
    {0, 0, 1, 1, False, "SET 1,({r}{d:+d}),B"},       // 0xC8
    {0, 0, 1, 1, False, "SET 1,({r}{d:+d}),C"},       // 0xC9
    {0, 0, 1, 1, False, "SET 1,({r}{d:+d}),D"},       // 0xCA
    {0, 0, 1, 1, False, "SET 1,({r}{d:+d}),E"},       // 0xCB
    {0, 0, 1, 1, False, "SET 1,({r}{d:+d}),H"},       // 0xCC
    {0, 0, 1, 1, False, "SET 1,({r}{d:+d}),L"},       // 0xCD
    {0, 0, 1, 1, False, "SET 1,({r}{d:+d})"},         // 0xCE
    {0, 0, 1, 1, False, "SET 1,({r}{d:+d}),A"},       // 0xCF

    {0, 0, 1, 1, False, "SET 2,({r}{d:+d}),B"},       // 0xD0
    {0, 0, 1, 1, False, "SET 2,({r}{d:+d}),C"},       // 0xD1
    {0, 0, 1, 1, False, "SET 2,({r}{d:+d}),D"},       // 0xD2
    {0, 0, 1, 1, False, "SET 2,({r}{d:+d}),E"},       // 0xD3
    {0, 0, 1, 1, False, "SET 2,({r}{d:+d}),H"},       // 0xD4
    {0, 0, 1, 1, False, "SET 2,({r}{d:+d}),L"},       // 0xD5
    {0, 0, 1, 1, False, "SET 2,({r}{d:+d})"},         // 0xD6
    {0, 0, 1, 1, False, "SET 2,({r}{d:+d}),A"},       // 0xD7
    {0, 0, 1, 1, False, "SET 3,({r}{d:+d}),B"},       // 0xD8
    {0, 0, 1, 1, False, "SET 3,({r}{d:+d}),C"},       // 0xD9
    {0, 0, 1, 1, False, "SET 3,({r}{d:+d}),D"},       // 0xDA
    {0, 0, 1, 1, False, "SET 3,({r}{d:+d}),E"},       // 0xDB
    {0, 0, 1, 1, False, "SET 3,({r}{d:+d}),H"},       // 0xDC
    {0, 0, 1, 1, False, "SET 3,({r}{d:+d}),L"},       // 0xDD
    {0, 0, 1, 1, False, "SET 3,({r}{d:+d})"},         // 0xDE
    {0, 0, 1, 1, False, "SET 3,({r}{d:+d}),A"},       // 0xDF

    {0, 0, 1, 1, False, "SET 4,({r}{d:+d}),B"},       // 0xE0
    {0, 0, 1, 1, False, "SET 4,({r}{d:+d}),C"},       // 0xE1
    {0, 0, 1, 1, False, "SET 4,({r}{d:+d}),D"},       // 0xE2
    {0, 0, 1, 1, False, "SET 4,({r}{d:+d}),E"},       // 0xE3
    {0, 0, 1, 1, False, "SET 4,({r}{d:+d}),H"},       // 0xE4
    {0, 0, 1, 1, False, "SET 4,({r}{d:+d}),L"},       // 0xE5
    {0, 0, 1, 1, False, "SET 4,({r}{d:+d})"},         // 0xE6
    {0, 0, 1, 1, False, "SET 4,({r}{d:+d}),A"},       // 0xE7
    {0, 0, 1, 1, False, "SET 5,({r}{d:+d}),B"},       // 0xE8
    {0, 0, 1, 1, False, "SET 5,({r}{d:+d}),C"},       // 0xE9
    {0, 0, 1, 1, False, "SET 5,({r}{d:+d}),D"},       // 0xEA
    {0, 0, 1, 1, False, "SET 5,({r}{d:+d}),E"},       // 0xEB
    {0, 0, 1, 1, False, "SET 5,({r}{d:+d}),H"},       // 0xEC
    {0, 0, 1, 1, False, "SET 5,({r}{d:+d}),L"},       // 0xED
    {0, 0, 1, 1, False, "SET 5,({r}{d:+d})"},         // 0xEE
    {0, 0, 1, 1, False, "SET 5,({r}{d:+d}),A"},       // 0xEF

    {0, 0, 1, 1, False, "SET 6,({r}{d:+d}),B"},       // 0xF0
    {0, 0, 1, 1, False, "SET 6,({r}{d:+d}),C"},       // 0xF1
    {0, 0, 1, 1, False, "SET 6,({r}{d:+d}),D"},       // 0xF2
    {0, 0, 1, 1, False, "SET 6,({r}{d:+d}),E"},       // 0xF3
    {0, 0, 1, 1, False, "SET 6,({r}{d:+d}),H"},       // 0xF4
    {0, 0, 1, 1, False, "SET 6,({r}{d:+d}),L"},       // 0xF5
    {0, 0, 1, 1, False, "SET 6,({r}{d:+d})"},         // 0xF6
    {0, 0, 1, 1, False, "SET 6,({r}{d:+d}),A"},       // 0xF7
    {0, 0, 1, 1, False, "SET 7,({r}{d:+d}),B"},       // 0xF8
    {0, 0, 1, 1, False, "SET 7,({r}{d:+d}),C"},       // 0xF9
    {0, 0, 1, 1, False, "SET 7,({r}{d:+d}),D"},       // 0xFA
    {0, 0, 1, 1, False, "SET 7,({r}{d:+d}),E"},       // 0xFB
    {0, 0, 1, 1, False, "SET 7,({r}{d:+d}),H"},       // 0xFC
    {0, 0, 1, 1, False, "SET 7,({r}{d:+d}),L"},       // 0xFD
    {0, 0, 1, 1, False, "SET 7,({r}{d:+d})"},         // 0xFE
    {0, 0, 1, 1, False, "SET 7,({r}{d:+d}),A"}        // 0xFF
};

//InstrType instr_table_by_prefix[256] = {
//    0:      (main_instructions,      ""),
//    0xED:   (extended_instructions,  ""),
//    0xCB:   (bit_instructions,       ""),
//    0xDD:   (index_instructions,     "IX"),
//    0xFD:   (index_instructions,     "IY"),
//    0xDDCB: (index_bit_instructions, "IX"),
//    0xFDCB: (index_bit_instructions, "IY")
//}
