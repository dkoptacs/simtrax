//===- IntrinsicsTrax.td - Defines Trax intrinsics ---------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines all of the Trax-specific intrinsics.
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "trax" in {  // All intrinsics start with "llvm.trax.".
  // print instructions in Trax
  def int_trax_printi : Intrinsic<[], [llvm_i32_ty]>;
  def int_trax_printf : Intrinsic<[], [llvm_float_ty]>;
  // profile instruction
  def int_trax_profile : Intrinsic<[], [llvm_i32_ty]>;
  // global load instructions
  def int_trax_loadi : Intrinsic<[llvm_i32_ty], [llvm_i32_ty, llvm_i32_ty]>;
  def int_trax_loadf : Intrinsic<[llvm_float_ty], [llvm_i32_ty, llvm_i32_ty]>;
  // global store instructions
  def int_trax_storei : Intrinsic<[], [llvm_i32_ty, llvm_i32_ty, llvm_i32_ty]>;
  def int_trax_storef : Intrinsic<[], [llvm_float_ty, llvm_i32_ty, llvm_i32_ty]>;
  // atomic_inc
  def int_trax_atominc : Intrinsic<[llvm_i32_ty], [llvm_i32_ty]>;
  // global register read (int)
  def int_trax_globalrrd : Intrinsic<[llvm_i32_ty], [llvm_i32_ty]>;
  // min/max instructions
  def int_trax_min : Intrinsic<[llvm_float_ty], [llvm_float_ty, llvm_float_ty]>;
  def int_trax_max : Intrinsic<[llvm_float_ty], [llvm_float_ty, llvm_float_ty]>;
  // inv_sqrt instruction
  def int_trax_invsqrt : Intrinsic<[llvm_float_ty], [llvm_float_ty]>;
  // rand intrinsic
  def int_trax_rand : Intrinsic<[llvm_float_ty],[]>;
  // barrier intrinsics
  def int_trax_increset : Intrinsic<[llvm_i32_ty], [llvm_i32_ty]>;
  def int_trax_barrier : Intrinsic<[], [llvm_i32_ty]>;
  // noise intrinsics
  def int_trax_writesrf : Intrinsic<[], [llvm_float_ty, llvm_float_ty, llvm_float_ty]>;
  def int_trax_writesri : Intrinsic<[], [llvm_i32_ty, llvm_i32_ty, llvm_i32_ty]>;
  def int_trax_callnoise : Intrinsic<[llvm_float_ty], []>;
  // stream queue intrinsics
  def int_trax_startsw : Intrinsic<[], [llvm_i32_ty]>;
  def int_trax_streamwi : Intrinsic<[], [llvm_i32_ty]>;
  def int_trax_streamwf : Intrinsic<[], [llvm_float_ty]>;
  def int_trax_endsw : Intrinsic<[], []>;
  
  def int_trax_startsr : Intrinsic<[llvm_i32_ty], []>;
  def int_trax_streamri : Intrinsic<[llvm_i32_ty], []>;
  def int_trax_streamrf : Intrinsic<[llvm_float_ty], []>;
  def int_trax_endsr : Intrinsic<[], []>;

  def int_trax_streamsize : Intrinsic<[llvm_i32_ty], [llvm_i32_ty]>;
  def int_trax_streamschedule : Intrinsic<[llvm_i32_ty], [llvm_i32_ty]>;
  def int_trax_setstreamid : Intrinsic<[], [llvm_i32_ty]>;
  // ID intrinsic
  def int_trax_getid : Intrinsic<[llvm_i32_ty], [llvm_i32_ty]>;
}
