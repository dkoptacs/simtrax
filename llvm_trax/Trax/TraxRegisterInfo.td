//===- TraxRegisterInfo.td - Trax Register defs --------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the MicroBlaze register file
//===----------------------------------------------------------------------===//

// We have banks of 32 registers each.
class TraxReg<string n> : Register<n> {
  field bits<5> Num;
  let Namespace = "Trax";
}

// Special purpose registers have 15-bit values
class TraxSReg<string n> : Register<n> {
  field bits<15> Num;
  let Namespace = "Trax";
}

// Trax general purpose registers
class TraxGPRReg<bits<5> num, string n> : TraxReg<n> {
  let Num = num;
}

// Trax special purpose registers
class TraxSPRReg<bits<15> num, string n> : TraxSReg<n> {
  let Num = num;
}

//===----------------------------------------------------------------------===//
//  Registers
//===----------------------------------------------------------------------===//

let Namespace = "Trax" in {
  // General Purpose Registers
  def R0  : TraxGPRReg< 0,  "r0">,   DwarfRegNum<[0]>;
  def R1  : TraxGPRReg< 1,  "r1">,   DwarfRegNum<[1]>;
  def R2  : TraxGPRReg< 2,  "r2">,   DwarfRegNum<[2]>;
  def R3  : TraxGPRReg< 3,  "r3">,   DwarfRegNum<[3]>;
  def R4  : TraxGPRReg< 4,  "r4">,   DwarfRegNum<[4]>;
  def R5  : TraxGPRReg< 5,  "r5">,   DwarfRegNum<[5]>;
  def R6  : TraxGPRReg< 6,  "r6">,   DwarfRegNum<[6]>;
  def R7  : TraxGPRReg< 7,  "r7">,   DwarfRegNum<[7]>;
  def R8  : TraxGPRReg< 8,  "r8">,   DwarfRegNum<[8]>;
  def R9  : TraxGPRReg< 9,  "r9">,   DwarfRegNum<[9]>;
  def R10 : TraxGPRReg< 10, "r10">,  DwarfRegNum<[10]>;
  def R11 : TraxGPRReg< 11, "r11">,  DwarfRegNum<[11]>;
  def R12 : TraxGPRReg< 12, "r12">,  DwarfRegNum<[12]>;
  def R13 : TraxGPRReg< 13, "r13">,  DwarfRegNum<[13]>;
  def R14 : TraxGPRReg< 14, "r14">,  DwarfRegNum<[14]>;
  def R15 : TraxGPRReg< 15, "r15">,  DwarfRegNum<[15]>;
  def R16 : TraxGPRReg< 16, "r16">,  DwarfRegNum<[16]>;
  def R17 : TraxGPRReg< 17, "r17">,  DwarfRegNum<[17]>;
  def R18 : TraxGPRReg< 18, "r18">,  DwarfRegNum<[18]>;
  def R19 : TraxGPRReg< 19, "r19">,  DwarfRegNum<[19]>;
  def R20 : TraxGPRReg< 20, "r20">,  DwarfRegNum<[20]>;
  def R21 : TraxGPRReg< 21, "r21">,  DwarfRegNum<[21]>;
  def R22 : TraxGPRReg< 22, "r22">,  DwarfRegNum<[22]>;
  def R23 : TraxGPRReg< 23, "r23">,  DwarfRegNum<[23]>;
  def R24 : TraxGPRReg< 24, "r24">,  DwarfRegNum<[24]>;
  def R25 : TraxGPRReg< 25, "r25">,  DwarfRegNum<[25]>;
  def R26 : TraxGPRReg< 26, "r26">,  DwarfRegNum<[26]>;
  def R27 : TraxGPRReg< 27, "r27">,  DwarfRegNum<[27]>;
  def R28 : TraxGPRReg< 28, "r28">,  DwarfRegNum<[28]>;
  def R29 : TraxGPRReg< 29, "r29">,  DwarfRegNum<[29]>;
  def R30 : TraxGPRReg< 30, "r30">,  DwarfRegNum<[30]>;
  def R31 : TraxGPRReg< 31, "r31">,  DwarfRegNum<[31]>;

  // Special Purpose Registers
  def RPC    : TraxSPRReg<0x0000, "rpc">,    DwarfRegNum<[32]>;
  def RMSR   : TraxSPRReg<0x0001, "rmsr">,   DwarfRegNum<[33]>;
  def REAR   : TraxSPRReg<0x0003, "rear">,   DwarfRegNum<[34]>;
  def RESR   : TraxSPRReg<0x0005, "resr">,   DwarfRegNum<[35]>;
  def RFSR   : TraxSPRReg<0x0007, "rfsr">,   DwarfRegNum<[36]>;
  def RBTR   : TraxSPRReg<0x000B, "rbtr">,   DwarfRegNum<[37]>;
  def REDR   : TraxSPRReg<0x000D, "redr">,   DwarfRegNum<[38]>;
  def RPID   : TraxSPRReg<0x1000, "rpid">,   DwarfRegNum<[39]>;
  def RZPR   : TraxSPRReg<0x1001, "rzpr">,   DwarfRegNum<[40]>;
  def RTLBX  : TraxSPRReg<0x1002, "rtlbx">,  DwarfRegNum<[41]>;
  def RTLBLO : TraxSPRReg<0x1003, "rtlblo">, DwarfRegNum<[42]>;
  def RTLBHI : TraxSPRReg<0x1004, "rtlbhi">, DwarfRegNum<[43]>;
  def RPVR0  : TraxSPRReg<0x2000, "rpvr0">,  DwarfRegNum<[44]>;
  def RPVR1  : TraxSPRReg<0x2001, "rpvr1">,  DwarfRegNum<[45]>;
  def RPVR2  : TraxSPRReg<0x2002, "rpvr2">,  DwarfRegNum<[46]>;
  def RPVR3  : TraxSPRReg<0x2003, "rpvr3">,  DwarfRegNum<[47]>;
  def RPVR4  : TraxSPRReg<0x2004, "rpvr4">,  DwarfRegNum<[48]>;
  def RPVR5  : TraxSPRReg<0x2005, "rpvr5">,  DwarfRegNum<[49]>;
  def RPVR6  : TraxSPRReg<0x2006, "rpvr6">,  DwarfRegNum<[50]>;
  def RPVR7  : TraxSPRReg<0x2007, "rpvr7">,  DwarfRegNum<[51]>;
  def RPVR8  : TraxSPRReg<0x2008, "rpvr8">,  DwarfRegNum<[52]>;
  def RPVR9  : TraxSPRReg<0x2009, "rpvr9">,  DwarfRegNum<[53]>;
  def RPVR10 : TraxSPRReg<0x200A, "rpvr10">, DwarfRegNum<[54]>;
  def RPVR11 : TraxSPRReg<0x200B, "rpvr11">, DwarfRegNum<[55]>;

  // The carry bit. In the Microblaze this is really bit 29 of the
  // MSR register but this is the only bit of that register that we
  // are interested in modeling.
  def CARRY  : TraxSPRReg<0x0000, "rmsr[c]">, DwarfRegNum<[33]>;
}

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

