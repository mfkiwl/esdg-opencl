//===-- LE1ISelLowering.cpp - LE1 DAG Lowering Implementation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that LE1 uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "le1-lower"
#include "LE1ISelLowering.h"
#include "LE1MachineFunction.h"
#include "LE1TargetMachine.h"
//#include "LE1TargetObjectFile.h"
#include "LE1Subtarget.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Intrinsics.h"
#include "llvm/CallingConv.h"
#include "InstPrinter/LE1InstPrinter.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetLibraryInfo.h"

//#include <iostream>

using namespace llvm;

// If I is a shifted mask, set the size (Size) and the first bit of the 
// mask (Pos), and return true.
// For example, if I is 0x003ff800, (Pos, Size) = (11, 11).  
//static bool IsShiftedMask(uint64_t I, uint64_t &Pos, uint64_t &Size) {
  //if (!isUInt<32>(I) || !isShiftedMask_32(I))
    // return false;

  //Size = CountPopulation_32(I);
  //Pos = CountTrailingZeros_32(I);
  //return true;
//}

const char *LE1TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case LE1ISD::Call:          return "LE1ISD::Call";
  case LE1ISD::Goto:          return "LE1ISD::Goto";
  case LE1ISD::BRF:           return "LE1ISD::BRF";
  case LE1ISD::BR:            return "LE1ISD::BR";
  case LE1ISD::Ret:           return "LE1ISD::Ret";
  case LE1ISD::RetFlag:       return "LE1ISD::RetFlag";
  case LE1ISD::Exit:          return "LE1ISD::Exit";

  case LE1ISD::Mov:           return "LE1ISD::Mov";
  case LE1ISD::MTB:           return "LE1ISD::MTB";
  case LE1ISD::MFB:           return "LE1ISD::MFB";
  case LE1ISD::MTBINV:        return "LE1ISD::MTBINV";

  case LE1ISD::Nandl:         return "LE1ISD::Nandl";
  case LE1ISD::Norl:          return "LE1ISD::Norl";
  case LE1ISD::Orl:           return "LE1ISD::Orl";
  case LE1ISD::Andc:          return "LE1ISD::Andc";
  case LE1ISD::Andl:          return "LE1ISD::Andl";
  case LE1ISD::Orc:           return "LE1ISD::Orc";

  case LE1ISD::Mull:          return "LE1ISD::Mull";
  case LE1ISD::Mulh:          return "LE1ISD::Mulh";
  case LE1ISD::Mulhs:         return "LE1ISD::Mulhs";
  case LE1ISD::Mulll:         return "LE1ISD::Mulll";
  case LE1ISD::Mullh:         return "LE1ISD::Mullh";
  case LE1ISD::Mulhh:         return "LE1ISD::Mulhh";
  case LE1ISD::Addcg:         return "LE1ISD::Addcg";
  case LE1ISD::Divs:          return "LE1ISD::Divs";

  case LE1ISD::Cmpeq:         return "LE1ISD::Cmpeq";
  case LE1ISD::Cmpge:         return "LE1ISD::Cmpge";
  case LE1ISD::Cmpgeu:        return "LE1ISD::Cmpgeu";
  case LE1ISD::Cmpgt:         return "LE1ISD::Cmpgt";
  case LE1ISD::Cmpgtu:        return "LE1ISD::Cmpgtu";
  case LE1ISD::Cmple:         return "LE1ISD::Cmple";
  case LE1ISD::Cmpleu:        return "LE1ISD::Cmpleu";
  case LE1ISD::Cmplt:         return "LE1ISD::Cmplt";
  case LE1ISD::Cmpltu:        return "LE1ISD::Cmpltu";
  case LE1ISD::Cmpne:         return "LE1ISD::Cmpne";

  case LE1ISD::Max:           return "LE1ISD::Max";
  case LE1ISD::Min:           return "LE1ISD::Min";

  case LE1ISD::SXTB:          return "LE1ISD::SXTB";
  case LE1ISD::SXTH:          return "LE1ISD::SXTH";
  case LE1ISD::ZXTB:          return "LE1ISD::ZXTB";
  case LE1ISD::ZXTH:          return "LE1ISD::ZXTH";

  case LE1ISD::Sh1Add:        return "LE1ISD::Sh1add";
  case LE1ISD::Sh2Add:        return "LE1ISD::Sh2add";
  case LE1ISD::Sh3Add:        return "LE1ISD::Sh3add";
  case LE1ISD::Sh4Add:        return "LE1ISD::Sh4add";

  case LE1ISD::Tbit:          return "LE1ISD::Tbit";
  case LE1ISD::Tbitf:         return "LE1ISD::Tbitf";
  case LE1ISD::Sbit:          return "LE1ISD::Sbit";
  case LE1ISD::Sbitf:         return "LE1ISD::Sbitf";

  case LE1ISD::CPUID:         return "LE1ISD::CPUID";
  case LE1ISD::LocalSize:     return "LE1ISD::LocalSize";
  case LE1ISD::GlobalId:      return "LE1ISD::GlobalId";
  case LE1ISD::READ_GROUP_ID: return "LE1ISD::READ_GROUP_ID";
  case LE1ISD::GROUP_ID_ADDR: return "LE1ISD::GROUP_ID_ADDR";
  case LE1ISD::LOAD_GROUP_ID: return "LE1ISD::LOAD_GROUP_ID";
  case LE1ISD::READ_ATTR:     return "LE1ISD::READ_ATTR";
  case LE1ISD::SET_ATTR:      return "LE1ISD::SET_ATTR";

  default:                         return NULL;
  }
}

