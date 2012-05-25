//===- TraxCallingConv.td - Calling Conventions for Trax -*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for Trax architecture.
//===----------------------------------------------------------------------===//

/// CCIfSubtarget - Match if the current subtarget has a feature F.
class CCIfSubtarget<string F, CCAction A>:
  CCIf<!strconcat("State.getTarget().getSubtarget<TraxSubtarget>().", F), A>;

//===----------------------------------------------------------------------===//
// Trax ABI Calling Convention
//===----------------------------------------------------------------------===//

def RetCC_Trax : CallingConv<[
  // i32 are returned in registers R3, R4
  CCIfType<[i32,f32], CCAssignToReg<[R3, R4, R5]>>
  // The problem is in this function
  // Trying this to see if it fixes the problem
  //CCIfType<[i32,f32], CCAssignToStack<4, 4>>
]>;

def CC_Trax : CallingConv<[
  CCIfType<[i32,f32], CCCustom<"CC_Trax_AssignReg">>,
  CCIfType<[i32,f32], CCAssignToStack<4, 4>>
]>;
