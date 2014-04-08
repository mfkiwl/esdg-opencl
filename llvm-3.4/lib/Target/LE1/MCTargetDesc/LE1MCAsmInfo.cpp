//===-- LE1MCAsmInfo.cpp - LE1 asm properties ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the LE1MCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "LE1MCAsmInfo.h"
#include "llvm/ADT/Triple.h"
#include <iostream>

using namespace llvm;

void LE1MCAsmInfo::anchor() {}

LE1MCAsmInfo::LE1MCAsmInfo(const MCRegisterInfo &MRI, StringRef TT) {
  std::cout << "Constructed LE1MCAsmInfo" << std::endl;

  Triple TheTriple(TT);
  IsLittleEndian = false;
  // TODO check these
  AlignmentIsInBytes          = false;
  Data16bitsDirective         = "\t.2byte\t";
  Data32bitsDirective         = "\t.4byte\t";
  Data64bitsDirective         = 0;
  PrivateGlobalPrefix         = "$";
  CommentString               = "#";
  ZeroDirective               = "\t.space\t";
  GPRel32Directive            = "\t.gpword\t";
  WeakRefDirective            = "\t.weak\t";

  SupportsDebugInformation = true;

  ExceptionsType = ExceptionHandling::None;
  HasLEB128 = true;
  DwarfRegNumForCFI = true;
}