LE1TargetLowering::
LE1TargetLowering(LE1TargetMachine &TM)
  : TargetLowering(TM, new TargetLoweringObjectFileELF()),
    Subtarget(&TM.getSubtarget<LE1Subtarget>()) {
  
  // LE1 does not have i1 type, so use i32 for
  // setcc operations results (slt, sgt, ...).
  //setBooleanVectorContents(ZeroOrOneBooleanContent); // FIXME: Is this correct?

  // Set up the register classes
  addRegisterClass(MVT::i32, &LE1::CPURegsRegClass);
  addRegisterClass(MVT::i1, &LE1::BRegsRegClass);

  computeRegisterProperties();
  
  setSelectIsExpensive(false);
  setIntDivIsCheap(false);
  // FIXME How to set default scheduler?
  //setSchedulingPreference(Sched::VLIW);

  // Basically try to completely remove calls to these library functions
  maxStoresPerMemset = 100;
  maxStoresPerMemsetOptSize = 100;
  maxStoresPerMemmove = 100;
  maxStoresPerMemmoveOptSize = 100;
  maxStoresPerMemcpy = 100;
  maxStoresPerMemcpyOptSize = 100;

  // Load extended operations for i1 types must be promoted
  setLoadExtAction(ISD::EXTLOAD,  MVT::i1,  Promote);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i1,  Promote);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i1,  Promote);

  /*
  setCondCodeAction(ISD::SETUGT,  MVT::i1,  Expand);
  setCondCodeAction(ISD::SETUGE,  MVT::i1,  Expand);
  setCondCodeAction(ISD::SETULT,  MVT::i1,  Expand);
  setCondCodeAction(ISD::SETULE,  MVT::i1,  Expand);
  setCondCodeAction(ISD::SETGT,   MVT::i1,  Expand);
  setCondCodeAction(ISD::SETGE,   MVT::i1,  Expand);
  setCondCodeAction(ISD::SETLT,   MVT::i1,  Expand);
  setCondCodeAction(ISD::SETLE,   MVT::i1,  Expand);
  setCondCodeAction(ISD::SETEQ,   MVT::i1,  Expand);
  setCondCodeAction(ISD::SETNE,   MVT::i1,  Legal);*/

  // Handle Vectors Comparisons
  // FIXME This screws with legalisation of boolean SetCC? Bug?
  for (unsigned i = (unsigned)MVT::FIRST_VECTOR_VALUETYPE;
       i <= (unsigned)MVT::LAST_VECTOR_VALUETYPE; ++i) {
    MVT VT((MVT::SimpleValueType)i);
    setCondCodeAction(ISD::SETUGT,  VT, Expand);
    setCondCodeAction(ISD::SETUGE,  VT, Expand);
    setCondCodeAction(ISD::SETULT,  VT, Expand);
    setCondCodeAction(ISD::SETULE,  VT, Expand);
    setCondCodeAction(ISD::SETGT,   VT, Expand);
    setCondCodeAction(ISD::SETGE,   VT, Expand);
    setCondCodeAction(ISD::SETLT,   VT, Expand);
    setCondCodeAction(ISD::SETLE,   VT, Expand);
    setCondCodeAction(ISD::SETEQ,   VT, Expand);
    setCondCodeAction(ISD::SETNE,   VT, Expand);
    setCondCodeAction(ISD::SETOEQ,  VT, Expand);
    setCondCodeAction(ISD::SETUEQ,  VT, Expand);
    setCondCodeAction(ISD::SETOGE,  VT, Expand);
    setCondCodeAction(ISD::SETOGT,  VT, Expand);
    setCondCodeAction(ISD::SETOLT,  VT, Expand);
    setCondCodeAction(ISD::SETOLE,  VT, Expand);
    setOperationAction(ISD::SETCC,   VT, Expand);
  }

  // LE1 doesn't have extending float->double load/store
  //setLoadExtAction(ISD::EXTLOAD, MVT::f32, Expand);
  //setTruncStoreAction(MVT::f64, MVT::f32, Expand);

  // LE1 Custom Operations
  // If we're compiling a library, expand the code using addcg and divs
  
  //if(Subtarget->hasExpandDiv()) {
    setOperationAction(ISD::SDIV,             MVT::i32, Custom);
    setOperationAction(ISD::UDIV,             MVT::i32, Custom);
    setOperationAction(ISD::SREM,             MVT::i32, Custom);
    setOperationAction(ISD::UREM,             MVT::i32, Custom);
    /*
  } else  {
    setLibcallName(RTLIB::SDIV_I32, "_i_div");
    setOperationAction(ISD::SDIV,   MVT::i32, Expand);
    setLibcallName(RTLIB::SREM_I32, "_i_rem");
    setOperationAction(ISD::SREM,   MVT::i32, Expand);

    setLibcallName(RTLIB::UDIV_I32, "_i_udiv");
    setOperationAction(ISD::UDIV,   MVT::i32, Expand);
    setLibcallName(RTLIB::UREM_I32, "_i_urem");
    setOperationAction(ISD::UREM,   MVT::i32, Expand);
  }*/
  //setOperationAction(ISD::SDIV, MVT::i32, Expand);
  //setOperationAction(ISD::UDIV, MVT::i32, Expand);

    /*
  // Softfloat Floating Point Library Calls
  // Integer to Float conversions
  setLibcallName(RTLIB::SINTTOFP_I32_F32, "int32_to_float32");
  //setLibcallName(RTLIB::SINTTOFP_I32_F32, "_r_ilfloat");
  setOperationAction(ISD::SINT_TO_FP, MVT::i32, Expand);

  // FIXME - Is this ok? Is sign detected..?
  setLibcallName(RTLIB::UINTTOFP_I32_F32, "int32_to_float32");
  setOperationAction(ISD::UINT_TO_FP, MVT::i32, Expand);

  setLibcallName(RTLIB::SINTTOFP_I32_F64, "int32_to_float64");
  //setLibcallName(RTLIB::SINTTOFP_I32_F64, "_d_ilfloat");
  setOperationAction(ISD::SINT_TO_FP, MVT::i32, Expand);

  //setLibcallName(RTLIB::UINTTOFP_I32_F64, "_d_ufloat");
  //setOperationAction(ISD::UINT_TO_FP, MVT::i32, Expand);

  //Software IEC/IEEE single-precision conversion routines.
  setLibcallName(RTLIB::FPTOSINT_F32_I32, "float32_to_int32");
  setOperationAction(ISD::FP_TO_SINT, MVT::f32, Expand);

  //FIXME
  //float32_to_int32_round_to_zero

  setLibcallName(RTLIB::FPEXT_F32_F64, "float32_to_float64");
  //setLibcallName(RTLIB::FPEXT_F32_F64, "_d_r");
  setOperationAction(ISD::FP_EXTEND, MVT::f32, Expand);

  //Software IEC/IEEE single-precision operations.
  // FIXME are these roundings correct? There is NEARBYINT_F too..
  setLibcallName(RTLIB::RINT_F32, "float32_round_to_int");
  setOperationAction(ISD::FRINT , MVT::f32, Expand);

  setLibcallName(RTLIB::ADD_F32, "float32_add");
  //setLibcallName(RTLIB::ADD_F32, "_r_add");
  setOperationAction(ISD::FADD, MVT::f32, Expand);

  setLibcallName(RTLIB::SUB_F32, "float32_sub");
  //setLibcallName(RTLIB::SUB_F32, "_r_sub");
  setOperationAction(ISD::FSUB, MVT::f32, Expand);




  setLibcallName(RTLIB::REM_F32, "float32_rem");
  setOperationAction(ISD::SREM, MVT::f32, Expand);
  //setLibcallName(RTLIB::UREM_F32, "float32_rem");
  setOperationAction(ISD::UREM, MVT::f32, Expand);

  setLibcallName(RTLIB::SQRT_F32, "float32_sqrt");

  setLibcallName(RTLIB::OEQ_F32, "float32_eq");
  //setLibcallName(RTLIB::OEQ_F32, "_r_eq");
  setOperationAction(ISD::SETOEQ, MVT::f32, Expand);

  setLibcallName(RTLIB::OLE_F32, "float32_le");
  //setLibcallName(RTLIB::OLE_F32, "_r_le");
  setOperationAction(ISD::SETOLE, MVT::f32, Expand);

  setLibcallName(RTLIB::OLT_F32, "float32_lt");
  //setLibcallName(RTLIB::OLT_F32, "_r_lt");
  setOperationAction(ISD::SETOLT, MVT::f32, Expand);

  // TODO I think that the functions for gt and lt are the same accept for
  // their NaN handling.
  setLibcallName(RTLIB::OGT_F32, "float32_lt");
  setOperationAction(ISD::SETOGT, MVT::f32, Expand);

  setLibcallName(RTLIB::UO_F32,  "float32_is_signaling_nan");

  // FIXME
  //float32_eq_signaling
  //float32_le_quiet
  //float32_lt_quiet
  //float32_is_signaling_nan

  //Software IEC/IEEE double-precision conversion routines.
  setLibcallName(RTLIB::FPTOSINT_F64_I32, "float64_to_int32");
  setOperationAction(ISD::FP_TO_SINT, MVT::f64, Expand);

  // FIXME
  //float64_to_int32_round_to_zero
 
  setLibcallName(RTLIB::FPROUND_F64_F32, "float64_to_float32");
  setOperationAction(ISD::FP_ROUND, MVT::f64, Expand);

  //Software IEC/IEEE double-precision operations.
  // FIXME are these roundings correct? There is NEARBYINT_F too..
  setLibcallName(RTLIB::RINT_F64, "float64_round_to_int");
  setOperationAction(ISD::FRINT, MVT::f64, Expand);

  setLibcallName(RTLIB::ADD_F64, "float64_add");
  //setLibcallName(RTLIB::ADD_F64, "_d_add");
  setOperationAction(ISD::FADD, MVT::f64, Expand);

  setLibcallName(RTLIB::SUB_F64, "float64_sub");
  //setLibcallName(RTLIB::SUB_F64, "_d_sub");
  setOperationAction(ISD::FSUB, MVT::f64, Expand);

  setLibcallName(RTLIB::MUL_F64, "float64_mul");
  //setLibcallName(RTLIB::MUL_F64, "_d_mul");
  setOperationAction(ISD::FMUL, MVT::f64, Expand);

  setLibcallName(RTLIB::DIV_F64, "float64_div");
  //setLibcallName(RTLIB::DIV_F64, "_d_div");
  setOperationAction(ISD::FDIV, MVT::f64, Expand);

  setLibcallName(RTLIB::REM_F64, "float64_rem");
  setOperationAction(ISD::SREM, MVT::f64, Expand);
  //setLibcallName(RTLIB::UREM_F64, "float64_rem");
  setOperationAction(ISD::UREM, MVT::f64, Expand);

  setLibcallName(RTLIB::SQRT_F64, "float64_sqrt");
  setOperationAction(ISD::FSQRT  , MVT::f64, Expand);

  setLibcallName(RTLIB::OEQ_F64, "float64_eq");
  //setLibcallName(RTLIB::OEQ_F64, "_d_eq");
  setOperationAction(ISD::SETOEQ, MVT::f64, Expand);

  setLibcallName(RTLIB::OLE_F64, "float64_le");
  //setLibcallName(RTLIB::OLE_F64, "_d_le");
  setOperationAction(ISD::SETOLE, MVT::f32, Expand);

  setLibcallName(RTLIB::OLT_F64, "float64_lt");
  //setLibcallName(RTLIB::OLT_F64, "_d_lt");
  setOperationAction(ISD::SETOLT, MVT::f64, Expand);

  setOperationAction(LibFunc::exp2,   MVT::f32, Expand);
  setOperationAction(LibFunc::exp2f,  MVT::f32, Expand);
  setOperationAction(LibFunc::exp2,   MVT::f64, Expand);
  setOperationAction(LibFunc::exp2f,  MVT::f64, Expand);
  */
  setLibcallName(RTLIB::MUL_F32, "float32_mul");
  setOperationAction(ISD::FMUL, MVT::f32, Expand);
  setLibcallName(RTLIB::DIV_F32, "float32_div");
  setOperationAction(ISD::FDIV, MVT::f32, Expand);

  // FIXME
  //float64_eq_signaling
  //float64_le_quiet
  //float64_lt_quiet
  //float64_is_signaling_nan

  setOperationAction(ISD::GlobalAddress,      MVT::i32,   Custom);
  setOperationAction(ISD::GlobalAddress,      MVT::i16,   Custom);
  setOperationAction(ISD::GlobalAddress,      MVT::i8,    Custom);

  setOperationAction(ISD::VASTART,            MVT::Other, Custom);

  // FIXME This should use Addcg and be custom
  setOperationAction(ISD::ADDE,             MVT::i32, Expand);
  setOperationAction(ISD::SUBE,             MVT::i32, Expand);

  // Operations not directly supported by LE1.
  setOperationAction(ISD::SDIV,             MVT::i64, Expand);
  setOperationAction(ISD::UDIV,             MVT::i64, Expand);
  //setOperationAction(ISD::SREM,             MVT::i32, Expand);
  //setOperationAction(ISD::UREM,             MVT::i32, Expand);
  setOperationAction(ISD::SDIVREM,          MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM,          MVT::i32, Expand);
  setOperationAction(ISD::SREM,             MVT::i64, Expand);
  setOperationAction(ISD::UREM,             MVT::i64, Expand);

  setOperationAction(ISD::ROTL,             MVT::i32, Expand);
  setOperationAction(ISD::ROTR,             MVT::i32, Expand);
  setOperationAction(ISD::ROTL,             MVT::i64, Expand);
  setOperationAction(ISD::ROTR,             MVT::i64, Expand);

  setOperationAction(ISD::SMUL_LOHI,        MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI,        MVT::i32, Expand);
  setOperationAction(ISD::MULHS,            MVT::i32, Custom);
  setOperationAction(ISD::MULHU,            MVT::i32, Custom);

  setOperationAction(ISD::BR_JT,            MVT::Other, Expand);
  setOperationAction(ISD::BR_CC,            MVT::Other, Expand);
  setOperationAction(ISD::BRIND,            MVT::Other, Expand);
  //setOperationAction(ISD::SELECT_CC,         MVT::Other, Expand);
  setOperationAction(ISD::BSWAP,            MVT::i32, Expand);
  setOperationAction(ISD::CTPOP,            MVT::i32, Expand);
  setOperationAction(ISD::CTTZ,             MVT::i32, Expand);
  setOperationAction(ISD::CTLZ,             MVT::i32, Expand);
  setOperationAction(ISD::CTTZ_ZERO_UNDEF,  MVT::i32, Expand);
  setOperationAction(ISD::CTLZ_ZERO_UNDEF,  MVT::i32, Expand);

  setOperationAction(ISD::SHL_PARTS,        MVT::i32,   Expand);
  setOperationAction(ISD::SRA_PARTS,        MVT::i32,   Expand);
  setOperationAction(ISD::SRL_PARTS,        MVT::i32,   Expand);
  setOperationAction(ISD::SIGN_EXTEND,      MVT::i32,   Expand);
  setOperationAction(ISD::ZERO_EXTEND,      MVT::i32,   Expand);

  setOperationAction(ISD::EXCEPTIONADDR,     MVT::i32, Expand);
  setOperationAction(ISD::EHSELECTION,       MVT::i32, Expand);

  setOperationAction(ISD::VAARG,             MVT::Other, Expand);
  setOperationAction(ISD::VACOPY,            MVT::Other, Expand);
  setOperationAction(ISD::VAEND,             MVT::Other, Expand);

  // Use the default for now
  setOperationAction(ISD::STACKSAVE,         MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE,      MVT::Other, Expand);

  //setOperationAction(ISD::MEMBARRIER,        MVT::Other, Custom);
  //setOperationAction(ISD::Constant,            MVT::i1, Promote); 
  setOperationAction(ISD::SIGN_EXTEND_INREG,  MVT::i1, Expand); 
  setOperationAction(ISD::ANY_EXTEND,         MVT::i1, Expand);
  //setOperationAction(ISD::TRUNCATE,           MVT::i1, Promote);

  setOperationAction(ISD::AND,                MVT::i1, Promote);
  setOperationAction(ISD::OR,                 MVT::i1, Promote);
  setOperationAction(ISD::ADD,                MVT::i1, Promote);
  setOperationAction(ISD::SUB,                MVT::i1, Promote);
  setOperationAction(ISD::XOR,                MVT::i1, Promote);
  setOperationAction(ISD::SHL,                MVT::i1, Promote);
  setOperationAction(ISD::SRA,                MVT::i1, Promote);
  setOperationAction(ISD::SRL,                MVT::i1, Promote);
  //setOperationAction(ISD::SETCC,              MVT::i1, Custom);
  //setOperationAction(ISD::SELECT,             MVT::i1, Promote);
  setOperationAction(ISD::SELECT_CC,          MVT::i1, Promote);
  //setOperationAction(ISD::CopyToReg,          MVT::i1, Promote);

  //setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Expand);
  //setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Expand);

  //setInsertFencesForAtomic(true);

  //setTargetDAGCombine(ISD::ADDE);
  //setTargetDAGCombine(ISD::SUBE);
  //setTargetDAGCombine(ISD::SDIVREM);
  //setTargetDAGCombine(ISD::UDIVREM);
  //setTargetDAGCombine(ISD::SETCC);
  //setTargetDAGCombine(ISD::AND);
  //setTargetDAGCombine(ISD::OR);
  //setOperationAction(ISD::EXTLOAD,    MVT::i8,    Custom);
  //setOperationAction(ISD::EXTLOAD,    MVT::i16,   Custom);
  //setOperationAction(ISD::SEXTLOAD,   MVT::i8,    Custom);
  //setOperationAction(ISD::SEXTLOAD,   MVT::i16,   Custom);
  //setOperationAction(ISD::ZEXTLOAD,   MVT::i8,    Custom);
  //setOperationAction(ISD::ZEXTLOAD,   MVT::i16,   Custom);

  setOperationAction(ISD::INTRINSIC_VOID, MVT::Other, Custom);
  setOperationAction(ISD::INTRINSIC_W_CHAIN, MVT::Other, Custom);
  setOperationAction(ISD::INTRINSIC_WO_CHAIN, MVT::Other, Custom);

  setOperationAction(ISD::FSQRT, MVT::f32, Expand);

  // FIXME don't know what this should be
  setMinFunctionAlignment(2);

  setStackPointerRegisterToSaveRestore(LE1::SP);
  computeRegisterProperties();

  //setExceptionPointerRegister(LE1::A0);
  //setExceptionSelectorRegister(LE1::A1);
}

EVT LE1TargetLowering::getSetCCResultType(EVT VT) const {
  if (!VT.isVector())
    return MVT::i1;
  else
    return VT;
}

SDValue LE1TargetLowering::
LowerOperation(SDValue Op, SelectionDAG &DAG) const
{
  switch (Op.getOpcode())
  {
    //case ISD::BRCOND:             return LowerBRCOND(Op, DAG);
    //case ISD::ConstantPool:       return LowerConstantPool(Op, DAG);
    //case ISD::DYNAMIC_STACKALLOC: return LowerDYNAMIC_STACKALLOC(Op, DAG);
    case ISD::GlobalAddress:      return LowerGlobalAddress(Op, DAG);
    //case ISD::BlockAddress:       return LowerBlockAddress(Op, DAG);
    //case ISD::GlobalTLSAddress:   return LowerGlobalTLSAddress(Op, DAG);
    //case ISD::JumpTable:          return LowerJumpTable(Op, DAG);
    //case ISD::SELECT:             return LowerSELECT(Op, DAG);
    case ISD::MULHS:              return LowerMULHS(Op, DAG);
    case ISD::MULHU:              return LowerMULHU(Op, DAG);
    case ISD::SDIV:               return LowerSDIV(Op, DAG);
    case ISD::UDIV:               return LowerUDIV(Op, DAG);
    case ISD::SREM:               return LowerSREM(Op, DAG);
    case ISD::UREM:               return LowerUREM(Op, DAG);
    case ISD::VASTART:            return LowerVASTART(Op, DAG);
    case ISD::SETCC:
    case ISD::SETNE:              return LowerSETCC(Op, DAG);
    //case ISD::EXTLOAD:
    //case ISD::ZEXTLOAD:
    //case ISD::SEXTLOAD:           return LowerLoad(Op, DAG);
    //case ISD::FCOPYSIGN:          return LowerFCOPYSIGN(Op, DAG);
    case ISD::FRAMEADDR:          return LowerFRAMEADDR(Op, DAG);
    //case ISD::MEMBARRIER:         return LowerMEMBARRIER(Op, DAG);
    //case ISD::ATOMIC_FENCE:       return LowerATOMIC_FENCE(Op, DAG);
    case ISD::INTRINSIC_VOID:
    case ISD::INTRINSIC_W_CHAIN:  return LowerIntrinsicWChain(Op, DAG);
    case ISD::INTRINSIC_WO_CHAIN: return LowerINTRINSIC_WO_CHAIN(Op, DAG);
  }
  return SDValue();
}

//===----------------------------------------------------------------------===//
//  Lower helper functions
//===----------------------------------------------------------------------===//

// AddLiveIn - This helper function adds the specified physical register to the
// MachineFunction as a live in value.  It also creates a corresponding
// virtual register for it.
static unsigned
AddLiveIn(MachineFunction &MF, unsigned PReg, const TargetRegisterClass *RC)
{
  assert(RC->contains(PReg) && "Not the correct regclass!");
  unsigned VReg = MF.getRegInfo().createVirtualRegister(RC);
  MF.getRegInfo().addLiveIn(PReg, VReg);
  return VReg;
}
/*
static bool SelectLoadLink(SDNode *CopyNode, SelectionDAG *CurDAG)
{
  if(CopyNode->getOperand(1) == CurDAG.getRegister(LE1::L0, MVT::i32))
    if(CopyNode->getOperand(2).getNode()->getOpcode() == ISD::LOAD) {
      CurDAG->getNode(LE1::LDL, dl, MVT::i32, MVT::Other, TargetAddr,
                      TargetConstantOff, Chain);
    }
  return false;
}*/

//===----------------------------------------------------------------------===//
//  Misc Lower Operation implementation
//===----------------------------------------------------------------------===//
//SDValue LE1TargetLowering::
//LowerADDC(SDValue Op, SelectionDAG &DAG) const
//{
  //DebugLoc dl = Op.getDebugLoc();
  //SDValue LHS = Op.getOperand(0);
  //SDValue RHS = Op.getOperand(1);
  //SDValue Zero = DAG.getRegister(LE1::ZERO, MVT::i32);
  
