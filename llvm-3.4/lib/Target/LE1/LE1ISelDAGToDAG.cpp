//===-- LE1ISelDAGToDAG.cpp - A dag to dag inst selector for LE1 --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the LE1 target.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "le1-isel"
#include "LE1.h"
#include "LE1MachineFunction.h"
#include "LE1RegisterInfo.h"
#include "LE1Subtarget.h"
#include "LE1TargetMachine.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>

using namespace llvm;

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// LE1DAGToDAGISel - LE1 specific code to select LE1 machine
// instructions for SelectionDAG operations.
//===----------------------------------------------------------------------===//
namespace {

class LE1DAGToDAGISel : public SelectionDAGISel {
  /// Subtarget - Keep a pointer to the LE1Subtarget around so that we can
  /// make the right decision when generating code for different targets.
  const LE1Subtarget &Subtarget;

  /// TM - Keep a reference to LE1TargetMachine.
  LE1TargetMachine &TM;
  const LE1InstrInfo *TII;

public:
  explicit LE1DAGToDAGISel(LE1TargetMachine &tm) :
  SelectionDAGISel(tm),
  Subtarget(tm.getSubtarget<LE1Subtarget>()),
  TM(tm),
  TII(static_cast<const LE1InstrInfo*>(TM.getInstrInfo())) {
  }

  // Pass Name
  virtual const char *getPassName() const {
    return "LE1 DAG->DAG Pattern Instruction Selection";
  }


private:
  // Include the pieces autogenerated from the target description.
  #include "LE1GenDAGISel.inc"

  /// getTargetMachine - Return a reference to the TargetMachine, casted
  /// to the target-specific type.
  const LE1TargetMachine &getTargetMachine() {
    return static_cast<const LE1TargetMachine &>(TM);
  }

  /// getInstrInfo - Return a reference to the TargetInstrInfo, casted
  /// to the target-specific type.
  const LE1InstrInfo *getInstrInfo() {
    return getTargetMachine().getInstrInfo();
  }

  //SDNode *getGlobalBaseReg();
  SDNode *Select(SDNode *N);

  // Complex Pattern.
  bool SelectAddri8(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectAddri12(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectAddri32(SDValue Addr, SDValue &Base, SDValue &Offset);

  // getI32Imm - Return a target constant with the specified
  // value, of type i32.
  inline SDValue getI32Imm(unsigned Imm) {
    return CurDAG->getTargetConstant(Imm, MVT::i32);
  }

  virtual bool SelectInlineAsmMemoryOperand(const SDValue &Op,
                                            char ConstraintCode,
                                            std::vector<SDValue> &OutOps);
};

}

bool LE1DAGToDAGISel::SelectAddri8(SDValue Addr, SDValue &Base,
                                    SDValue &Offset) {
  EVT ValTy = Addr.getValueType();

  if (CurDAG->isBaseWithConstantOffset(Addr)) {
    ConstantSDNode *CN = cast<ConstantSDNode>(Addr.getOperand(1));
    if (CN->getZExtValue() == (CN->getZExtValue() & 0xFF)) {
      Base = Addr.getOperand(0);
      Offset = Addr.getOperand(1);
      return true;
    }
  }
  if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base   = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
    Offset = CurDAG->getTargetConstant(0, ValTy);
    return true;
  }
  return false;
}

bool LE1DAGToDAGISel::SelectAddri12(SDValue Addr, SDValue &Base,
                                    SDValue &Offset) {
  if (CurDAG->isBaseWithConstantOffset(Addr)) {
    ConstantSDNode *CN = cast<ConstantSDNode>(Addr.getOperand(1));
    if (CN->getZExtValue() == (CN->getZExtValue() & 0xFFF)) {
      Base = Addr.getOperand(0);
      Offset = Addr.getOperand(1);
      return true;
    }
  }
  return false;
}

bool LE1DAGToDAGISel::SelectAddri32(SDValue Addr, SDValue &Base,
                                    SDValue &Offset) {
  DEBUG(dbgs() << "SelectAddri32 with " << Addr.getNode()->getOperationName()
        << "\n");
  SDLoc dl(Addr.getNode());
  if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr))
    return false;
  if (GlobalAddressSDNode *GA = cast<GlobalAddressSDNode>(Addr)) {
    Base = CurDAG->getTargetGlobalAddress(GA->getGlobal(), dl, MVT::i32, 0);
    Offset = CurDAG->getTargetConstant(GA->getOffset(), MVT::i32);
    return true;
  }
  return false;
}

