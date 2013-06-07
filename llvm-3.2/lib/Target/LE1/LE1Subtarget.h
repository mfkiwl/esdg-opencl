//=====-- LE1Subtarget.h - Define Subtarget for the LE1 -----*- C++ -*--====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the LE1 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LE1SUBTARGET_H
#define LE1SUBTARGET_H

#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/MC/MCInstrItineraries.h"
#include <string>
#include <iostream>

#define GET_SUBTARGETINFO_HEADER
#include "LE1GenSubtargetInfo.inc"

namespace llvm {
class StringRef;

class LE1Subtarget : public LE1GenSubtargetInfo {

public:

  enum LE1ArchEnum {
    Scalar,
    ScalarExpandDiv
  };

  // LE1 architecture version
  LE1ArchEnum LE1ArchVersion;

  InstrItineraryData InstrItins;

  bool HasExpandDiv;
  bool NeedsNops;

public:

  /// This constructor initializes the data members to match that
  /// of the specified triple.
  LE1Subtarget(const std::string &TT, const std::string &CPU,
                const std::string &FS);// bool little);

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef FS);

  const InstrItineraryData *getInstrItineraryData() const 
  {  return &InstrItins; }

  const LE1ArchEnum &getLE1ArchVersion() const { return LE1ArchVersion; }

  bool hasExpandDiv() const { return HasExpandDiv; }
  bool needsNops() const { return NeedsNops; }
};
} // End llvm namespace

#endif