//SDValue LE1TargetLowering::
//LowerADDE(SDValue Op, SelectionDAG &DAG) const
//{

//}
SDValue LE1TargetLowering::
LowerMULHS(SDValue Op, SelectionDAG &DAG) const
{
  // Taken from 'A Hacker's Delight'
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue ShiftImm = DAG.getTargetConstant(16, MVT::i32);
  SDValue MaskImm = DAG.getTargetConstant(0xFFFF, MVT::i32);

  SDValue v0, v1;

  // Check whether the second operand is an immediate value
  if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(RHS)) {
    // If it's large, move it to a register and then modify
    if(C->getConstantIntValue()->getBitWidth() > 16) {
      SDValue v = DAG.getNode(LE1ISD::Mov, dl, MVT::i32, RHS);
      v0 = DAG.getNode(ISD::AND, dl, MVT::i32, RHS, MaskImm);
      v1 = DAG.getNode(ISD::SRA, dl, MVT::i32, RHS, ShiftImm);
    }
  }
  else {
    v0 = DAG.getNode(ISD::AND, dl, MVT::i32, RHS, MaskImm);
    v1 = DAG.getNode(ISD::SRA, dl, MVT::i32, RHS, ShiftImm);
  }

  SDValue u0 = DAG.getNode(ISD::AND, dl, MVT::i32, LHS, MaskImm);
  SDValue u1 = DAG.getNode(ISD::SRA, dl, MVT::i32, LHS, ShiftImm);

  SDValue w0 = DAG.getNode(ISD::MUL, dl, MVT::i32, u0, v0);
  SDValue t1 = DAG.getNode(ISD::MUL, dl, MVT::i32, u1, v0);
  SDValue t2 = DAG.getNode(ISD::SRL, dl, MVT::i32, w0, ShiftImm);
  SDValue t = DAG.getNode(ISD::ADD, dl, MVT::i32, t1, t2);

  SDValue w1 = DAG.getNode(ISD::AND, dl, MVT::i32, t, MaskImm);
  SDValue w2 = DAG.getNode(ISD::SRA, dl, MVT::i32, t, ShiftImm);

  SDValue w3 = DAG.getNode(ISD::MUL, dl, MVT::i32, u0, v1);
  SDValue w4 = DAG.getNode(ISD::ADD, dl, MVT::i32, w3, w1);

  SDValue u1v1 = DAG.getNode(ISD::MUL, dl, MVT::i32, u1, v1);
  SDValue w4Shift = DAG.getNode(ISD::SRA, dl, MVT::i32, w4, ShiftImm);
  SDValue resA = DAG.getNode(ISD::ADD, dl, MVT::i32, u1v1, w4Shift);
  return DAG.getNode(ISD::ADD, dl, MVT::i32, resA, w2);
}

SDValue LE1TargetLowering::
LowerMULHU(SDValue Op, SelectionDAG &DAG) const
{
  // Taken from 'A Hacker's Delight'
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue ShiftImm = DAG.getTargetConstant(16, MVT::i32);
  SDValue MaskImm = DAG.getTargetConstant(0xFFFF, MVT::i32);

  SDValue v0, v1;

  // Check whether the second operand is an immediate value
  if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(RHS)) {
    // If it's large, move it to a register and then modify
    if(C->getConstantIntValue()->getBitWidth() > 16) {
      SDValue v = DAG.getNode(LE1ISD::Mov, dl, MVT::i32, RHS);
      v0 = DAG.getNode(ISD::AND, dl, MVT::i32, RHS, MaskImm);
      v1 = DAG.getNode(ISD::SRL, dl, MVT::i32, RHS, ShiftImm);
    }
  }
  else {
    v0 = DAG.getNode(ISD::AND, dl, MVT::i32, RHS, MaskImm);
    v1 = DAG.getNode(ISD::SRA, dl, MVT::i32, RHS, ShiftImm);
  }

  SDValue u0 = DAG.getNode(ISD::AND, dl, MVT::i32, LHS, MaskImm);
  SDValue u1 = DAG.getNode(ISD::SRL, dl, MVT::i32, LHS, ShiftImm);

  SDValue w0 = DAG.getNode(ISD::MUL, dl, MVT::i32, u0, v0);
  SDValue t1 = DAG.getNode(ISD::MUL, dl, MVT::i32, u1, v0);
  SDValue t2 = DAG.getNode(ISD::SRL, dl, MVT::i32, w0, ShiftImm);
  SDValue t = DAG.getNode(ISD::ADD, dl, MVT::i32, t1, t2);

  SDValue w1 = DAG.getNode(ISD::AND, dl, MVT::i32, t, MaskImm);
  SDValue w2 = DAG.getNode(ISD::SRL, dl, MVT::i32, t, ShiftImm);

  SDValue w3 = DAG.getNode(ISD::MUL, dl, MVT::i32, u0, v1);
  SDValue w4 = DAG.getNode(ISD::ADD, dl, MVT::i32, w3, w1);

  SDValue u1v1 = DAG.getNode(ISD::MUL, dl, MVT::i32, u1, v1);
  SDValue w4Shift = DAG.getNode(ISD::SRL, dl, MVT::i32, w4, ShiftImm);
  SDValue resA = DAG.getNode(ISD::ADD, dl, MVT::i32, u1v1, w4Shift);
  return DAG.getNode(ISD::ADD, dl, MVT::i32, resA, w2);

  /*
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue ShiftImm = DAG.getTargetConstant(16, MVT::i32);
  SDValue Zero = DAG.getRegister(LE1::ZERO, MVT::i32);

  SDValue Mullhu0 = DAG.getNode(LE1ISD::Mullhu, dl, MVT::i32, LHS, RHS);
  SDValue Mullhu1 = DAG.getNode(LE1ISD::Mullhu, dl, MVT::i32, RHS, LHS);
  SDValue Mulllu0 = DAG.getNode(LE1ISD::Mulllu, dl, MVT::i32, LHS, RHS);
  SDValue Mulhhu0 = DAG.getNode(LE1ISD::Mulhhu, dl, MVT::i32, LHS, RHS);

  SDValue Shru0 = DAG.getNode(ISD::SRL, dl, MVT::i32, Mullhu0, ShiftImm);
  SDValue Shru1 = DAG.getNode(ISD::SRL, dl, MVT::i32, Mullhu1, ShiftImm);

  SDValue Add0 = DAG.getNode(ISD::ADD, dl, MVT::i32, Shru0, Shru1);
  SDValue Add1 = DAG.getNode(ISD::ADD, dl, MVT::i32, Mulhhu0, Add0);
  // Add1 now holds the current top 32 bits, now just have to calculate any
  // bits carried from the additions of the lower 32 bits

  SDValue MTB0 = DAG.getNode(LE1ISD::MTB, dl, MVT::i1, Zero);
  SDValue Addcg0 = DAG.getNode(LE1ISD::Addcg, dl,
                               DAG.getVTList(MVT::i32, MVT::i1),
                               Mulllu0, Mullhu0, MTB0);
  SDValue Cout0(Addcg0.getNode(), 1);

  SDValue Addcg1 = DAG.getNode(LE1ISD::Addcg, dl,
                               DAG.getVTList(MVT::i32, MVT::i1),
                               Addcg0, Mullhu1, Cout0);
  SDValue Cout1(Addcg1.getNode(), 1);

  SDValue Add2 = DAG.getNode(ISD::ADD, dl, MVT::i32, Add1,
                             DAG.getTargetConstant(1, MVT::i32));

  return DAG.getNode(ISD::SELECT, dl, MVT::i32, Cout1,
                                Add2, Add1);*/
}

SDValue LE1TargetLowering::
LowerSDIV(SDValue Op, SelectionDAG &DAG) const
{
  //std::cout << "LowerSDIV\n";
  assert((Op.getValueType() == MVT::i32) && "32-bit division only!");
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue Zero = DAG.getRegister(LE1::ZERO, MVT::i32);

  // Check whether the polarity of the values. If the argument is negative,
  // use its absolute value.
  //SDValue Cmplt0 = DAG.getNode(LE1ISD::Cmplt, dl, MVT::i32, LHS, Zero);
  SDValue Cmplt0 = DAG.getSetCC(dl, MVT::i32, LHS, Zero, ISD::SETLT);
  SDValue Sub0 = DAG.getNode(ISD::SUB, dl, MVT::i32, Zero, LHS);
  //SDValue MTB1 = DAG.getNode(LE1ISD::MTB, dl, MVT::i1, Cmplt0);
  SDValue Slct0 = DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmplt0, Sub0, LHS);

  //SDValue Cmplt1 = DAG.getNode(LE1ISD::Cmplt, dl, MVT::i32, RHS, Zero);
  SDValue Cmplt1 = DAG.getSetCC(dl, MVT::i32, RHS, Zero, ISD::SETLT);
  SDValue Sub1 = DAG.getNode(ISD::SUB, dl, MVT::i32, Zero, RHS);
  //SDValue MTB2 = DAG.getNode(LE1ISD::MFB, dl, MVT::i1, Cmplt1);
  SDValue Slct1 = DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmplt1, Sub1, RHS);

  // Check whether the polarities are the same
  //SDValue Cmpeq0 = DAG.getNode(LE1ISD::Cmpeq, dl, MVT::i1, Cmplt1, Cmplt0);
  SDValue Cmpeq0 = DAG.getSetCC(dl, MVT::i1, Cmplt1, Cmplt0, ISD::SETEQ);

  // Begin the division process
  SDValue MTB0 = DAG.getNode(LE1ISD::MTB, dl, MVT::i1, Zero);
  SDValue Addcg0 = DAG.getNode(LE1ISD::Addcg, dl, 
                               DAG.getVTList(MVT::i32, MVT::i1),
                               Slct0, Slct0, MTB0);
  SDValue AddCout(Addcg0.getNode(), 1);
  SDValue Res = DivStep(Zero, Slct1, AddCout, Addcg0, MTB0, DAG);

  // If the polarities were the same, return result otherwise return the
  // corrected value using Sub2
  SDValue Sub2 = DAG.getNode(ISD::SUB, dl, MVT::i32, Zero, Res);
  return DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmpeq0, Res, Sub2);
}

SDValue LE1TargetLowering::
LowerSREM(SDValue Op, SelectionDAG &DAG) const
{
  //std::cout << "LowerSREM\n";
  assert((Op.getValueType() == MVT::i32) && "32-bit remainder only!");
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue Zero = DAG.getRegister(LE1::ZERO, MVT::i32);

  //SDValue Cmplt0 = DAG.getNode(LE1ISD::Cmplt, dl, MVT::i1, LHS, Zero);
  SDValue Cmplt0 = DAG.getSetCC(dl, MVT::i1, LHS, Zero, ISD::SETLT);
  SDValue Sub0 = DAG.getNode(ISD::SUB, dl, MVT::i32, Zero, LHS);
  SDValue Slct0 = DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmplt0, Sub0, LHS);

  //SDValue Cmplt1 = DAG.getNode(LE1ISD::Cmplt, dl, MVT::i1, RHS, Zero);
  SDValue Cmplt1 = DAG.getSetCC(dl, MVT::i1, RHS, Zero, ISD::SETLT);
  SDValue Sub1 = DAG.getNode(ISD::SUB, dl, MVT::i32, Zero, RHS);
  SDValue Slct1 = DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmplt1, Sub1, RHS);

  SDValue MTB0 = DAG.getNode(LE1ISD::MTB, dl, MVT::i1, Zero);
  SDValue Addcg0 = DAG.getNode(LE1ISD::Addcg, dl, 
                               DAG.getVTList(MVT::i32, MVT::i1),
                               Slct0, Slct0, MTB0);
  SDValue AddCout(Addcg0.getNode(), 1);

  // Perform the addcg/divs operations
  SDValue Res = RemStep(Zero, Slct1, AddCout, Addcg0, MTB0, DAG);
  //SDValue ResCout(Res.getNode(), 1);

  //SDValue Cmpge0 = DAG.getNode(LE1ISD::Cmpge, dl, MVT::i1, Res, Zero);
  SDValue Cmpge0 = DAG.getSetCC(dl, MVT::i1, Res, Zero, ISD::SETGE);
  SDValue Add0 = DAG.getNode(ISD::ADD, dl, MVT::i32, Res, Slct1);
  SDValue Slct2 = DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmpge0, Res, Add0);
  SDValue Sub2 = DAG.getNode(ISD::SUB, dl, MVT::i32, Zero, Slct2);
  return DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmplt0, Sub2, Slct2); 

}

SDValue LE1TargetLowering::
LowerUDIV(SDValue Op, SelectionDAG &DAG) const
{
  //std::cout << "LowerUDIV\n";
  assert((Op.getValueType() == MVT::i32) && "32-bit division only!");
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue Zero = DAG.getRegister(LE1::ZERO, MVT::i32);

  //SDValue Cmplt = DAG.getNode(LE1ISD::Cmplt, dl, MVT::i32, RHS, Zero);
  SDValue CMPLT = DAG.getSetCC(dl, MVT::i32, RHS, Zero, ISD::SETLT);
  SDValue MTB0 = DAG.getNode(LE1ISD::MTB, dl, MVT::i1, Zero);
  //SDValue ShiftBit = DAG.getNode(LE1ISD::MFB, dl, MVT::i32, Cmplt);
  SDValue Shru0 = DAG.getNode(ISD::SRA, dl, MVT::i32, RHS, CMPLT);
  SDValue Shru1 = DAG.getNode(ISD::SRA, dl, MVT::i32, LHS, CMPLT);

  //SDValue Cmpgeu = DAG.getNode(LE1ISD::Cmpgeu, dl, MVT::i1, LHS, RHS);
  SDValue CMPGEU = DAG.getSetCC(dl, MVT::i1, LHS, RHS, ISD::SETUGE);

  SDValue Addcg = DAG.getNode(LE1ISD::Addcg, dl,
                             DAG.getVTList(MVT::i32, MVT::i1),
                             Shru1, Shru1, MTB0);
  //std::cout << "ADD0\n";
  SDValue CarryA(Addcg.getNode(), 1);
  SDValue Res = DivStep(Zero, Shru0, CarryA, Addcg, MTB0, DAG);
  //SDValue MTB1 = DAG.getNode(LE1ISD::MTB, dl, MVT::i1, CMPLT);
  SDValue MFB0 = DAG.getNode(LE1ISD::MFB, dl, MVT::i32, CMPGEU);
  return DAG.getNode(ISD::SELECT, dl, MVT::i32, CMPLT, MFB0, Res);
  //return DAG.getNode(ISD::SELECT, dl, MVT::i32, MTB1, Cmpgeu, Res);

}