/// Select instructions not customized! Used for
/// expanded, promoted and normal instructions
SDNode* LE1DAGToDAGISel::Select(SDNode *Node) {

  unsigned Opcode = Node->getOpcode();
  SDLoc dl(Node);

  // Dump information about the Node being selected
  DEBUG(dbgs() << "Selecting:" << Opcode << "\n");
  DEBUG(Node->dump(CurDAG));

  switch(Opcode) {
  default:
    // Select the default instruction
    return SelectCode(Node);
    break;
    // FIXME ADDE should take a valid carry flag. This is an expanded op
    // in TargetLowering, should try to make it custom
  case ISD::ADDE: {
    /*
    // Lower ADDC to ADDCG but with zero moved in BReg
    SDValue InFlag = Node->getOperand(2);
    SDValue CmpLHS = Node->getOperand(0);
    SDValue LHS = Node->getOperand(0);
    SDValue RHS = Node->getOperand(1);
    EVT VT = LHS.getValueType();

    SDNode *Carry = CurDAG->getMachineNode(LE1::CMPLTU, dl, VT,
                                           CmpLHS, InFlag.getOperand(1));
    SDNode *AddCarry = CurDAG->getMachineNode(LE1::ADD, dl, VT,
                                              SDValue(Carry,0), RHS);
    return CurDAG->SelectNodeTo(Node, LE1::ADD, VT, MVT::Glue,
                                LHS, SDValue(AddCarry,0));
    */
    break;
  }
  case LE1ISD::ADDCG: {
    SDValue LHS = Node->getOperand(0);
    SDValue RHS = Node->getOperand(1);
    SDValue Cin = Node->getOperand(2);
    return CurDAG->getMachineNode(LE1::ADDCG, dl, MVT::i32, MVT::i1, 
                                  LHS, RHS, Cin);
    break;
  }
  case LE1ISD::DIVS: {
    SDValue LHS = Node->getOperand(0);
    SDValue RHS = Node->getOperand(1);
    SDValue Cin = Node->getOperand(2);
    return CurDAG->getMachineNode(LE1::DIVS, dl, MVT::i32, MVT::i1,
                                  LHS, RHS, Cin);
    break;
  }
    //FIXME instructions should be given node names so selection is automatic
  case LE1ISD::MTB:
    return CurDAG->getMachineNode(LE1::MTB, dl, MVT::i1, Node->getOperand(0));
    break;
 case LE1ISD::MFB:
    return CurDAG->getMachineNode(LE1::MFB, dl, MVT::i32, Node->getOperand(0));
    break;
  case LE1ISD::SET_ATTR: {
    SDValue Chain = Node->getOperand(0);
    SDValue Value = Node->getOperand(1);
    SDValue Addr = Node->getOperand(2);
    SDNode *Result = CurDAG->getMachineNode(LE1::SetAttr, dl, MVT::Other,
                                            Value, Addr, Chain);
    //ReplaceUses(Node, Result);
    return Result;
    break;
  }
  case LE1ISD::NUM_CORES:
    return CurDAG->getMachineNode(LE1::LE1_NUM_CORES, dl, MVT::i32);
  }
  return SelectCode(N);
}

bool LE1DAGToDAGISel::
SelectInlineAsmMemoryOperand(const SDValue &Op, char ConstraintCode,
                             std::vector<SDValue> &OutOps) {
  assert(ConstraintCode == 'm' && "unexpected asm memory constraint");
  OutOps.push_back(Op);
  return false;
}

/// createLE1ISelDag - This pass converts a legalized DAG into a
/// LE1-specific DAG, ready for instruction scheduling.
FunctionPass *llvm::createLE1ISelDag(LE1TargetMachine &TM) {
  return new LE1DAGToDAGISel(TM);
}