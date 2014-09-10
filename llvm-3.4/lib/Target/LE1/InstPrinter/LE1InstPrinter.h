//===-- LE1InstPrinter.h - Convert LE1 MCInst to assembly syntax --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints a LE1 MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#ifndef LE1INSTPRINTER_H
#define LE1INSTPRINTER_H
#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class TargetMachine;
class LE1MCInst;

class LE1InstPrinter : public MCInstPrinter {
public:
  LE1InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                 const MCRegisterInfo &MRI) :
    MCInstPrinter(MAI, MII, MRI) {}

  // Autogenerated by tblgen.
  void printInstruction(const MCInst *MI, raw_ostream &O);
  //static const char *getInstructionName(unsigned Opcode);
  static const char *getRegisterName(unsigned RegNo);

  //virtual StringRef getOpcodeName(unsigned Opcode) const;
  virtual void printRegName(raw_ostream &OS, unsigned RegNo) const;
  virtual void printInst(const MCInst *MI, raw_ostream &O, StringRef Annot);

private:
  void printInst(const LE1MCInst *MI, raw_ostream &O, StringRef Annot);
  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  void printUnsignedImm(const MCInst *MI, int opNum, raw_ostream &O);
  void printMemOperand(const MCInst *MI, int opNum, raw_ostream &O);
  void printMemOperandEA(const MCInst *MI, int opNum, raw_ostream &O);
  void printGlobalOffset(const MCInst *MI, int opNum, raw_ostream &O);
  void printFCCOperand(const MCInst *MI, int opNum, raw_ostream &O);
};
} // end namespace llvm

#endif