SDValue LE1TargetLowering::
LowerUREM(SDValue Op, SelectionDAG &DAG) const
{
  //std::cout << "LowerUREM\n";
  assert((Op.getValueType() == MVT::i32) && "32-bit remainder only!");
  DebugLoc dl = Op.getDebugLoc();
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue Zero = DAG.getRegister(LE1::ZERO, MVT::i32);

  //SDValue Cmplt0 = DAG.getNode(LE1ISD::Cmplt, dl, MVT::i32, RHS, Zero);
  SDValue Cmplt0 = DAG.getSetCC(dl, MVT::i32, RHS, Zero, ISD::SETLT);
  SDValue MTB0 = DAG.getNode(LE1ISD::MTB, dl, MVT::i1, Zero);
  //SDValue MTB1 = DAG.getNode(LE1ISD::MFB, dl, MVT::i32, Cmplt0);

  SDValue Shru0 = DAG.getNode(ISD::SRA, dl, MVT::i32, RHS, Cmplt0);
  SDValue Shru1 = DAG.getNode(ISD::SRA, dl, MVT::i32, LHS, Cmplt0);
  //SDValue Cmpgeu0 = DAG.getNode(LE1ISD::Cmpgeu, dl, MVT::i1, LHS, RHS);
  SDValue Cmpgeu0 = DAG.getSetCC(dl, MVT::i1, LHS, RHS, ISD::SETUGE);
  // FIXME unused value?!
  SDValue Sub0 = DAG.getNode(ISD::SUB, dl, MVT::i32, LHS, RHS);

  SDValue Addcg0 = DAG.getNode(LE1ISD::Addcg, dl,
                               DAG.getVTList(MVT::i32, MVT::i1),
                               Shru1, Shru1, MTB0);
  SDValue AddCout(Addcg0.getNode(),1);

  SDValue Slct0 = DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmpgeu0, LHS, RHS);
  SDValue Res = RemStep(Zero, Shru0, AddCout, Addcg0, MTB0, DAG);

  //SDValue Cmpge0 = DAG.getNode(LE1ISD::Cmpge, dl, MVT::i1, Res, Zero);
  SDValue Cmpge0 = DAG.getSetCC(dl, MVT::i1, Res, Zero, ISD::SETGE);
  SDValue Add0 = DAG.getNode(ISD::ADD, dl, MVT::i32, Res, Shru0);
  SDValue Slct1 = DAG.getNode(ISD::SELECT, dl, MVT::i32, Cmpge0, Res, Add0);
  SDValue MTB1 = DAG.getNode(LE1ISD::MTB, dl, MVT::i1, Cmplt0);
  return DAG.getNode(ISD::SELECT, dl, MVT::i32, MTB1, Slct0, Slct1);

}

SDValue LE1TargetLowering::
RemStep(SDValue DivArg1, SDValue DivArg2, SDValue DivCin, SDValue AddArg,
        SDValue AddCin, SelectionDAG &DAG) const {
  // Need to do this 31 times
  static int stepCount = 0;
  // FIXME no DebugLoc
  DebugLoc dl;

  SDValue DivRes = DAG.getNode(LE1ISD::Divs, dl,
                               DAG.getVTList(MVT::i32, MVT::i1),
                               DivArg1, DivArg2, DivCin);
  SDValue DivCout(DivRes.getNode(), 1);
  
  SDValue AddRes = DAG.getNode(LE1ISD::Addcg, dl,
                               DAG.getVTList(MVT::i32, MVT::i1),
                               AddArg, AddArg, AddCin);
  SDValue AddCout(AddRes.getNode(), 1);

  ++stepCount;
  if(stepCount < 31)
    return RemStep(DivRes, DivArg2, AddCout, AddRes, DivCout, DAG);
  else {
    stepCount = 0;
    return DAG.getNode(LE1ISD::Divs, dl,
                       DAG.getVTList(MVT::i32, MVT::i1),
                       DivRes, DivArg2, AddCout);
  }
}

SDValue LE1TargetLowering::
DivStep(SDValue DivArg1, SDValue DivArg2, SDValue DivCin, SDValue AddArg,
        SDValue AddCin, SelectionDAG &DAG) const {
  // Need to do this 32 times
  static int stepCount = 0;
  //FIXME no DebugLoc
  DebugLoc dl;

  SDValue DivRes = DAG.getNode(LE1ISD::Divs, dl,
                               DAG.getVTList(MVT::i32, MVT::i1),
                               DivArg1, DivArg2, DivCin);
  SDValue DivCout(DivRes.getNode(), 1);
  
  SDValue AddRes = DAG.getNode(LE1ISD::Addcg, dl,
                               DAG.getVTList(MVT::i32, MVT::i1),
                               AddArg, AddArg, AddCin);
  SDValue AddCout(AddRes.getNode(), 1);

  ++stepCount;
  if(stepCount < 32)
    return DivStep(DivRes, DivArg2, AddCout, AddRes, DivCout, DAG);
  else {
    stepCount = 0;
    SDValue Zero = DAG.getRegister(LE1::ZERO, MVT::i32);
    SDValue Final = DAG.getNode(LE1ISD::Addcg, dl,
                                DAG.getVTList(MVT::i32, MVT::i1),
                                AddRes, AddRes, DivCout);
    //SDValue Cmpge = DAG.getNode(LE1ISD::Cmpge, dl, MVT::i1, DivRes, Zero);
    SDValue Cmpge = DAG.getSetCC(dl, MVT::i1, DivRes, Zero, ISD::SETGE);
    //SDValue Cmpge = DAG.getNode(ISD::SETCC, dl, MVT::i1, DivRes, Zero,
      //                          DAG.getConstant(ISD::SETGE, MVT::i32));
    SDValue ORC = DAG.getNode(LE1ISD::Orc, dl, MVT::i32, Final, Zero);
    SDValue MFB = DAG.getNode(LE1ISD::MFB, dl, MVT::i32, Cmpge);
    return DAG.getNode(LE1ISD::Sh1Add, dl, MVT::i32, ORC, MFB);
    // return AddRes;
  }
}

SDValue LE1TargetLowering::LowerSETCC(SDValue Op,
                                      SelectionDAG &DAG) const {
  std::cerr << "LowerSETCC\n";
  SDValue LHS = Op.getOperand(1);
  SDValue RHS = Op.getOperand(2);
  DebugLoc dl = Op.getDebugLoc();
  SDNode* Node = Op.getNode();
  ISD::CondCode CC = cast<CondCodeSDNode>(Node->getOperand(2))->get();

  /*
  // XOR on vector and scalar input
  if (!LHS.getValueType().isVector() != !RHS.getValueType().isVector()) {
    SDValue vector;
    SDValue scalar;
    if (LHS.getValueType().isVector()) {
      vector = LHS;
      scalar = RHS;
    }
    else {
      vector = RHS;
      scalar = LHS;
    }
    EVT vectorType = vector.getValueType();
    unsigned numElements = vectorType.getVectorNumElements();
    SDValue *newVector = new SDValue[numElements];
    for (unsigned i = 0; i < numElements; ++i)
      newVector[i] = scalar;
    SDValue buildVector = DAG.getNode(ISD::BUILD_VECTOR, dl, vectorType,
                                      newVector, numElements);

    return DAG.getSetCC(dl, MVT::i1, buildVector, vector, CC);
  }*/

  SDValue LHSz = DAG.getNode(ISD::ZERO_EXTEND, dl, EVT(MVT::i32), LHS);
  SDValue RHSz = DAG.getNode(ISD::ZERO_EXTEND, dl, EVT(MVT::i32), RHS);

  return DAG.getSetCC(dl, MVT::i1, LHSz, RHSz, CC);
}

SDValue LE1TargetLowering::LowerINTRINSIC_WO_CHAIN(SDValue Op,
                                                   SelectionDAG &DAG) const {
  //DEBUG(dbgs() << "LowerINTRINISIC_WO_CHAIN\n");
  DebugLoc dl = Op.getDebugLoc();
  unsigned IntNo = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();

  // Create a GlobalAddressSDNode with a GlobalValue
  // Then get the address:
  // DAG.getTargetGlobalAddress(GV, dl, getPointerTy(), G->getOffset(), Flags);
  // Then create a load from that address.

  // Most of the intrinsics rely on CPU Id
  SDValue CPUId = DAG.getNode(LE1ISD::CPUID, dl, MVT::i32);
  CPUId = DAG.getNode(ISD::SRL, dl, MVT::i32, CPUId,
                      DAG.getConstant(8, MVT::i32));

  switch(IntNo) {
  case Intrinsic::le1_read_cpuid: {
    return CPUId;
  }
  case Intrinsic::le1_read_group_id_0: {
    SDValue Index = DAG.getNode(ISD::MUL, dl, MVT::i32, CPUId,
                                DAG.getTargetConstant(4, MVT::i32));
    SDValue GroupAddr = DAG.getNode(LE1ISD::GROUP_ID_ADDR, dl, MVT::i32);
    GroupAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, Index, GroupAddr);
    return DAG.getNode(LE1ISD::READ_ATTR, dl, MVT::i32, GroupAddr);
  }
  case Intrinsic::le1_read_group_id_1: {
    SDValue Index = DAG.getNode(ISD::MUL, dl, MVT::i32, CPUId,
                                DAG.getTargetConstant(4, MVT::i32));
    Index = DAG.getNode(ISD::ADD, dl, MVT::i32, Index,
                        DAG.getConstant(1, MVT::i32));
    SDValue GroupAddr = DAG.getNode(LE1ISD::GROUP_ID_ADDR, dl, MVT::i32);
    GroupAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, Index, GroupAddr);
    return DAG.getNode(LE1ISD::READ_ATTR, dl, MVT::i32, GroupAddr);
  }
  case Intrinsic::le1_read_group_id_2: {
    SDValue Index = DAG.getNode(ISD::MUL, dl, MVT::i32, CPUId,
                                DAG.getConstant(4, MVT::i32));
    Index = DAG.getNode(ISD::ADD, dl, MVT::i32, Index,
                        DAG.getConstant(2, MVT::i32));
    SDValue GroupAddr = DAG.getNode(LE1ISD::GROUP_ID_ADDR, dl, MVT::i32);
    GroupAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, Index, GroupAddr);
    return DAG.getNode(LE1ISD::READ_ATTR, dl, MVT::i32, GroupAddr);
  }
  default:
    return SDValue();
    break;
  }

}

SDValue LE1TargetLowering::LowerIntrinsicWChain(SDValue Op,
                                                 SelectionDAG &DAG) const {
  DebugLoc dl = Op.getDebugLoc();
  unsigned IntNo = cast<ConstantSDNode>(Op.getOperand(1))->getZExtValue();
  SDValue Chain = Op.getOperand(0);
  SDValue CPUId = DAG.getNode(LE1ISD::CPUID, dl, MVT::i32);
  CPUId = DAG.getNode(ISD::SRL, dl, MVT::i32, CPUId,
                      DAG.getConstant(8, MVT::i32));


  // get_global_id = group_id * local_size * local_id
  // group_id = cpuid
  // local_size
  // local_id
  SDValue Result;

  switch(IntNo) {
  case Intrinsic::le1_set_group_id_0: {
    SDValue GroupId = Op.getOperand(2);
    SDValue Index = DAG.getNode(ISD::MUL, dl, MVT::i32, CPUId,
                                DAG.getTargetConstant(4, MVT::i32));
    //Index = DAG.getNode(ISD::ADD, dl, MVT::i32, Index,
      //                  DAG.getConstant(0, MVT::i32));
    SDValue GroupAddr = DAG.getNode(LE1ISD::GROUP_ID_ADDR, dl, MVT::i32);
    GroupAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, Index, GroupAddr);
    //SDValue NumCores = DAG.getNode(LE1ISD::NUM_CORES, dl, MVT::i32);
    //SDValue Workgroup = DAG.getNode(ISD::MUL, dl, MVT::i32, NumCores, GroupId);
    //Workgroup = DAG.getNode(ISD::ADD, dl, MVT::i32, Workgroup, CPUId);
    Result = DAG.getNode(LE1ISD::SET_ATTR, dl, MVT::Other, Chain,
                         GroupId, GroupAddr);
    break;
  }
  case Intrinsic::le1_set_group_id_1: {
    SDValue GroupId = Op.getOperand(2);
    SDValue Index = DAG.getNode(ISD::MUL, dl, MVT::i32, CPUId,
                                DAG.getTargetConstant(4, MVT::i32));
    Index = DAG.getNode(ISD::ADD, dl, MVT::i32, Index,
                        DAG.getConstant(1, MVT::i32));
    SDValue GroupAddr = DAG.getNode(LE1ISD::GROUP_ID_ADDR, dl, MVT::i32);
    GroupAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, Index, GroupAddr);

    //SDValue NumCores = DAG.getNode(LE1ISD::NUM_CORES, dl, MVT::i32);
    //SDValue Workgroup = DAG.getNode(ISD::MUL, dl, MVT::i32, NumCores, GroupId);
    //Workgroup = DAG.getNode(ISD::ADD, dl, MVT::i32, Workgroup, CPUId);
    Result = DAG.getNode(LE1ISD::SET_ATTR, dl, MVT::Other, Chain,
                         GroupId, GroupAddr);
    break;
  }
  case Intrinsic::le1_set_group_id_2: {
    SDValue GroupId = Op.getOperand(2);
    SDValue Index = DAG.getNode(ISD::MUL, dl, MVT::i32, CPUId,
                                DAG.getTargetConstant(4, MVT::i32));
    Index = DAG.getNode(ISD::ADD, dl, MVT::i32, Index,
                        DAG.getConstant(2, MVT::i32));
    SDValue GroupAddr = DAG.getNode(LE1ISD::GROUP_ID_ADDR, dl, MVT::i32);
    GroupAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, Index, GroupAddr);
    //SDValue NumCores = DAG.getNode(LE1ISD::NUM_CORES, dl, MVT::i32);
    //SDValue Workgroup = DAG.getNode(ISD::MUL, dl, MVT::i32, NumCores, GroupId);
    //Workgroup = DAG.getNode(ISD::ADD, dl, MVT::i32, Workgroup, CPUId);
    Result = DAG.getNode(LE1ISD::SET_ATTR, dl, MVT::Other, Chain,
                         GroupId, GroupAddr);
    break;
  }
  default:
    llvm_unreachable("unhandled intrinsic with chain!");
    break;
  }
  //DAG.ReplaceAllUsesWith(Op, Result);
  return Result;
}