def GPR : RegisterClass<"Trax", [i32,f32], 32,
  [
  // Return Values and Arguments
  R3, R4, R5, R6, R7, R8, R9, R10,

  // Not preserved across procedure calls
  R11, R12,

  // Callee save
  R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R30, R31,

  // Reserved
  R0,  // Always zero
  R1,  // The stack pointer
  R2,  // Read-only small data area anchor
  R13, // Read-write small data area anchor
  R14, // Return address for interrupts
  R15, // Return address for sub-routines
  R16, // Return address for trap
  R17, // Return address for exceptions
  R18, // Reserved for assembler
  R19  // The frame-pointer
  ]>
{
  let MethodProtos = [{
    iterator allocation_order_end(const MachineFunction &MF) const;
  }];
  let MethodBodies = [{
    GPRClass::iterator
    GPRClass::allocation_order_end(const MachineFunction &MF) const {
      // The last 10 registers on the list above are reserved
      return end()-10;
    }
  }];
}

def SPR : RegisterClass<"Trax", [i32], 32,
  [
  // Reserved
  RPC,
  RMSR,
  REAR,
  RESR,
  RFSR,
  RBTR,
  REDR,
  RPID,
  RZPR,
  RTLBX,
  RTLBLO,
  RTLBHI,
  RPVR0,
  RPVR1,
  RPVR2,
  RPVR3,
  RPVR4,
  RPVR5,
  RPVR6,
  RPVR7,
  RPVR8,
  RPVR9,
  RPVR10,
  RPVR11
  ]>
{
  let MethodProtos = [{
    iterator allocation_order_end(const MachineFunction &MF) const;
  }];
  let MethodBodies = [{
    SPRClass::iterator
    SPRClass::allocation_order_end(const MachineFunction &MF) const {
      // None of the special purpose registers are allocatable.
      return end()-24;
    }
  }];
}

def CRC : RegisterClass<"Trax", [i32], 32, [CARRY]> {
  let CopyCost = -1;
}
