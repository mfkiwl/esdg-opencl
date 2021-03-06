set(LLVM_TARGET_DEFINITIONS LE1.td)

tablegen(LLVM LE1GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM LE1GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM LE1GenCodeEmitter.inc -gen-emitter)
tablegen(LLVM LE1GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM LE1GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM LE1GenCallingConv.inc -gen-callingconv)
tablegen(LLVM LE1GenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM LE1GenDFAPacketizer.inc -gen-dfa-packetizer)
add_public_tablegen_target(LE1CommonTableGen)

add_llvm_target(LE1CodeGen
  LE1AsmPrinter.cpp
  LE1BBMerger.cpp
  LE1CFGOptimiser.cpp
  LE1ExpandPredSpillCode.cpp
  LE1InstrInfo.cpp
  LE1VLIWPacketizer.cpp
  LE1ISelDAGToDAG.cpp
  LE1ISelLowering.cpp
  LE1FrameLowering.cpp
  LE1MCInstLower.cpp
  LE1RegisterInfo.cpp
  LE1Subtarget.cpp
  LE1TargetMachine.cpp
  LE1TargetTransformInfo.cpp
  LE1SelectionDAGInfo.cpp
  )


add_subdirectory(InstPrinter)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