SDValue LE1TargetLowering::LowerGlobalAddress(SDValue Op,
                                               SelectionDAG &DAG) const {
  //std::cout << "Entering LE1TargetLowering::LowerGlobalAddress\n";
  // FIXME there isn't actually debug info here
  //std::cout << "LowerGlobal, Opcode = " << Op.getOpcode() << std::endl;
  DebugLoc dl = Op.getDebugLoc();
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();
  EVT ValTy = Op.getValueType();
  SDValue GA = DAG.getTargetGlobalAddress(GV, dl, ValTy, Offset);

  //LE1TargetObjectFile &TLOF =
    //(LE1TargetObjectFile&)getObjFileLowering();
  //if(TLOF.IsGlobalInSmallSection(GV, getTargetMachine())) {
    //std::cout << "isSmallSection\n";
    return DAG.getNode(LE1ISD::TargetGlobal, dl, ValTy, GA);
  //}
  //else std::cout << "!isSmallSection\n";
  return DAG.getNode(LE1ISD::TargetGlobalConst, dl, ValTy, GA);

  return DAG.getNode(LE1ISD::Mov, dl, MVT::i32, GA);

  // Otherwise, just return the address
  return GA;

}

SDValue LE1TargetLowering::LowerLoad(SDValue Op, SelectionDAG &DAG) const {
  /*DebugLoc dl = Op.getDebugLoc();
  LoadSDNode *LoadNode = cast<LoadSDNode>(Op.getNode());
  SDValue Chain = LoadNode->getChain();
  EVT VT = Op.getValueType();
  //EVT VT = LoadNode->getMemoryVT();

  // Only lower load ops that are directly using a global address
  if(Op.getOperand(1)->getOpcode() == LE1ISD::TargetGlobal) {

    SDValue TargetAddr = Op.getOperand(1);
    SDValue TargetOffset = Op.getOperand(2);
    unsigned Opcode = 0;

    switch(Op.getOpcode()) {
    case ISD::EXTLOAD:
    case ISD::ZEXTLOAD:
      Opcode = (VT == MVT::i8) ? LE1ISD::LoadGlobalU8 : LE1ISD::LoadGlobalU16;
      break;
    case ISD::SEXTLOAD:
      Opcode = (VT == MVT::i8) ? LE1ISD::LoadGlobalS8 : LE1ISD::LoadGlobalS16;
      break;
    }
    return DAG.getNode(Opcode, dl, VT, MVT::Other, TargetAddr, TargetOffset,
                       Chain);
  }
  else*/
    return Op;
}

/*
SDValue LE1TargetLowering::LowerBlockAddress(SDValue Op,
                                              SelectionDAG &DAG) const {
  const BlockAddress *BA = cast<BlockAddressSDNode>(Op)->getBlockAddress();
  // FIXME there isn't actually debug info here
  // FIXME most of the function doesn't look like it relates to LE1
  DebugLoc dl = Op.getDebugLoc();

  if (getTargetMachine().getRelocationModel() != Reloc::PIC_) {
    // %hi/%lo relocation
    SDValue BAHi = DAG.getBlockAddress(BA, MVT::i32, true,
                                       LE1II::MO_ABS_HI);
    SDValue BALo = DAG.getBlockAddress(BA, MVT::i32, true,
                                       LE1II::MO_ABS_LO);
    SDValue Hi = DAG.getNode(LE1ISD::Hi, dl, MVT::i32, BAHi);
    SDValue Lo = DAG.getNode(LE1ISD::Lo, dl, MVT::i32, BALo);
    return DAG.getNode(ISD::ADD, dl, MVT::i32, Hi, Lo);
  }

  EVT ValTy = Op.getValueType();
  SDValue BAGOTOffset = DAG.getBlockAddress(BA, ValTy, true,
                                            LE1II::MO_GOT);
  BAGOTOffset = DAG.getNode(LE1ISD::WrapperPIC, dl, ValTy, BAGOTOffset);
  SDValue BALOOffset = DAG.getBlockAddress(BA, ValTy, true,
                                           LE1II::MO_ABS_LO);
  SDValue Load = DAG.getLoad(ValTy, dl, DAG.getEntryNode(), BAGOTOffset,
                             MachinePointerInfo(), false, false, false, 0);
  SDValue Lo = DAG.getNode(LE1ISD::Lo, dl, ValTy, BALOOffset);
  return DAG.getNode(ISD::ADD, dl, ValTy, Load, Lo);
}*/

/*
SDValue LE1TargetLowering::
LowerJumpTable(SDValue Op, SelectionDAG &DAG) const
{
  SDValue ResNode;
  SDValue HiPart;
  // FIXME there isn't actually debug info here
  DebugLoc dl = Op.getDebugLoc();
  EVT ValTy = Op.getValueType();
  bool IsPIC = getTargetMachine().getRelocationModel() == Reloc::PIC_;
  unsigned char OpFlag = IsPIC ? LE1II::MO_GOT : LE1II::MO_ABS_HI;

  EVT PtrVT = Op.getValueType();
  JumpTableSDNode *JT  = cast<JumpTableSDNode>(Op);

  SDValue JTI = DAG.getTargetJumpTable(JT->getIndex(), PtrVT, OpFlag);

  if (!IsPIC) {
    SDValue Ops[] = { JTI };
    HiPart = DAG.getNode(LE1ISD::Hi, dl, DAG.getVTList(MVT::i32), Ops, 1);
  } else {// Emit Load from Global Pointer
    JTI = DAG.getNode(LE1ISD::WrapperPIC, dl, MVT::i32, JTI);
    HiPart = DAG.getLoad(ValTy, dl, DAG.getEntryNode(), JTI,
                         MachinePointerInfo(),
                         false, false, false, 0);
  }

  SDValue JTILo = DAG.getTargetJumpTable(JT->getIndex(), PtrVT,
                                         LE1II::MO_ABS_LO);
  SDValue Lo = DAG.getNode(LE1ISD::Lo, dl, MVT::i32, JTILo);
  ResNode = DAG.getNode(ISD::ADD, dl, MVT::i32, HiPart, Lo);

  return ResNode;
}*/
/*
SDValue LE1TargetLowering::
LowerConstantPool(SDValue Op, SelectionDAG &DAG) const
{
  SDValue ResNode;
  ConstantPoolSDNode *N = cast<ConstantPoolSDNode>(Op);
  const Constant *C = N->getConstVal();
  // FIXME there isn't actually debug info here
  DebugLoc dl = Op.getDebugLoc();
  EVT ValTy = Op.getValueType();

  // gp_rel relocation
  // FIXME: we should reference the constant pool using small data sections,
  // but the asm printer currently doesn't support this feature without
  // hacking it. This feature should come soon so we can uncomment the
  // stuff below.
  //if (IsInSmallSection(C->getType())) {
  //  SDValue GPRelNode = DAG.getNode(LE1ISD::GPRel, MVT::i32, CP);
  //  SDValue GOT = DAG.getGLOBAL_OFFSET_TABLE(MVT::i32);
  //  ResNode = DAG.getNode(ISD::ADD, MVT::i32, GOT, GPRelNode);

  if (getTargetMachine().getRelocationModel() != Reloc::PIC_) {
    SDValue CPHi = DAG.getTargetConstantPool(C, MVT::i32, N->getAlignment(),
                                             N->getOffset(), LE1II::MO_ABS_HI);
    SDValue CPLo = DAG.getTargetConstantPool(C, MVT::i32, N->getAlignment(),
                                             N->getOffset(), LE1II::MO_ABS_LO);
    SDValue HiPart = DAG.getNode(LE1ISD::Hi, dl, MVT::i32, CPHi);
    SDValue Lo = DAG.getNode(LE1ISD::Lo, dl, MVT::i32, CPLo);
    ResNode = DAG.getNode(ISD::ADD, dl, MVT::i32, HiPart, Lo);
  } else {
    SDValue CP = DAG.getTargetConstantPool(C, MVT::i32, N->getAlignment(),
                                           N->getOffset(), LE1II::MO_GOT);
    CP = DAG.getNode(LE1ISD::WrapperPIC, dl, MVT::i32, CP);
    SDValue Load = DAG.getLoad(ValTy, dl, DAG.getEntryNode(),
                               CP, MachinePointerInfo::getConstantPool(),
                               false, false, false, 0);
    SDValue CPLo = DAG.getTargetConstantPool(C, MVT::i32, N->getAlignment(),
                                             N->getOffset(), LE1II::MO_ABS_LO);
    SDValue Lo = DAG.getNode(LE1ISD::Lo, dl, MVT::i32, CPLo);
    ResNode = DAG.getNode(ISD::ADD, dl, MVT::i32, Load, Lo);
  }

  return ResNode;
}*/

SDValue LE1TargetLowering::LowerVASTART(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  LE1FunctionInfo *FuncInfo = MF.getInfo<LE1FunctionInfo>();

  DebugLoc dl = Op.getDebugLoc();
  SDValue FI = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(),
                                 getPointerTy());

  // vastart just stores the address of the VarArgsFrameIndex slot into the
  // memory location argument.
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), dl, FI, Op.getOperand(1),
                      MachinePointerInfo(SV),
                      false, false, 0);
}


SDValue LE1TargetLowering::
LowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const {
  // check the depth
  assert((cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue() == 0) &&
         "Frame address can only be determined for current frame.");

  MachineFrameInfo *MFI = DAG.getMachineFunction().getFrameInfo();
  MFI->setFrameAddressIsTaken(true);
  EVT VT = Op.getValueType();
  DebugLoc dl = Op.getDebugLoc();
  SDValue FrameAddr = DAG.getCopyFromReg(DAG.getEntryNode(), dl, LE1::STRP, VT);
  return FrameAddr;
}

// TODO: set SType according to the desired memory barrier behavior.
/*SDValue LE1TargetLowering::LowerMEMBARRIER(SDValue Op,
                                            SelectionDAG& DAG) const {
  unsigned SType = 0;
  DebugLoc dl = Op.getDebugLoc();
  return DAG.getNode(LE1ISD::Sync, dl, MVT::Other, Op.getOperand(0),
                     DAG.getConstant(SType, MVT::i32));
}*/

//===----------------------------------------------------------------------===//
//                      Calling Convention Implementation
//===----------------------------------------------------------------------===//
#include "LE1GenCallingConv.inc"

//===----------------------------------------------------------------------===//
// TODO: Implement a generic logic using tblgen that can support this.
// LE1 O32 ABI rules:
// ---
// i32 - Passed in A0, A1, A2, A3 and stack
// f32 - Only passed in f32 registers if no int reg has been used yet to hold
//       an argument. Otherwise, passed in A1, A2, A3 and stack.
// f64 - Only passed in two aliased f32 registers if no int reg has been used
//       yet to hold an argument. Otherwise, use A2, A3 and stack. If A1 is
//       not used, it must be shadowed. If only A3 is avaiable, shadow it and
//       go to stack.
//
//  For vararg functions, all arguments are passed in A0, A1, A2, A3 and stack.
//===----------------------------------------------------------------------===//
/*static bool CC_LE1(unsigned ValNo, MVT ValVT,
                       MVT LocVT, CCValAssign::LocInfo LocInfo,
                       ISD::ArgFlagsTy ArgFlags, CCState &State) {

  static const unsigned IntRegsSize=8;

  static const unsigned IntRegs[] = {
      LE1::AR0, LE1::AR1, LE1::AR2, LE1::AR3,
      LE1::AR4, LE1::AR5, LE1::AR6, LE1::AR7
  };

  // ByVal Args
  if (ArgFlags.isByVal()) {
    State.HandleByVal(ValNo, ValVT, LocVT, LocInfo,
                      1, 4, ArgFlags);
    unsigned NextReg = (State.getNextStackOffset() + 3) / 4;
    for (unsigned r = State.getFirstUnallocated(IntRegs, IntRegsSize);
         r < std::min(IntRegsSize, NextReg); ++r)
      State.AllocateReg(IntRegs[r]);
    return false;
  }

  // Promote i8 and i16
  if (LocVT == MVT::i8 || LocVT == MVT::i16) {
    LocVT = MVT::i32;
    if (ArgFlags.isSExt())
      LocInfo = CCValAssign::SExt;
    else if (ArgFlags.isZExt())
      LocInfo = CCValAssign::ZExt;
    else
      LocInfo = CCValAssign::AExt;
  }

  unsigned Reg = State.AllocateReg(IntRegs, IntRegsSize);
  unsigned SizeInBytes = ValVT.getSizeInBits() >> 3;
  unsigned OrigAlign = ArgFlags.getOrigAlign();
  unsigned Offset = State.AllocateStack(SizeInBytes, OrigAlign);

  if (!Reg)
    State.addLoc(CCValAssign::getMem(ValNo, ValVT, Offset, LocVT, LocInfo));
  else
    State.addLoc(CCValAssign::getReg(ValNo, ValVT, Reg, LocVT, LocInfo));

  return false; // CC must always match
}*/


