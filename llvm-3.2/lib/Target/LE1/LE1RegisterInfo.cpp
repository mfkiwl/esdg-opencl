//===- LE1RegisterInfo.cpp - LE1 Register Information -== -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the LE1 implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "le1-reg-info"

#include "LE1.h"
#include "LE1Subtarget.h"
#include "LE1RegisterInfo.h"
#include "LE1MachineFunction.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
//#include "llvm/Analysis/DebugInfo.h"

#define GET_REGINFO_TARGET_DESC
#include "LE1GenRegisterInfo.inc"

using namespace llvm;

LE1RegisterInfo::LE1RegisterInfo(const LE1Subtarget &ST,
                                   const TargetInstrInfo &tii)
  : LE1GenRegisterInfo(LE1::L0), Subtarget(ST), TII(tii) {}

//unsigned LE1RegisterInfo::getPICCallReg() { return LE1::T9; }

//===----------------------------------------------------------------------===//
// Callee Saved Registers methods
//===----------------------------------------------------------------------===//

/// LE1 Callee Saved Registers
const uint16_t* LE1RegisterInfo::
getCalleeSavedRegs(const MachineFunction *MF) const
{
  // LE1 callee-save register range is $16-$23, $f20-$f30
  static const uint16_t CalleeSavedRegs[] = {
    //LE1::F31, LE1::F30, LE1::F29, LE1::F28, LE1::F27, LE1::F26,
    //LE1::F25, LE1::F24, LE1::F23, LE1::F22, LE1::F21, LE1::F20,
    //LE1::RA, LE1::FP, LE1::S7, LE1::S6, LE1::S5, LE1::S4,
    //LE1::S3, LE1::S2, LE1::S1, LE1::S0, 0
    LE1::L0, LE1::TS0, LE1::TS1, LE1::TS2, LE1::TS3,
    LE1::TS4, LE1::TS5, LE1::TS6, 0
  };

  return CalleeSavedRegs;
}

BitVector LE1RegisterInfo::
getReservedRegs(const MachineFunction &MF) const {
  static const unsigned ReservedCPURegs[] = {
    //LE1::ZERO, LE1::AT, LE1::K0, LE1::K1, 
    //LE1::GP, LE1::SP, LE1::FP, LE1::RA, 0
    LE1::ZERO, LE1::SP, LE1::L0, LE1::IdX, LE1::IdY, LE1::IdZ, LE1::T45, 0
  };

  BitVector Reserved(getNumRegs());
  typedef TargetRegisterClass::iterator RegIter;

  for (const unsigned *Reg = ReservedCPURegs; *Reg; ++Reg)
    Reserved.set(*Reg);

  return Reserved;
}

// This function eliminate ADJCALLSTACKDOWN,
// ADJCALLSTACKUP pseudo instructions
void LE1RegisterInfo::
eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I) const {
  // Simply discard ADJCALLSTACKDOWN, ADJCALLSTACKUP instructions.
  MBB.erase(I);
}

// FrameIndex represent objects inside a abstract stack.
// We must replace FrameIndex with an stack/frame pointer
// direct reference.
void LE1RegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                    RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  LE1FunctionInfo *LE1FI = MF.getInfo<LE1FunctionInfo>();

  unsigned i = 0;
  while (!MI.getOperand(i).isFI()) {
    ++i;
    assert(i < MI.getNumOperands() &&
           "Instr doesn't have FrameIndex operand!");
  }

  DEBUG(errs() << "\nFunction : " << MF.getFunction()->getName() << "\n";
        errs() << "<--------->\n" << MI);

  int FrameIndex = MI.getOperand(i).getIndex();
  int stackSize  = MF.getFrameInfo()->getStackSize();
  int spOffset   = MF.getFrameInfo()->getObjectOffset(FrameIndex);

  DEBUG(errs() << "FrameIndex : " << FrameIndex << "\n"
               << "spOffset   : " << spOffset << "\n"
               << "stackSize  : " << stackSize << "\n");

  const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();
  int MinCSFI = 0;
  int MaxCSFI = -1;

  if (CSI.size()) {
    MinCSFI = CSI[0].getFrameIdx();
    MaxCSFI = CSI[CSI.size() - 1].getFrameIdx();
  }

  // The following stack frame objects are always referenced relative to $sp:
  //  1. Outgoing arguments.
  //  2. Pointer to dynamically allocated stack space.
  //  3. Locations for callee-saved registers.
  // Everything else is referenced relative to whatever register
  // getFrameRegister() returns.
  unsigned FrameReg;

  if (LE1FI->isOutArgFI(FrameIndex) || LE1FI->isDynAllocFI(FrameIndex) ||
      (FrameIndex >= MinCSFI && FrameIndex <= MaxCSFI))
    FrameReg = LE1::SP;
  else
    FrameReg = getFrameRegister(MF);

  // Calculate final offset.
  // - There is no need to change the offset if the frame object is one of the
  //   following: an outgoing argument, pointer to a dynamically allocated
  //   stack space or a $gp restore location,
  // - If the frame object is any of the following, its offset must be adjusted
  //   by adding the size of the stack:
  //   incoming argument, callee-saved register location or local variable.  
  int Offset;

  if (LE1FI->isOutArgFI(FrameIndex) || LE1FI->isGPFI(FrameIndex) ||
      LE1FI->isDynAllocFI(FrameIndex))
    Offset = spOffset;
  else
    Offset = spOffset + stackSize;

  Offset    += MI.getOperand(i+1).getImm();

  DEBUG(errs() << "Offset     : " << Offset << "\n" << "<--------->\n");

  // If MI is not a debug value, make sure Offset fits in the 16-bit immediate
  // field. 
  //if (!MI.isDebugValue() && (Offset >= 0x8000 || Offset < -0x8000)) {
    //MachineBasicBlock &MBB = *MI.getParent();
    //DebugLoc DL = II->getDebugLoc();
    //int ImmHi = (((unsigned)Offset & 0xffff0000) >> 16) +
      //          ((Offset & 0x8000) != 0);

    // FIXME: change this when le1 goes MC".
    //BuildMI(MBB, II, DL, TII.get(LE1::NOAT));
    //BuildMI(MBB, II, DL, TII.get(LE1::LUi), LE1::T45).addImm(ImmHi);
    //BuildMI(MBB, II, DL, TII.get(LE1::ADDu), LE1::T45).addReg(FrameReg)
      //                                                 .addReg(LE1::T45);
    //FrameReg = LE1::T45;
    //Offset = (short)(Offset & 0xffff);

    //BuildMI(MBB, ++II, MI.getDebugLoc(), TII.get(LE1::ATMACRO));
  //}

  MI.getOperand(i).ChangeToRegister(FrameReg, false);
  MI.getOperand(i+1).ChangeToImmediate(Offset);
}

unsigned LE1RegisterInfo::
getFrameRegister(const MachineFunction &MF) const {
  //const TargetFrameLowering *TFI = MF.getTarget().getFrameLowering();

  //return TFI->hasFP(MF) ? LE1::STRP : LE1::SP;
  return LE1::SP;
}

unsigned LE1RegisterInfo::
getEHExceptionRegister() const {
  llvm_unreachable("What is the exception register");
  return 0;
}

unsigned LE1RegisterInfo::
getEHHandlerRegister() const {
  llvm_unreachable("What is the exception handler register");
  return 0;
}
