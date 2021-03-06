#include "LE1TargetMachine.h"
#include "LE1Subtarget.h"
#include "LE1MachineFunction.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/LatencyPriorityQueue.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/ScheduleHazardRecognizer.h"
#include "llvm/CodeGen/SchedulerRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"

#include <iostream>

using namespace llvm;

namespace {

  class LE1ExpandPredSpillCode : public MachineFunctionPass {
    LE1TargetMachine &TM;
    const LE1Subtarget &ST;

  public:
    static char ID;
    LE1ExpandPredSpillCode(LE1TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm), ST(*tm.getSubtargetImpl()) {}

    const char *getPassName() const {
      return "LE1 Expand Predicate Spill code";
    }
    bool runOnMachineFunction(MachineFunction &MF);
  };

  char LE1ExpandPredSpillCode::ID = 0;

bool LE1ExpandPredSpillCode::runOnMachineFunction(MachineFunction &MF) {

  const LE1InstrInfo *TII = TM.getInstrInfo();
  unsigned TmpReg = LE1::T45;

  // Loop over all the basic blocks
  for(MachineFunction::iterator MBBb = MF.begin(), MBBe = MF.end();
      MBBb != MBBe; ++ MBBb) {
    MachineBasicBlock* MBB = MBBb;

    // Traverse the basic block
    for(MachineBasicBlock::iterator MII = MBB->begin(); MII!= MBB->end();
        ++MII) {
      MachineInstr *MI = MII;

      if (!MI->isPseudo())
        continue;

      DebugLoc DL = MI->getDebugLoc();

      int Opc = MI->getOpcode();
      if(Opc == LE1::STW_PRED) {
        unsigned numOps = MI->getNumOperands();
        assert(MI->getOperand(0).isReg() && "Not Register. Store");
        int SrcReg = MI->getOperand(0).getReg();
        assert(LE1::BRegsRegClass.contains(SrcReg) &&
               "Src is not a Branch Register");
        assert(MI->getOperand(1).isReg() && "Not a Register. Store");
        unsigned FP = MI->getOperand(1).getReg();
        assert(FP == TM.getRegisterInfo()->getFrameRegister(MF) &&
               "Operand 1 is not FrameReg");
        assert(MI->getOperand(2).isImm() && "Operand 2 Not offset");
        int FI = MI->getOperand(2).getImm();


        MII = MBB->erase(MI);
        // These pseudo insts do not seem to get eliminated, so sometimes
        // it's necessary to delete them here
        if (MII->getOpcode() == LE1::STW_PRED) {
          int SrcReg2 = MII->getOperand(0).getReg();
          unsigned FP2 = MII->getOperand(1).getReg();
          int FI2 = MII->getOperand(2).getImm();
          if ( (SrcReg == SrcReg2) && (FP == FP2) && (FI == FI2))
            MII = MBB->erase(MII);
        }
        BuildMI(*MBB, MII, DL, TII->get(LE1::MFB), TmpReg)
          .addReg(SrcReg);
        BuildMI(*MBB, MII, DL, TII->get(LE1::STW)).addReg(TmpReg)
                .addReg(FP).addImm(FI);
      }
      else if(Opc == LE1::LDW_PRED) {

        assert(MI->getOperand(0).isReg() && "Not a register"); 
        int DstReg = MI->getOperand(0).getReg();
        assert(LE1::BRegsRegClass.contains(DstReg) &&
               "Not a Predicate Register");
        assert(MI->getOperand(1).isReg() && "Not a register");
        unsigned FP = MI->getOperand(1).getReg();
        assert(FP == TM.getRegisterInfo()->getFrameRegister(MF) &&
               "Not a Frame Pointer");
        assert(MI->getOperand(2).isImm() && "Not a Frame Index");
        int FI = MI->getOperand(2).getImm();

        MII = MBB->erase(MI);
        if (MII->getOpcode() == LE1::STW_PRED) {
          int SrcReg = MII->getOperand(0).getReg();
          int FI2 = MII->getOperand(2).getImm();
          if ((SrcReg == DstReg) && (FI == FI2))
            MII = MBB->erase(MII);
        }
        BuildMI(*MBB, MII, DL, TII->get(LE1::LDW), TmpReg)
                .addReg(FP).addImm(FI);
        BuildMI(*MBB, MII, DL, TII->get(LE1::MTB), DstReg).addReg(TmpReg);
      }
    }
  }

  return true;
}

} // namespace

FunctionPass *llvm::createLE1ExpandPredSpillCode(LE1TargetMachine &TM) {
  return new LE1ExpandPredSpillCode(TM);
}