//===----------------------------------------------------------------------===//
//                  Call Calling Convention Implementation
//===----------------------------------------------------------------------===//

static const unsigned O32IntRegsSize = 8;
//FIXME this needs to include AR4,5,6,7
static const uint16_t O32IntRegs[] = {
  LE1::AR0, LE1::AR1, LE1::AR2, LE1::AR3, 
  LE1::AR4, LE1::AR5, LE1::AR6, LE1::AR7
};

// Return next O32 integer argument register.
//static unsigned getNextIntArgReg(unsigned Reg) {
//  assert((Reg == LE1::AR0) || (Reg == LE1::AR2));
//  return (Reg == LE1::AR0) ? LE1::AR1 : LE1::AR3;
//}

// Write ByVal Arg to arg registers and stack.
static void
WriteByValArg(SDValue& ByValChain, SDValue Chain, DebugLoc dl,
              SmallVector<std::pair<unsigned, SDValue>, 8>& RegsToPass,
              SmallVector<SDValue, 8>& MemOpChains, int& LastFI,
              MachineFrameInfo *MFI, SelectionDAG &DAG, SDValue Arg,
              const CCValAssign &VA, const ISD::ArgFlagsTy& Flags,
              MVT PtrType, bool isLittle) {
  //std::cout << "Entering WriteByValArg\n";

  unsigned LocMemOffset = 0;
  if(VA.isMemLoc())
    LocMemOffset = VA.getLocMemOffset();
  unsigned Offset = 0;
  uint32_t RemainingSize = Flags.getByValSize();
  unsigned ByValAlign = Flags.getByValAlign();

  // Copy the first 4 words of byval arg to registers A0 - A3.
  // FIXME: Use a stricter alignment if it enables better optimization in passes
  //        run later.
  for (; RemainingSize >= 4 && LocMemOffset < 4 * 4;
       Offset += 4, RemainingSize -= 4, LocMemOffset += 4) {
    SDValue LoadPtr = DAG.getNode(ISD::ADD, dl, MVT::i32, Arg,
                                  DAG.getConstant(Offset, MVT::i32));
    SDValue LoadVal = DAG.getLoad(MVT::i32, dl, Chain, LoadPtr,
                                  MachinePointerInfo(),
                                  false, false, false,
                                  std::min(ByValAlign, (unsigned )4));
    MemOpChains.push_back(LoadVal.getValue(1));
    unsigned DstReg = O32IntRegs[LocMemOffset / 4];
    RegsToPass.push_back(std::make_pair(DstReg, LoadVal));
  }

  if (RemainingSize == 0) {
    //std::cout << "RemainingSize = 0\n";
    return;
  }

  // If there still is a register available for argument passing, write the
  // remaining part of the structure to it using subword loads and shifts.
  if (LocMemOffset < 4 * 4) {
    assert(RemainingSize <= 3 && RemainingSize >= 1 &&
           "There must be one to three bytes remaining.");
    unsigned LoadSize = (RemainingSize == 3 ? 2 : RemainingSize);
    SDValue LoadPtr = DAG.getNode(ISD::ADD, dl, MVT::i32, Arg,
                                  DAG.getConstant(Offset, MVT::i32));
    unsigned Alignment = std::min(ByValAlign, (unsigned )4);
    SDValue LoadVal = DAG.getExtLoad(ISD::ZEXTLOAD, dl, MVT::i32, Chain,
                                     LoadPtr, MachinePointerInfo(),
                                     MVT::getIntegerVT(LoadSize * 8), false,
                                     false, Alignment);
    MemOpChains.push_back(LoadVal.getValue(1));

    // If target is big endian, shift it to the most significant half-word or
    // byte.
    if (!isLittle)
      LoadVal = DAG.getNode(ISD::SHL, dl, MVT::i32, LoadVal,
                            DAG.getConstant(32 - LoadSize * 8, MVT::i32));

    Offset += LoadSize;
    RemainingSize -= LoadSize;

    // Read second subword if necessary.
    if (RemainingSize != 0)  {
      assert(RemainingSize == 1 && "There must be one byte remaining.");
      LoadPtr = DAG.getNode(ISD::ADD, dl, MVT::i32, Arg, 
                            DAG.getConstant(Offset, MVT::i32));
      unsigned Alignment = std::min(ByValAlign, (unsigned )2);
      SDValue Subword = DAG.getExtLoad(ISD::ZEXTLOAD, dl, MVT::i32, Chain,
                                       LoadPtr, MachinePointerInfo(),
                                       MVT::i8, false, false, Alignment);
      MemOpChains.push_back(Subword.getValue(1));
      // Insert the loaded byte to LoadVal.
      // FIXME: Use INS if supported by target.
      unsigned ShiftAmt = isLittle ? 16 : 8;
      SDValue Shift = DAG.getNode(ISD::SHL, dl, MVT::i32, Subword,
                                  DAG.getConstant(ShiftAmt, MVT::i32));
      LoadVal = DAG.getNode(ISD::OR, dl, MVT::i32, LoadVal, Shift);
    }

    unsigned DstReg = O32IntRegs[LocMemOffset / 4];
    RegsToPass.push_back(std::make_pair(DstReg, LoadVal));
    return;
  }

  // Create a fixed object on stack at offset LocMemOffset and copy
  // remaining part of byval arg to it using memcpy.
  SDValue Src = DAG.getNode(ISD::ADD, dl, MVT::i32, Arg,
                            DAG.getConstant(Offset, MVT::i32));
  LastFI = MFI->CreateFixedObject(RemainingSize, LocMemOffset, true);
  SDValue Dst = DAG.getFrameIndex(LastFI, PtrType);
  ByValChain = DAG.getMemcpy(ByValChain, dl, Dst, Src,
                             DAG.getConstant(RemainingSize, MVT::i32),
                             std::min(ByValAlign, (unsigned)4),
                             /*isVolatile=*/false, /*AlwaysInline=*/false,
                             MachinePointerInfo(0), MachinePointerInfo(0));

  //std::cout << "Leaving WriteByValArg\n";
}

/// LowerCall - functions arguments are copied from virtual regs to
/// (physical regs)/(stack frame), CALLSEQ_START and CALLSEQ_END are emitted.
/// TODO: isTailCall.
SDValue
LE1TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                 SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG                     = CLI.DAG;
  DebugLoc &dl                          = CLI.DL;
  SmallVector<ISD::OutputArg, 32> &Outs = CLI.Outs;
  SmallVector<SDValue, 32> &OutVals     = CLI.OutVals;
  SmallVector<ISD::InputArg, 32> &Ins   = CLI.Ins;
  SDValue InChain                         = CLI.Chain;
  SDValue Callee                        = CLI.Callee;
  bool &isTailCall                      = CLI.IsTailCall;
  CallingConv::ID CallConv              = CLI.CallConv;
  bool isVarArg                         = CLI.IsVarArg;

  isTailCall = false;

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  const TargetFrameLowering *TFL = MF.getTarget().getFrameLowering();

  // Position-independent code: a body of code that executes correctly
  // regardless of its absolute address.
  //bool IsPIC = getTargetMachine().getRelocationModel() == Reloc::PIC_;
  LE1FunctionInfo *LE1FI = MF.getInfo<LE1FunctionInfo>();

  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
		 getTargetMachine(), ArgLocs, *DAG.getContext());

  CCInfo.AnalyzeCallOperands(Outs, CC_LE1);

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NextStackOffset = CCInfo.getNextStackOffset();

  // Chain is the output chain of the last Load/Store or CopyToReg node.
  // ByValChain is the output chain of the last Memcpy node created for copying
  // byval arguments to the stack.
  SDValue Chain, CallSeqStart, ByValChain;
  SDValue NextStackOffsetVal = DAG.getIntPtrConstant(NextStackOffset, true);
  Chain = CallSeqStart = DAG.getCALLSEQ_START(InChain, NextStackOffsetVal);
  ByValChain = InChain;

  //std::cout << "Chain Op = " << Chain.getNode()->getOpcode() << std::endl;

  // If this is the first call, create a stack frame object that points to
  // a location to which .cprestore saves $gp.
  //if (IsPIC && !LE1FI->getGPFI())
    //LE1FI->setGPFI(MFI->CreateFixedObject(4, 0, true));

  // Get the frame index of the stack frame object that points to the location
  // of dynamically allocated area on the stack.
  int DynAllocFI = LE1FI->getDynAllocFI();

  // Update size of the maximum argument space.
  // For O32, a minimum of four words (16 bytes) of argument space is
  // allocated.
  
  unsigned MaxCallFrameSize = LE1FI->getMaxCallFrameSize();

  if (MaxCallFrameSize < NextStackOffset) {
    LE1FI->setMaxCallFrameSize(NextStackOffset);

    // Set the offsets relative to $sp of the $gp restore slot and dynamically
    // allocated stack space. These offsets must be aligned to a boundary
    // determined by the stack alignment of the ABI.
    unsigned StackAlignment = TFL->getStackAlignment();
    NextStackOffset = (NextStackOffset + StackAlignment - 1) /
                      StackAlignment * StackAlignment;

    //if (IsPIC)
      //MFI->setObjectOffset(LE1FI->getGPFI(), NextStackOffset);

    MFI->setObjectOffset(DynAllocFI, NextStackOffset);
  }

  // With EABI is it possible to have 16 args on registers.
  //SmallVector<std::pair<unsigned, SDValue>, 16> RegsToPass;
  
  // With LE1 it is possible to have 8 args on registers.
  SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;

  int FirstFI = -MFI->getNumFixedObjects() - 1, LastFI = 0;

  // Walk the register/memloc assignments, inserting copies/loads.
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    SDValue Arg = OutVals[i];
    CCValAssign &VA = ArgLocs[i];

    // ByVal Arg.
    ISD::ArgFlagsTy Flags = Outs[i].Flags;
    if (Flags.isByVal()) {
      //assert(Subtarget->isABI_O32() &&
        //     "No support for ByVal args by ABIs other than O32 yet.");
      assert(Flags.getByValSize() &&
             "ByVal args of size 0 should have been ignored by front-end.");
      WriteByValArg(ByValChain, Chain, dl, RegsToPass, MemOpChains, LastFI, MFI,
                    DAG, Arg, VA, Flags, getPointerTy(), false);
      continue;
    }

    // Promote the value if needed.
    switch (VA.getLocInfo()) {
    default: llvm_unreachable("Unknown loc info!");
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    }

    // Arguments that can be passed on register must be kept at
    // RegsToPass vector
    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
      continue;
    }

    // Register can't get to this point...
    assert(VA.isMemLoc());


    // Create the frame index object for this incoming parameter
    LastFI = MFI->CreateFixedObject(VA.getValVT().getSizeInBits()/8,
                                    VA.getLocMemOffset(), true);
    SDValue PtrOff = DAG.getFrameIndex(LastFI, getPointerTy());

    // emit ISD::STORE whichs stores the
    // parameter value to a stack Location
    MemOpChains.push_back(DAG.getStore(Chain, dl, Arg, PtrOff,
                                       MachinePointerInfo(),
                                       false, false, 0));
  }

  // Extend range of indices of frame objects for outgoing arguments that were
  // created during this function call. Skip this step if no such objects were
  // created.
  if (LastFI)
    LE1FI->extendOutArgFIRange(FirstFI, LastFI);

  // If a memcpy has been created to copy a byval arg to a stack, replace the
  // chain input of CallSeqStart with ByValChain.
  if (InChain != ByValChain)
    DAG.UpdateNodeOperands(CallSeqStart.getNode(), ByValChain,
                           NextStackOffsetVal);

  // Transform all store nodes into one single node because all store
  // nodes are independent of each other.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other,
                        &MemOpChains[0], MemOpChains.size());

  // If the callee is a GlobalAddress/ExternalSymbol node (quite common, every
  // direct call is) turn it into a TargetGlobalAddress/TargetExternalSymbol
  // node so that legalize doesn't hack it.

  //unsigned char OpFlag = IsPIC ? LE1II::MO_GOT_CALL : LE1II::MO_NO_FLAG;
  unsigned char OpFlag = LE1II::MO_NO_FLAG;

  bool LoadSymAddr = false;
  SDValue CalleeLo;

  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) {
    //if (IsPIC && G->getGlobal()->hasInternalLinkage()) {
      //Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl,
        //                                  getPointerTy(), 0, 0);
      //CalleeLo = DAG.getTargetGlobalAddress(G->getGlobal(), dl, getPointerTy(),
        //                                    0, LE1II::MO_ABS_LO);
    //} else {
      //std::cout << "GlobalAddress\n";
      Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl,
                                          getPointerTy(), 0, 0);
      //std::cout << "Callee is TargetGlobalAddress\n";
    //}

    LoadSymAddr = true;
  }
  else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    Callee = DAG.getTargetExternalSymbol(S->getSymbol(),
                                getPointerTy(), OpFlag);
    LoadSymAddr = true;
    //std::cout << "ExternalSymbol\n";
  }

  SDValue InFlag;
  if(!LoadSymAddr) {
    //std::cout << "not LoadSymAddr\n";
    //SDValue LoadValue = DAG.getLoad(MVT::i32, dl, DAG.getEntryNode(), Callee,
      //                              MachinePointerInfo::getGOT(),
        //                            false, false, false, 0);
    //Chain = DAG.getCopyToReg(Chain, dl, LE1::L0, LoadValue, SDValue(0,0));
    /*
    std::cout << "Callee NumOperands = " << Callee.getNumOperands() << std::endl;
    if(Callee.getOpcode() == ISD::CopyToReg)
      std::cout << "CopyToReg\n";
    else if(Callee.getOpcode() == ISD::LOAD &&
           (dyn_cast<GlobalAddressSDNode>(Callee.getNode()->getOperand(1)))) {
      std::cout << "Load\n";
      LoadSDNode *LoadNode = cast<LoadSDNode>(Callee.getNode());
      SDNode *Target = LoadNode->getBasePtr().getNode();
      std::cout << "got target\n";
      //SDValue Base = Target->getOperand(0);
      SDValue Base = LoadNode->getOperand(1);
      std::cout << "got base\n";
        int64_t Offset = cast<GlobalAddressSDNode>(Base)->getOffset();
        std::cout << "got offset\n";
        const GlobalValue *GV = cast<GlobalAddressSDNode>(Base)->getGlobal();
        std::cout << "got GV\n";
      SDValue TargetAddr = DAG.getTargetGlobalAddress(GV, dl,
                                                             getPointerTy(), 0);
      std::cout << "created TargetAddr\n";
      SDValue TargetOffset = DAG.getTargetConstant(Offset, getPointerTy());
      std::cout << "created TargetOffset\n";
      SDValue LoadLink = DAG.getNode(LE1ISD::LoadLink, dl,
                                     DAG.getVTList(MVT::i32, MVT::Other),
                                         TargetAddr, TargetOffset, Chain);
      std::cout << "created LoadLink\n";
      //MachineSDNode::mmo_iterator MemOp = MF.allocateMemRefsArray(1);
      //MemOp[0] = LoadNode->getMemOperand();
      //cast<MachineSDNode>(LoadLink)->setMemRefs(MemOp, MemOp+1);
      //DAG.ReplaceAllUsesOfValueWith(Callee, LoadLink);
      Callee = LoadLink;
    }*/
    //for(int i=0;i<Callee.getNumOperands();++i) {
      //if(Callee.getOperand(i).isTargetMemoryOpcode())
        //std::cout << "Operand " << i << " isTargetMemoryOpcode\n";
      //else if(Callee.getOperand(i).isTargetOpcode())
        //std::cout << "Operand " << i << " isTargetOpcode\n";
    //}
    //Callee.getNode()
    Chain = DAG.getCopyToReg(Chain, dl, LE1::L0, Callee, SDValue(0,0));
    //Callee = DAG.getNode(LE1ISD::MTL, dl, MVT::i32,
              //          DAG.getRegister(LE1::L0, MVT::i32), Callee);
    InFlag = Chain.getValue(1);
    Callee = DAG.getRegister(LE1::L0, MVT::i32);
  }

  // Create nodes that load address of callee and copy it to T9
  //if (IsPIC) {
    //std::cout << "IsPIC\n";
    //if (LoadSymAddr) {
      // Load callee address
      //std::cout << "Load callee address\n";
      //Callee = DAG.getNode(LE1ISD::WrapperPIC, dl, MVT::i32, Callee);
      //SDValue LoadValue = DAG.getLoad(MVT::i32, dl, DAG.getEntryNode(), Callee,
        //                              MachinePointerInfo::getGOT(),
          //                            false, false, 0);

      // Use GOT+LO if callee has internal linkage.
      //if (CalleeLo.getNode()) {
        //std::cout << "CalleeLo\n";
        //SDValue Lo = DAG.getNode(LE1ISD::Lo, dl, MVT::i32, CalleeLo);
        //Callee = DAG.getNode(ISD::ADD, dl, MVT::i32, LoadValue, Lo);
      //} else
        //Callee = LoadValue;
    //}

    // copy to T9
    //Chain = DAG.getCopyToReg(Chain, dl, LE1::T9, Callee, SDValue(0, 0));
    //InFlag = Chain.getValue(1);
    //Callee = DAG.getRegister(LE1::T9, MVT::i32);
  //}

  // Build a sequence of copy-to-reg nodes chained together with token
  // chain and flag operands which copy the outgoing args into registers.
  // The InFlag in necessary since all emitted instructions must be
  // stuck together.
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Chain = DAG.getCopyToReg(Chain, dl, RegsToPass[i].first,
                             RegsToPass[i].second, InFlag);
    InFlag = Chain.getValue(1);
  }

  // LE1Call = #chain, #link_register #target_address, #opt_in_flags...
  //             = Chain, LinkReg, Callee, Reg#1, Reg#2, ...
  //
  // Returns a chain & a flag for retval copy to use.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  SDValue LinkReg = DAG.getRegister(LE1::L0, MVT::i32);
  Ops.push_back(LinkReg);
  Ops.push_back(Callee);

  // Add argument registers to the end of the list so that they are
  // known live into the call.
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i)
    Ops.push_back(DAG.getRegister(RegsToPass[i].first,
                                  RegsToPass[i].second.getValueType()));

  if (InFlag.getNode())
    Ops.push_back(InFlag);

  if(LoadSymAddr)
    Chain  = DAG.getNode(LE1ISD::Call, dl, NodeTys,
                        &Ops[0], Ops.size());
  else
    Chain = DAG.getNode(LE1ISD::Call, dl, NodeTys, &Ops[0], Ops.size());

  InFlag = Chain.getValue(1);

  // Create the CALLSEQ_END node.
  Chain = DAG.getCALLSEQ_END(Chain,
                             DAG.getIntPtrConstant(NextStackOffset, true),
                             DAG.getIntPtrConstant(0, true), InFlag);
  InFlag = Chain.getValue(1);

  //std::cout << "Leaving LE1TargetLowering::LowerCall\n";
  // Handle result values, copying them out of physregs into vregs that we
  // return.
  return LowerCallResult(Chain, InFlag, CallConv, isVarArg,
                         Ins, dl, DAG, InVals);
}

/// LowerCallResult - Lower the result values of a call into the
/// appropriate copies out of appropriate physical registers.
SDValue
LE1TargetLowering::LowerCallResult(SDValue Chain, SDValue InFlag,
                                    CallingConv::ID CallConv, bool isVarArg,
                                    const SmallVectorImpl<ISD::InputArg> &Ins,
                                    DebugLoc dl, SelectionDAG &DAG,
                                    SmallVectorImpl<SDValue> &InVals) const {
  //std::cout << "Entering LE1TargetLowering::LowerCallResult\n";
  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
		 getTargetMachine(), RVLocs, *DAG.getContext());

  CCInfo.AnalyzeCallResult(Ins, RetCC_LE1);

  // Copy all of the result registers out of their specified physreg.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    Chain = DAG.getCopyFromReg(Chain, dl, RVLocs[i].getLocReg(),
                               RVLocs[i].getValVT(), InFlag).getValue(1);
    InFlag = Chain.getValue(2);
    InVals.push_back(Chain.getValue(0));
  }

  //std::cout << "Leaving LE1TargetLowering::LowerCallResult\n";
  return Chain;
}

//===----------------------------------------------------------------------===//
//             Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//
static void ReadByValArg(MachineFunction &MF, SDValue Chain, DebugLoc dl,
                         std::vector<SDValue>& OutChains,
                         SelectionDAG &DAG, unsigned NumWords, SDValue FIN,
                         const CCValAssign &VA, const ISD::ArgFlagsTy& Flags) {
  // FIXME shouldn't have to copy values into the frame
  //std::cout << "Entering ReadByValArg\n";
  unsigned LocMem = VA.getLocMemOffset();
  unsigned FirstWord = LocMem / 4;

  // copy register A0 - A3 to frame object
  // copy register AR0 - AR7 to frame object
  for (unsigned i = 0; i < NumWords; ++i) {
    unsigned CurWord = FirstWord + i;
    if (CurWord >= O32IntRegsSize)
      break;

    unsigned SrcReg = O32IntRegs[CurWord];
    unsigned Reg = AddLiveIn(MF, SrcReg, &LE1::CPURegsRegClass);
    SDValue StorePtr = DAG.getNode(ISD::ADD, dl, MVT::i32, FIN,
                                   DAG.getConstant(i * 4, MVT::i32));
    SDValue Store = DAG.getStore(Chain, dl, DAG.getRegister(Reg, MVT::i32),
                                 StorePtr, MachinePointerInfo(), false,
                                 false, 0);
    OutChains.push_back(Store);
  }
  //std::cout << "Leaving ReadByValArg\n";
}

/// LowerFormalArguments - transform physical registers into virtual registers
/// and generate load operations for arguments places on the stack.
SDValue
LE1TargetLowering::LowerFormalArguments(SDValue Chain,
                                        CallingConv::ID CallConv,
                                        bool isVarArg,
                                      const SmallVectorImpl<ISD::InputArg> &Ins,
                                        DebugLoc dl, SelectionDAG &DAG,
                                      SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  LE1FunctionInfo *LE1FI = MF.getInfo<LE1FunctionInfo>();

  LE1FI->setVarArgsFrameIndex(0);

  // Used with vargs to acumulate store chains.
  std::vector<SDValue> OutChains;

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 getTargetMachine(), ArgLocs, *DAG.getContext());

  //if (IsO32)
    //CCInfo.AnalyzeFormalArguments(Ins, CC_MipsO32);
  //else
    //CCInfo.AnalyzeFormalArguments(Ins, CC_Mips);
  CCInfo.AnalyzeFormalArguments(Ins, CC_LE1);
  int LastFI = 0;// MipsFI->LastInArgFI is 0 at the entry of this function.
  //std::cout << "ArgLocs.size = " << ArgLocs.size() << std::endl;
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    EVT ValVT = VA.getValVT();
    ISD::ArgFlagsTy Flags = Ins[i].Flags;
    bool IsRegLoc = VA.isRegLoc();

    if (Flags.isByVal() && !IsRegLoc) {
      //std::cout << "isByVal\n";
      assert(Flags.getByValSize() &&
             "ByVal args of size 0 should have been ignored by front-end.");
      //if (IsO32) {
        unsigned NumWords = (Flags.getByValSize() + 3) / 4;
        LastFI = MFI->CreateFixedObject(NumWords * 4, VA.getLocMemOffset(),
                                        true);
        SDValue FIN = DAG.getFrameIndex(LastFI, getPointerTy());
        InVals.push_back(FIN);
        ReadByValArg(MF, Chain, dl, OutChains, DAG, NumWords, FIN, VA, Flags);
      //} else // N32/64
        //LastFI = CopyMips64ByValRegs(MF, Chain, dl, OutChains, DAG, VA, Flags,
          //                           MFI, IsRegLoc, InVals, MipsFI,
            //                         getPointerTy());
      continue;
    }

    // Arguments stored on registers
    if (IsRegLoc) {
      //std::cout << "isRegLoc\n";
      EVT RegVT = VA.getLocVT();
      unsigned ArgReg = VA.getLocReg();
      const TargetRegisterClass *RC = &LE1::CPURegsRegClass;

      //if (RegVT == MVT::i32)
        //RC = Mips::CPURegsRegisterClass;
      //else if (RegVT == MVT::i64)
        //RC = Mips::CPU64RegsRegisterClass;
      //else if (RegVT == MVT::f32)
        //RC = Mips::FGR32RegisterClass;
      //else if (RegVT == MVT::f64)
        //RC = HasMips64 ? Mips::FGR64RegisterClass : Mips::AFGR64RegisterClass;
      //else
        //llvm_unreachable("RegVT not supported by FormalArguments Lowering");

      // Transform the arguments stored on
      // physical registers into virtual ones
      unsigned Reg = AddLiveIn(DAG.getMachineFunction(), ArgReg, RC);
      SDValue ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, RegVT);

      // If this is an 8 or 16-bit value, it has been passed promoted
      // to 32 bits.  Insert an assert[sz]ext to capture this, then
      // truncate to the right size.
      if (VA.getLocInfo() != CCValAssign::Full) {
        unsigned Opcode = 0;
        if (VA.getLocInfo() == CCValAssign::SExt)
          Opcode = ISD::AssertSext;
        else if (VA.getLocInfo() == CCValAssign::ZExt)
          Opcode = ISD::AssertZext;
        if (Opcode)
          ArgValue = DAG.getNode(Opcode, dl, RegVT, ArgValue,
                                 DAG.getValueType(ValVT));
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, ValVT, ArgValue);
      }

      // Handle floating point arguments passed in integer registers.
      //if ((RegVT == MVT::i32 && ValVT == MVT::f32) ||
        //  (RegVT == MVT::i64 && ValVT == MVT::f64))
        //ArgValue = DAG.getNode(ISD::BITCAST, dl, ValVT, ArgValue);
      //else if (IsO32 && RegVT == MVT::i32 && ValVT == MVT::f64) {
        //unsigned Reg2 = AddLiveIn(DAG.getMachineFunction(),
          //                        getNextIntArgReg(ArgReg), RC);
        //SDValue ArgValue2 = DAG.getCopyFromReg(Chain, dl, Reg2, RegVT);
        //if (!Subtarget->isLittle())
          //std::swap(ArgValue, ArgValue2);
        //ArgValue = DAG.getNode(MipsISD::BuildPairF64, dl, MVT::f64,
          //                     ArgValue, ArgValue2);
      //}

      InVals.push_back(ArgValue);
    } else { // VA.isRegLoc()

      // sanity check
      assert(VA.isMemLoc());

      // The stack pointer offset is relative to the caller stack frame.
      LastFI = MFI->CreateFixedObject(ValVT.getSizeInBits()/8,
                                      VA.getLocMemOffset(), true);

      // Create load nodes to retrieve arguments from the stack
      SDValue FIN = DAG.getFrameIndex(LastFI, getPointerTy());
      InVals.push_back(DAG.getLoad(ValVT, dl, Chain, FIN,
                                   MachinePointerInfo::getFixedStack(LastFI),
                                   false, false, false, 0));
    }
  }

  // The mips ABIs for returning structs by value requires that we copy
  // the sret argument into $v0 for the return. Save the argument into
  // a virtual register so that we can access it from the return points.
  if (DAG.getMachineFunction().getFunction()->hasStructRetAttr()) {
    unsigned Reg = LE1FI->getSRetReturnReg();
    if (!Reg) {
      Reg = MF.getRegInfo().createVirtualRegister(getRegClassFor(MVT::i32));
      LE1FI->setSRetReturnReg(Reg);
    }
    SDValue Copy = DAG.getCopyToReg(DAG.getEntryNode(), dl, Reg, InVals[0]);
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, Copy, Chain);
  }

  if (isVarArg) {
    //unsigned NumOfRegs = IsO32 ? 4 : 8;
    unsigned NumOfRegs = 8;
    //const unsigned *ArgRegs = IsO32 ? O32IntRegs : Mips64IntRegs;
    const uint16_t *ArgRegs = O32IntRegs;
    unsigned Idx = CCInfo.getFirstUnallocated(O32IntRegs, NumOfRegs);
    //int FirstRegSlotOffset = IsO32 ? 0 : -64 ; // offset of $a0's slot.
    int FirstRegSlotOffset = 0;
    const TargetRegisterClass *RC = &LE1::CPURegsRegClass;
    //  = IsO32 ? Mips::CPURegsRegisterClass : Mips::CPU64RegsRegisterClass;
    unsigned RegSize = RC->getSize();
    int RegSlotOffset = FirstRegSlotOffset + Idx * RegSize;

    // Offset of the first variable argument from stack pointer.
    int FirstVaArgOffset;

    //if (IsO32 || (Idx == NumOfRegs)) {
    if(Idx == NumOfRegs) {
      FirstVaArgOffset =
        (CCInfo.getNextStackOffset() + RegSize - 1) / RegSize * RegSize;
    } else
      FirstVaArgOffset = RegSlotOffset;

    // Record the frame index of the first variable argument
    // which is a value necessary to VASTART.
    LastFI = MFI->CreateFixedObject(RegSize, FirstVaArgOffset, true);
    LE1FI->setVarArgsFrameIndex(LastFI);

    // Copy the integer registers that have not been used for argument passing
    // to the argument register save area. For O32, the save area is allocated
    // in the caller's stack frame, while for N32/64, it is allocated in the
    // callee's stack frame.
    for (int StackOffset = RegSlotOffset;
         Idx < NumOfRegs; ++Idx, StackOffset += RegSize) {
      unsigned Reg = AddLiveIn(DAG.getMachineFunction(), ArgRegs[Idx], RC);
      SDValue ArgValue = DAG.getCopyFromReg(Chain, dl, Reg,
                                            MVT::getIntegerVT(RegSize * 8));
      LastFI = MFI->CreateFixedObject(RegSize, StackOffset, true);
      SDValue PtrOff = DAG.getFrameIndex(LastFI, getPointerTy());
      OutChains.push_back(DAG.getStore(Chain, dl, ArgValue, PtrOff,
                                       MachinePointerInfo(), false, false, 0));
    }
  }

  LE1FI->setLastInArgFI(LastFI);

  // All stores are grouped in one node to allow the matching between
  // the size of Ins and InVals. This only happens when on varg functions
  if (!OutChains.empty()) {
    OutChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other,
                        &OutChains[0], OutChains.size());
  }
  //std::cout << "Leaving LowerFormalArguments\n";
  return Chain;
}

/*
SDValue
LE1TargetLowering::LowerFormalArguments(SDValue Chain,
                                         CallingConv::ID CallConv,
                                         bool isVarArg,
                                         const SmallVectorImpl<ISD::InputArg>
                                         &Ins,
                                         DebugLoc dl, SelectionDAG &DAG,
                                         SmallVectorImpl<SDValue> &InVals)
                                          const {
  std::cout << "Entering LE1TargetLowering::LowerFormalArguments\n";
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  LE1FunctionInfo *LE1FI = MF.getInfo<LE1FunctionInfo>();

  LE1FI->setVarArgsFrameIndex(0);

  // Used with vargs to acumulate store chains.
  std::vector<SDValue> OutChains;

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;

  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
		 getTargetMachine(), ArgLocs, *DAG.getContext());

  CCInfo.AnalyzeFormalArguments(Ins, CC_LE1);

  int LastFI = 0;// LE1FI->LastInArgFI is 0 at the entry of this function.
  std::cout << "ArgLocs.size = " << ArgLocs.size() << std::endl;
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];

    // Arguments stored on registers
    //if (VA.isRegLoc()) {
    if(i < 8) {
      EVT RegVT = VA.getLocVT();
      unsigned ArgReg = VA.getLocReg();
      if (RegVT != MVT::i32)
        llvm_unreachable("RegVT not supported by FormalArguments Lowering");

      const TargetRegisterClass *RC = LE1::CPURegsRegisterClass;

      //if (RegVT == MVT::i32)
        //RC = LE1::CPURegsRegisterClass;
      //else
        //llvm_unreachable("RegVT not supported by FormalArguments Lowering");

      // Transform the arguments stored on
      // physical registers into virtual ones
      unsigned Reg = AddLiveIn(DAG.getMachineFunction(), ArgReg, RC);
      SDValue ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, RegVT);

      // If this is an 8 or 16-bit value, it has been passed promoted
      // to 32 bits.  Insert an assert[sz]ext to capture this, then
      // truncate to the right size.
      if (VA.getLocInfo() != CCValAssign::Full) {
        unsigned Opcode = 0;
        if (VA.getLocInfo() == CCValAssign::SExt)
          Opcode = ISD::AssertSext;
        else if (VA.getLocInfo() == CCValAssign::ZExt)
          Opcode = ISD::AssertZext;
        if (Opcode)
          ArgValue = DAG.getNode(Opcode, dl, RegVT, ArgValue,
                                 DAG.getValueType(VA.getValVT()));
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), ArgValue);
      }

      InVals.push_back(ArgValue);
    } else { // VA.isRegLoc()

      // sanity check
      assert(VA.isMemLoc());

      ISD::ArgFlagsTy Flags = Ins[i].Flags;

      if (Flags.isByVal()) {
        //assert(Subtarget->isABI_O32() &&
          //     "No support for ByVal args by ABIs other than O32 yet.");
        assert(Flags.getByValSize() &&
               "ByVal args of size 0 should have been ignored by front-end.");
        unsigned NumWords = (Flags.getByValSize() + 3) / 4;
        LastFI = MFI->CreateFixedObject(NumWords * 4, VA.getLocMemOffset(),
                                        true);
        SDValue FIN = DAG.getFrameIndex(LastFI, getPointerTy());
        InVals.push_back(FIN);
        ReadByValArg(MF, Chain, dl, OutChains, DAG, NumWords, FIN, VA, Flags);

        continue;
      }

      // The stack pointer offset is relative to the caller stack frame.
      LastFI = MFI->CreateFixedObject(VA.getValVT().getSizeInBits()/8,
                                      VA.getLocMemOffset(), true);

      // Create load nodes to retrieve arguments from the stack
      SDValue FIN = DAG.getFrameIndex(LastFI, getPointerTy());
      InVals.push_back(DAG.getLoad(VA.getValVT(), dl, Chain, FIN,
                                   MachinePointerInfo::getFixedStack(LastFI),
                                   false, false, false, 0));
    }
  }

  // The le1 ABIs for returning structs by value requires that we copy
  // the sret argument into $v0 for the return. Save the argument into
  // a virtual register so that we can access it from the return points.
  if (DAG.getMachineFunction().getFunction()->hasStructRetAttr()) {
    unsigned Reg = LE1FI->getSRetReturnReg();
    if (!Reg) {
      Reg = MF.getRegInfo().createVirtualRegister(getRegClassFor(MVT::i32));
      LE1FI->setSRetReturnReg(Reg);
    }
    SDValue Copy = DAG.getCopyToReg(DAG.getEntryNode(), dl, Reg, InVals[0]);
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, Copy, Chain);
  }

  LE1FI->setLastInArgFI(LastFI);

  // All stores are grouped in one node to allow the matching between
  // the size of Ins and InVals. This only happens when on varg functions
  if (!OutChains.empty()) {
    OutChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other,
                        &OutChains[0], OutChains.size());
  }

  //std::cout << "Leaving LE1TargetLowering::LowerFormalArguments\n";
  return Chain;
}
*/
//===----------------------------------------------------------------------===//
//               Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue
LE1TargetLowering::LowerReturn(SDValue Chain,
                                CallingConv::ID CallConv, bool isVarArg,
                                const SmallVectorImpl<ISD::OutputArg> &Outs,
                                const SmallVectorImpl<SDValue> &OutVals,
                                DebugLoc dl, SelectionDAG &DAG) const {
  //std::cout << "Entering LE1TargetLowering::LowerReturn\n";
  // CCValAssign - represent the assignment of
  // the return value to a location
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
		 getTargetMachine(), RVLocs, *DAG.getContext());

  // Analize return values.
  CCInfo.AnalyzeReturn(Outs, RetCC_LE1);

  // If this is the first return lowered for this function, add
  // the regs to the liveout set for the function.
  if (DAG.getMachineFunction().getRegInfo().liveout_empty()) {
    for (unsigned i = 0; i != RVLocs.size(); ++i)
      if (RVLocs[i].isRegLoc())
        DAG.getMachineFunction().getRegInfo().addLiveOut(RVLocs[i].getLocReg());
  }

  SDValue Flag;

  // Copy the result values into the output registers.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign &VA = RVLocs[i];
    //assert(VA.isRegLoc() && "Can only return in registers!");

    // FIXME - This isn't necessarily a register now!
    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(),
                             OutVals[i], Flag);

    // guarantee that all emitted copies are
    // stuck together, avoiding something bad
    Flag = Chain.getValue(1);
  }

  // The le1 ABIs for returning structs by value requires that we copy
  // the sret argument into $v0 for the return. We saved the argument into
  // a virtual register in the entry block, so now we copy the value out
  // and into $v0.
  if (DAG.getMachineFunction().getFunction()->hasStructRetAttr()) {
    MachineFunction &MF      = DAG.getMachineFunction();
    LE1FunctionInfo *LE1FI = MF.getInfo<LE1FunctionInfo>();
    unsigned Reg = LE1FI->getSRetReturnReg();

    if (!Reg)
      llvm_unreachable("sret virtual register not created in the entry block");
    SDValue Val = DAG.getCopyFromReg(Chain, dl, Reg, getPointerTy());

    Chain = DAG.getCopyToReg(Chain, dl, LE1::STRP, Val, Flag);
    Flag = Chain.getValue(1);
  }

 
  //std::cout << "Leaving LE1TargetLowering::LowerReturn\n";
  if (Flag.getNode())
    return DAG.getNode(LE1ISD::Ret, dl, MVT::Other,
                       Chain, 
                       DAG.getRegister(LE1::SP, MVT::i32), 
                       DAG.getConstant(0, MVT::i32),
                       DAG.getRegister(LE1::L0, MVT::i32), 
                       Flag);

    //return DAG.getNode(LE1ISD::Ret, dl, MVT::Other, Chain, 
      //                 DAG.getRegister(LE1::SP, MVT::i32),
        //               SPVal,
          //             DAG.getRegister(LE1::L0, MVT::i32), Flag);
  else // Return Void
    //return DAG.getNode(LE1ISD::RetFlag, dl, MVT::Other, 
      //                 Chain); 
    return DAG.getNode(LE1ISD::Ret, dl, MVT::Other, Chain,
                       DAG.getRegister(LE1::SP, MVT::i32),
                       DAG.getConstant(0, MVT::i32),
                       DAG.getRegister(LE1::L0, MVT::i32));

}

//===----------------------------------------------------------------------===//
//                           LE1 Inline Assembly Support
//===----------------------------------------------------------------------===//

/// getConstraintType - Given a constraint letter, return the type of
/// constraint it is for this target.
LE1TargetLowering::ConstraintType LE1TargetLowering::
getConstraintType(const std::string &Constraint) const
{
  // LE1 specific constrainy
  // GCC config/le1/constraints.md
  //
  // 'd' : An address register. Equivalent to r
  //       unless generating LE116 code.
  // 'y' : Equivalent to r; retained for
  //       backwards compatibility.
  // 'f' : Floating Point registers.
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
      default : break;
      case 'd':
      case 'y':
      case 'f':
        return C_RegisterClass;
        break;
    }
  }
  return TargetLowering::getConstraintType(Constraint);
}

/// Examine constraint type and operand type and determine a weight value.
/// This object must already have been set up with the operand type
/// and the current alternative constraint selected.
TargetLowering::ConstraintWeight
LE1TargetLowering::getSingleConstraintMatchWeight(
    AsmOperandInfo &info, const char *constraint) const {
  ConstraintWeight weight = CW_Invalid;
  Value *CallOperandVal = info.CallOperandVal;
    // If we don't have a value, we can't do a match,
    // but allow it at the lowest weight.
  if (CallOperandVal == NULL)
    return CW_Default;
  Type *type = CallOperandVal->getType();
  // Look at the constraint type.
  switch (*constraint) {
  default:
    weight = TargetLowering::getSingleConstraintMatchWeight(info, constraint);
    break;
  case 'd':
  case 'y':
    if (type->isIntegerTy())
      weight = CW_Register;
    break;
  case 'f':
    if (type->isFloatTy())
      weight = CW_Register;
    break;
  }
  return weight;
}

/// Given a register class constraint, like 'r', if this corresponds directly
/// to an LLVM register class, return a register of 0 and the register class
/// pointer.
std::pair<unsigned, const TargetRegisterClass*> LE1TargetLowering::
getRegForInlineAsmConstraint(const std::string &Constraint, EVT VT) const
{
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    case 'd': // Address register. Same as 'r' unless generating LE116 code.
    case 'y': // Same as 'r'. Exists for compatibility.
    case 'r':
      return std::make_pair(0U, &LE1::CPURegsRegClass);
    case 'f':
      //if (VT == MVT::f32)
        //return std::make_pair(0U, LE1::FGR32RegisterClass);
      //if (VT == MVT::f64)
        //if ((!Subtarget->isSingleFloat()) && (!Subtarget->isFP64bit()))
          //return std::make_pair(0U, LE1::AFGR64RegisterClass);
      break;
    }
  }
  return TargetLowering::getRegForInlineAsmConstraint(Constraint, VT);
}

bool
LE1TargetLowering::isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const {
  // The LE1 target isn't yet aware of offsets.
  return false;
}

//bool LE1TargetLowering::isFPImmLegal(const APFloat &Imm, EVT VT) const {
  //if (VT != MVT::f32 && VT != MVT::f64)
    //return false;
  //if (Imm.isNegZero())
    //return false;
  //return Imm.isZero();
//}
