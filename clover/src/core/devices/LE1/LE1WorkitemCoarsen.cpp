#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "LE1WorkitemCoarsen.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

typedef std::vector<Stmt*> StmtSet;
typedef std::vector<DeclRefExpr*> DeclRefSet;
typedef std::map<std::string, StmtSet> StmtSetMap;
typedef std::map<std::string, DeclRefSet> DeclRefSetMap;

bool WorkitemCoarsen::CreateWorkgroup(std::string &Filename) {
#ifdef DEBUGCL
  std::cerr << "CreateWorkgroup\n";
#endif
  OpenCLCompiler<KernelInitialiser> InitCompiler(LocalX, LocalY, LocalZ);
  OrigFilename = Filename;
  InitCompiler.setFile(OrigFilename);
  InitCompiler.Parse();

  // At this point the rewriter's buffer should be full with the rewritten
  // file contents.
  const RewriteBuffer *RewriteBuf = InitCompiler.getRewriteBuf();
  if (RewriteBuf == NULL)
    return false;

  // TODO Don't want to have to write the source string to a file before
  // passing it to this function.
  //clang_parseTranslationUnit from libclang provides the option to pass source
  // files via memory buffers ("unsaved_files" parameter).
  // You can "follow" its code path to see how this can be done programmatically

  InitKernelSource = std::string(RewriteBuf->begin(), RewriteBuf->end());
#ifdef DEBUGCL
  std::cerr << "Finished initialising workgroup\n";
#endif

  std::ofstream init_kernel;
  InitKernelFilename = std::string("init.");
  InitKernelFilename.append(OrigFilename);
  init_kernel.open(InitKernelFilename.c_str());
  init_kernel << InitKernelSource;
  init_kernel.close();
  return HandleBarriers();
}

bool WorkitemCoarsen::HandleBarriers() {
#ifdef DEBUGCL
  std::cerr << "HandleBarriers\n";
#endif
  OpenCLCompiler<ThreadSerialiser> SerialCompiler(LocalX, LocalY, LocalZ);
  SerialCompiler.setFile(InitKernelFilename);
  SerialCompiler.Parse();

  const RewriteBuffer *RewriteBuf = SerialCompiler.getRewriteBuf();
  if (RewriteBuf == NULL) {
    FinalKernel = InitKernelSource;
    return true;
  }

  //std::ofstream final_kernel;
  //FinalFilename = InitFilename.append(".final.cl");
  //final_kernel.open(FinalFilename.c_str());
  FinalKernel = std::string(RewriteBuf->begin(), RewriteBuf->end());
  //final_kernel.close();

#ifdef DEBUGCL
  std::cerr << "Finalised kernel:\n";
#endif

  return true;
}

//template <typename T>
//OpenCLCompiler<T>::KernelInitialiser::KernelInitialiser(Rewriter &R,
WorkitemCoarsen::KernelInitialiser::KernelInitialiser(Rewriter &R,
                                                        unsigned x,
                                                        unsigned y,
                                                        unsigned z)
    : LocalX(x), LocalY(y), LocalZ(z), TheRewriter(R) {

  if (LocalZ > 1) {
    OpenWhile << "\n__kernel_local_id[2] = 0;\n";
    OpenWhile  << "while (__kernel_local_id[2] < " << LocalZ << ") {\n";
  }
  if (LocalY > 1) {
    OpenWhile << "__kernel_local_id[1] = 0;\n";
    OpenWhile << "while (__kernel_local_id[1] < " << LocalY << ") {\n";
  }
  if (LocalX > 1) {
    OpenWhile << "__kernel_local_id[0] = 0;\n";
    OpenWhile << "while (__kernel_local_id[0] < " << LocalX << ") {\n";
  }

  if (LocalX > 1) {
    CloseWhile << "\n__kernel_local_id[0]++;\n";
    CloseWhile  << "}\n";
  }
  if (LocalY > 1) {
    CloseWhile << " __kernel_local_id[1]++;\n";
    CloseWhile << "}\n";
  }
  if (LocalZ > 1) {
    CloseWhile << "__kernel_local_id[2]++;\n";
    CloseWhile << "}\n";
  }
}

//template <typename T>
//void OpenCLCompiler<T>::KernelInitialiser::CloseLoop(SourceLocation Loc) {
void WorkitemCoarsen::KernelInitialiser::CloseLoop(SourceLocation Loc) {
  TheRewriter.InsertText(Loc, CloseWhile.str(), true, true);
}

//template <typename T>
void WorkitemCoarsen::KernelInitialiser::OpenLoop(SourceLocation Loc) {
  TheRewriter.InsertText(Loc, OpenWhile.str(), true, true);
}

//template <typename T>
bool WorkitemCoarsen::KernelInitialiser::VisitFunctionDecl(FunctionDecl *f) {
  // Only function definitions (with bodies), not declarations.
  if (f->hasBody()) {
    Stmt *FuncBody = f->getBody();
    SourceLocation FuncBodyStart =
    FuncBody->getLocStart().getLocWithOffset(2);

    std::stringstream FuncBegin;
    FuncBegin << "  int __kernel_local_id[";
    if (LocalZ && LocalY && LocalX)
      FuncBegin << "3";
    else if (LocalY && LocalX)
      FuncBegin << "2";
    else
      FuncBegin << "1";
    FuncBegin << "];\n";

    TheRewriter.InsertText(FuncBodyStart, FuncBegin.str(), true, true);
    OpenLoop(FuncBodyStart);
    CloseLoop(FuncBody->getLocEnd());
  }
  return true;
}

// If there are grouped declarations, split them into individual decls.
bool WorkitemCoarsen::KernelInitialiser::VisitDeclStmt(Stmt *s) {
  DeclStmt *DS = cast<DeclStmt>(s);
  if (DS->isSingleDecl())
    return true;
  else {
    // Remove the original declarations
    TheRewriter.RemoveText(s->getSourceRange());

    // Iterate through the group, splitting each into individual stmts.
    NamedDecl *ND = NULL;
    DeclGroupRef::iterator DE = DS->decl_end();
    --DE;

    for (DeclGroupRef::iterator DI = DS->decl_begin(); DI != DE; ++DI) {
      ND = cast<NamedDecl>((*DI));
      std::string key = ND->getName().str();
#ifdef DEBUGCL
      std::cerr << ND->getName().str() << " is declared\n";
#endif

      std::string type = cast<ValueDecl>(ND)->getType().getAsString();
      std::stringstream newDecl;
      newDecl << "\n" << type << " " << key << ";";
      TheRewriter.InsertText(ND->getLocStart(), newDecl.str(), true);
    }

    // Handle the last declaration which could also be initialised.
    DeclStmt::reverse_decl_iterator Last = DS->decl_rbegin();
    ND = cast<NamedDecl>(*Last);
    std::string key = ND->getName().str();
    std::string type = cast<ValueDecl>(ND)->getType().getAsString();
    std::stringstream newDecl;
    newDecl << "\n" << type << " " << key;
    TheRewriter.InsertText(ND->getLocStart(), newDecl.str(), true);

    if (s->child_begin() == s->child_end()) {
      TheRewriter.InsertText(ND->getLocStart(), ";");
      return true;
    }

    for (Stmt::child_iterator SI = s->child_begin(), SE = s->child_end();
         SI != SE; ++SI) {
      std::stringstream init;
      init << " = " << TheRewriter.ConvertToString(*SI) << ";";
      TheRewriter.InsertText(ND->getLocStart(), init.str(), true);
    }
  }
  return true;
}

//template <typename T>
bool WorkitemCoarsen::KernelInitialiser::VisitCallExpr(Expr *s) {
  CallExpr *Call = cast<CallExpr>(s);
  FunctionDecl* FD = Call->getDirectCallee();
  DeclarationName DeclName = FD->getNameInfo().getName();
  std::string FuncName = DeclName.getAsString();

  // Modify any calls to get_global_id to use the generated local ids.
  IntegerLiteral *Arg;
  unsigned Index = 0;
  if ((FuncName.compare("get_global_id") == 0) ||
      (FuncName.compare("get_local_id") == 0) ||
      (FuncName.compare("get_local_size") == 0)) {

    if (IntegerLiteral::classof(Call->getArg(0))) {
      Arg = cast<IntegerLiteral>(Call->getArg(0));
    }
    else if (ImplicitCastExpr::classof(Call->getArg(0))) {
      Expr *Cast = cast<ImplicitCastExpr>(Call->getArg(0))->getSubExpr();
      Arg = cast<IntegerLiteral>(Cast);
    }
    else
      llvm_unreachable("unhandled argument type!");

    Index = Arg->getValue().getZExtValue();
  }
  if (FuncName.compare("get_global_id") == 0) {

    // Remove the semi-colon after the call
    TheRewriter.RemoveText(Call->getLocEnd().getLocWithOffset(1), 1);

    switch(Index) {
    default:
      break;
    case 0:
      TheRewriter.InsertText(Call->getLocEnd().getLocWithOffset(2),
                             " + __kernel_local_id[0];\n", true, true);
      break;
    case 1:
      TheRewriter.InsertText(Call->getLocEnd().getLocWithOffset(2),
                             " + __kernel_local_id[1];\n", true, true);
      break;
    case 2:
      TheRewriter.InsertText(Call->getLocEnd().getLocWithOffset(2),
                             " + __kernel_local_id[2];\n", true, true);
      break;
    }
  }
  else if (FuncName.compare("get_local_id") == 0) {
    TheRewriter.RemoveText(Call->getSourceRange());
    if (Index == 0)
      TheRewriter.InsertText(Call->getLocEnd().getLocWithOffset(1),
                             "__kernel_local_id[0]");
    else if (Index == 1)
      TheRewriter.InsertText(Call->getLocEnd().getLocWithOffset(1),
                             "__kernel_local_id[1]");
    else
      TheRewriter.InsertText(Call->getLocEnd().getLocWithOffset(1),
                             "__kernel_local_id[2]");

  }
  else if (FuncName.compare("get_local_size") == 0) {
    TheRewriter.RemoveText(Call->getSourceRange());
    std::stringstream local;
    if (Index == 0)
      local << LocalX;
    else if (Index == 1)
      local << LocalY;
    else
      local << LocalZ;

    TheRewriter.InsertText(Call->getLocStart(), local.str());

  }

  return true;
}

//template <typename T>
WorkitemCoarsen::ThreadSerialiser::ThreadSerialiser(Rewriter &R,
                                   unsigned x,
                                   unsigned y,
                                   unsigned z)
        : LocalX(x), LocalY(y), LocalZ(z), TheRewriter(R) {
  if (LocalZ > 1) {
    OpenWhile << "\n__kernel_local_id[2] = 0;\n";
    OpenWhile  << "while (__kernel_local_id[2] < " << LocalZ << ") {\n";

    SaveIDs << "__kernel_local_id_save[2] = __kernel_local_id[2];\n";
    RestoreIDs << "__kernel_local_id[2] = __kernel_local_id_save[2];\n";
  }
  if (LocalY > 1) {
    OpenWhile << "__kernel_local_id[1] = 0;\n";
    OpenWhile << "while (__kernel_local_id[1] < " << LocalY << ") {\n";

    SaveIDs << "__kernel_local_id_save[1] = __kernel_local_id[1];\n";
    RestoreIDs << "__kernel_local_id[1] = __kernel_local_id_save[1];\n";
  }
  if (LocalX > 1) {
    OpenWhile << "__kernel_local_id[0] = 0;\n";
    OpenWhile << "while (__kernel_local_id[0] < " << LocalX << ") {\n";

    SaveIDs << "__kernel_local_id_save[0] = __kernel_local_id[0];\n";
    RestoreIDs << "__kernel_local_id[0] = __kernel_local_id_save[0];\n";
  }
  if (LocalX > 1) {
    CloseWhile << "\n__kernel_local_id[0]++;\n";
    CloseWhile  << "}\n";
  }
  if (LocalY > 1) {
    CloseWhile << " __kernel_local_id[1]++;\n";
    CloseWhile << "}\n";
  }
  if (LocalZ > 1) {
    CloseWhile << "__kernel_local_id[2]++;\n";
    CloseWhile << "}\n";
  }

  LocalArray << "  int __kernel_local_id[";
  SavedLocalArray << "  int __kernel_local_id_save[";
  if (LocalZ > 1 && LocalY > 1 && LocalX > 1) {
    LocalArray << "3";
    SavedLocalArray << "3";
  }
  else if (LocalY > 1 && LocalX > 1) {
    LocalArray << "2";
    SavedLocalArray << "2";
  }
  else {
    LocalArray << "1";
    SavedLocalArray << "1";
  }
  LocalArray << "];\n";
  SavedLocalArray << "];\n";

}

void WorkitemCoarsen::ThreadSerialiser::CloseLoop(SourceLocation Loc) {
    TheRewriter.InsertText(Loc, CloseWhile.str(), true, true);
}

void WorkitemCoarsen::ThreadSerialiser::OpenLoop(SourceLocation Loc) {
  TheRewriter.InsertText(Loc, OpenWhile.str(), true, true);
}

void WorkitemCoarsen::ThreadSerialiser::SaveLocalIDs(SourceLocation Loc) {
  TheRewriter.InsertText(Loc, SavedLocalArray.str());
  TheRewriter.InsertText(Loc, SaveIDs.str());
}

void WorkitemCoarsen::ThreadSerialiser::RestoreLocalIDs(SourceLocation Loc) {
  TheRewriter.InsertText(Loc, RestoreIDs.str());
}

void WorkitemCoarsen::ThreadSerialiser::CreateLocalVariable(DeclRefExpr *Ref,
                                                            bool ScalarRepl) {
  NamedDecl *ND = Ref->getDecl();
  std::string varName = ND->getName().str();

  // Do not create local variables for variables who have limited scope,
  // such as one declared in loop headers
  if (ScopedVariables.find(varName) != ScopedVariables.end())
    return;

#ifdef DEBUGCL
  std::cerr << "Creating local variable for " << varName << std::endl;
#endif

  // Make sure we don't add the same value more than once
  if (NewLocalDecls.find(varName) != NewLocalDecls.end()) {
#ifdef DEBUGCL
    std::cerr << "But already added a new declaration for it\n";
#endif
    return;
  }

  std::string type = cast<ValueDecl>(ND)->getType().getAsString();
  std::stringstream NewDecl;
  NewDecl << type << " " << varName;

  // For variables that are live across loop boundaries, we use scalar
  // replication to hold the values of each work-item. This requires us
  // to retrospectively look back at the previous references and turn them
  // into array accesses as well as just declaring the variables as an array.
  if (ScalarRepl) {
#ifdef DEBUGCL
    std::cerr << "Declaring it as an array\n";
#endif
    if (LocalX != 0)
      NewDecl << "[" << LocalX << "]";
    if (LocalY > 1)
      NewDecl << "[" << LocalY << "]";
    if (LocalZ > 1)
      NewDecl << "[" << LocalZ << "]";
    NewScalarRepls.insert(std::make_pair(varName, ND));

    // Visit all the references of this variable
    DeclRefSet varRefs = AllRefs[varName];
    for (std::vector<DeclRefExpr*>::iterator RI = varRefs.begin(),
         RE = varRefs.end(); RI != RE; ++RI) {
      AccessScalar(*RI);
    }
  }
  else {
    NewLocalDecls.insert(std::make_pair(varName, ND));
  }
  NewDecl << ";\n";

  // Declare the variable
  TheRewriter.InsertText(FuncStart, NewDecl.str(), true, true);

  // Remove the old declaration; if it wasn't initialised, remove the whole
  // statement and not just the type declaration.
  if (DeclStmts.find(varName) != DeclStmts.end()) {
    DeclStmt *DS = DeclStmts[varName];
#ifdef DEBUGCL
    std::cerr << "Removing old declaration of " << ND->getName().str()
      << std::endl;
    //DS->dumpAll();
#endif

    if (DS->child_begin() != DS->child_end()) {
      TheRewriter.RemoveText(ND->getLocStart(), type.length());

      if(ScalarRepl)
        AccessScalar(DeclStmts[varName]->getSingleDecl());
    }
    else {
      // Remove the semi-colon?
      //TheRewriter.RemoveText(ND->getLocEnd());
      TheRewriter.RemoveText(DS->getSourceRange());
      //TheRewriter.RemoveText(ND->getSourceRange());
    }
    DeclStmts.erase(varName);
  }
}

void WorkitemCoarsen::ThreadSerialiser::AccessScalar(Decl *decl) {
  NamedDecl *ND = cast<NamedDecl>(decl);
  unsigned offset = ND->getName().str().length();
  offset += cast<ValueDecl>(ND)->getType().getAsString().length();
  // increment because of a space between type and name
  ++offset;
  SourceLocation Loc = ND->getLocStart().getLocWithOffset(offset);
  TheRewriter.InsertText(Loc, "[__kernel_local_id[0]]", true);
}

//template <typename T>
void WorkitemCoarsen::ThreadSerialiser::AccessScalar(DeclRefExpr *Ref) {
#ifdef DEBUGCL
  std::cerr << "Creating scalar access for "
      << Ref->getDecl()->getName().str() << std::endl;
#endif
  unsigned offset = Ref->getDecl()->getName().str().length();
  SourceLocation loc = Ref->getLocEnd().getLocWithOffset(offset);
  TheRewriter.InsertText(loc, "[__kernel_local_id[0]]", true);
}

void WorkitemCoarsen::ThreadSerialiser::CoarsenLoopBody(Stmt *LoopBody,
                                                        CallExpr *Barrier) {
  // Use an offset to account for '{'
  SourceLocation BeginLoc = LoopBody->getLocStart().getLocWithOffset(2);

  // First we need to save the existing __kernel_local_id values as we are
  // going to rewrite them to run another set of loops.
  SaveLocalIDs(BeginLoc);

  // Then we need to open the loop.
  OpenLoop(BeginLoc);

  // Then we close the loop at the location of the barrier and remove the call.
  CloseLoop(Barrier->getLocStart());
  // Then we restore the previous values of __kernel_local_id
  RestoreLocalIDs(Barrier->getLocStart());

  TheRewriter.InsertText(Barrier->getLocStart(), "//");

}

//template <typename T>
bool WorkitemCoarsen::ThreadSerialiser::BarrierInLoop(ForStmt* s) {
  Stmt* ForBody = cast<ForStmt>(s)->getBody();
  SourceLocation ForLoc = s->getLocStart();
  bool FoundBarriers = false;

  for (Stmt::child_iterator FI = ForBody->child_begin(),
       FE = ForBody->child_end(); FI != FE; ++FI) {

    // Recursively visit inner loops
    if (ForStmt* nested = dyn_cast_or_null<ForStmt>(*FI)) {
      if (BarrierInLoop(nested)) {
        LoopsWithBarrier.insert(std::make_pair(ForLoc, s));
        //LoopsToDistribute.push_back(s);
        FoundBarriers = true;
      }
    }
    else if (CallExpr* Call = dyn_cast_or_null<CallExpr>(*FI)) {
      FunctionDecl* FD = Call->getDirectCallee();
      DeclarationName DeclName = FD->getNameInfo().getName();
      std::string FuncName = DeclName.getAsString();

      if (FuncName.compare("barrier") == 0) {
        // Only record loops once, each loop may have several barriers within
        // it though.
        CoarsenLoopBody(s->getBody(), Call);

        if (LoopsToDistribute.empty()) {
          LoopsWithBarrier.insert(std::make_pair(ForLoc, s));
          //LoopsToDistribute.push_back(s);
        }
        // FIXME When does LoopsToDistribute get cleared? Do we need to do the
        // following checking..?
        //else if (LoopsToDistribute.front() != s){ }
        LoopBarrier = Call;
        LoopBarriers.insert(std::make_pair(Call->getLocStart(), Call));
        FoundBarriers = true;
      }
    }
  }
  if (!FoundBarriers)
    LoopsWithoutBarrier.insert(std::make_pair(ForLoc, s));

  return FoundBarriers;
}

// Check for barrier calls within loops, this is necessary to close the
// nested loops properly.
bool WorkitemCoarsen::ThreadSerialiser::VisitForStmt(Stmt *s) {
#ifdef DEBUGCL
  std::cerr << "VisitForStmt\n";
#endif
  ForStmt* ForLoop = cast<ForStmt>(s);
  SourceLocation ForLoc = ForLoop->getLocStart();

  // Check whether we've already visited the loop
  if (LoopsWithBarrier.find(ForLoc) != LoopsWithBarrier.end())
    return true;
  else if (LoopsWithoutBarrier.find(ForLoc) != LoopsWithoutBarrier.end())
    return true;

  return BarrierInLoop(ForLoop);

  if (BarrierInLoop(ForLoop)) {
    // LoopsToReplicate is a vector containing one or more (nested)
    // loops that need to be distributed.

    for (std::map<SourceLocation, CallExpr*>::iterator LBI
         = LoopBarriers.begin(), LBE = LoopBarriers.end(); LBI != LBE; ++LBI) {
      SourceLocation BarrierLoc = LBI->first;
      LoopBarrier = LBI->second;

      bool isFinal = false;
      Stmt::child_iterator Final;
      for (Stmt::child_iterator FI = ForLoop->getBody()->child_begin(),
           FE = ForLoop->getBody()->child_end(); FI != FE; ++FI) {
        isFinal = false;
        if (*FI == LoopBarrier) {
          std::cerr << "Found loop\n";
          isFinal = true;
          Final = FI;
        }
      }
      // Close the loop(s)
      //SourceLocation BarrierLoc = LoopBarrier->getLocStart();
      for (unsigned i = 0; i < LoopsToDistribute.size(); ++i)
        TheRewriter.InsertText(BarrierLoc, "}\n", true, true);


      if (isFinal) {
        CloseLoop(ForLoop->getBody()->getLocEnd().getLocWithOffset(2));
        OpenLoop(ForLoop->getBody()->getLocEnd().getLocWithOffset(2));
      }
      else {
        CloseLoop(BarrierLoc);
        OpenLoop(BarrierLoc);
      }

      // Initialise the new loop(s)
      while (!LoopsToDistribute.empty()) {
        ForStmt *Loop = LoopsToDistribute.back();
#ifdef DEBUGCL
        std::cerr << "Initialising new loop:\n";
#endif

        // If the loop header declares variables, we stop them from being made
        // local as their scope can die with the closing of the loop, as the
        // header will be duplicated again.

        // If the for loop use DeclRefExpr, we will need to move the
        // declarations into a global scope since the while loop was has just
        // been closed.
        bool isDeclStmt = false;
        if ((isa<DeclStmt>(Loop->getInit()))) {
#ifdef DEBUGCL
          std::cerr << "Loop init is single decl\n";
#endif
          DeclStmt *InitDecl = cast<DeclStmt>(Loop->getInit());
          if (InitDecl->isSingleDecl()) {
            NamedDecl *ND = cast<NamedDecl>(InitDecl->getSingleDecl());
            std::string key = ND->getName().str();
            ScopedVariables.insert(std::make_pair(key, ND));
          }
          isDeclStmt = true;
          // FIXME Need to handle multiple declarations!!
        }
        else {
#ifdef DEBUGCL
          std::cerr << "Loop init has multiple declarations\n";
#endif
          for (Stmt::child_iterator FI = Loop->getInit()->child_begin(),
               FE = Loop->getInit()->child_end(); FI != FE; ++FI) {

            if (isa<DeclStmt>(*FI)) {
#ifdef DEBUGCL
              std::cerr << "Found DeclStmt in the Loop Init\n";
#endif
              DeclStmt *DS = cast<DeclStmt>(*FI);
              NamedDecl *ND = cast<NamedDecl>(DS->getSingleDecl());
              std::string key = ND->getName().str();
              ScopedVariables.insert(std::make_pair(key, ND));
            }
            else if (isa<DeclRefExpr>(*FI)) {
              DeclRefExpr *Ref = cast<DeclRefExpr>(*FI);
              NamedDecl *ND = Ref->getDecl();
              std::string key = ND->getName().str();

              if (NewLocalDecls.find(key) == NewLocalDecls.end()) {
                CreateLocalVariable(Ref, false);
              }
            }
          }
        }
        for (Stmt::child_iterator FI = Loop->getCond()->child_begin(),
            FE = Loop->getCond()->child_end(); FI != FE; ++FI) {
          if (isa<DeclRefExpr>(*FI)) {
            DeclRefExpr *Ref = cast<DeclRefExpr>(*FI);
            NamedDecl *ND = Ref->getDecl();
            std::string key = ND->getName().str();
            if (NewLocalDecls.find(key) == NewLocalDecls.end()) {
              CreateLocalVariable(Ref, false);
            }
          }
        }
        for (Stmt::child_iterator FI = Loop->getInc()->child_begin(),
            FE = Loop->getInc()->child_end(); FI != FE; ++FI) {
          if (isa<DeclRefExpr>(*FI)) {
            DeclRefExpr *Ref = cast<DeclRefExpr>(*FI);
            NamedDecl *ND = Ref->getDecl();
            std::string key = ND->getName().str();
            if (NewLocalDecls.find(key) == NewLocalDecls.end()) {
              CreateLocalVariable(Ref, false);
            }
          }
        }

        LoopsToDistribute.pop_back();
        Stmt *Init = Loop->getInit();
        //const DeclStmt *CondVar = Loop->getConditionVariableDeclStmt();
        Expr *Cond = Loop->getCond();
        Expr *Inc = Loop->getInc();

        // FIXME I should look the code above and see how it affects the
        // creation of a new loop. I just had to remove a ';' from the
        // end of Init
        std::stringstream LoopHeader;
        LoopHeader << "for (";
        if (Init)
          LoopHeader << TheRewriter.ConvertToString(Init);
        if (!isDeclStmt)
          LoopHeader << "; ";
        // FIXME Need to handle CondVar
        //if (CondVar)
        //std::cout << TheRewriter.ConvertToString(static_cast<CondVar);
        if (Cond)
          LoopHeader << TheRewriter.ConvertToString(Cond) << "; ";
        if (Inc)
          LoopHeader << TheRewriter.ConvertToString(Inc) << ") {\n";

#ifdef DEBUGCL
        std::cerr << "Loop header: " << LoopHeader.str() << std::endl;
#endif
        TheRewriter.InsertText(BarrierLoc, LoopHeader.str(), true, true);
      }
    }
  }
  return true;
}

// Remove barrier calls and modify calls to kernel builtin functions.
//template <typename T>
bool WorkitemCoarsen::ThreadSerialiser::VisitCallExpr(Expr *s) {
#ifdef DEBUGCL
  std::cerr << "VisitCallExpr\n";
#endif
  CallExpr *Call = cast<CallExpr>(s);
  FunctionDecl* FD = Call->getDirectCallee();
  DeclarationName DeclName = FD->getNameInfo().getName();
  std::string FuncName = DeclName.getAsString();

  if (FuncName.compare("barrier") == 0) {
    BarrierCalls.push_back(Call);
    TheRewriter.InsertText(Call->getLocStart(), "//", true, true);

    if (LoopBarriers.find(Call->getLocStart()) == LoopBarriers.end()) {
      // Close the triple nest so all work items complete
      CloseLoop(Call->getLocEnd().getLocWithOffset(2));
      // Then open another nested loop for the code after the barrier
      OpenLoop(Call->getLocEnd().getLocWithOffset(2));
    }
  }
  return true;
}

// TODO Need to handle continues and breaks
//template <typename T>
bool WorkitemCoarsen::ThreadSerialiser::WalkUpFromUnaryContinueStmt(
  UnaryOperator *S) {
  return true;
}

//template <typename T>
bool WorkitemCoarsen::ThreadSerialiser::VisitDeclStmt(Stmt *s) {
  DeclStmt *DS = cast<DeclStmt>(s);
  if (DS->isSingleDecl()) {
    NamedDecl *ND = cast<NamedDecl>(DS->getSingleDecl());
    std::string key = ND->getName().str();
    DeclStmts.insert(std::make_pair(key, DS));
  }
  return true;
}

//template <typename T>
bool WorkitemCoarsen::ThreadSerialiser::VisitDeclRefExpr(Expr *expr) {
  DeclRefExpr *RefExpr = cast<DeclRefExpr>(expr);
  std::string key = RefExpr->getDecl()->getName().str();

  if (AllRefs.find(key) == AllRefs.end()) {
    DeclRefSet refset;
    refset.push_back(RefExpr);
    AllRefs.insert(std::make_pair(key, refset));
  }
  else {
    AllRefs[key].push_back(RefExpr);
  }

  SourceLocation RefLoc = RefExpr->getLocStart();

  // If we've already added it, don't do it again, but we may need to access
  // an array instead
  if (NewScalarRepls.find(key) != NewScalarRepls.end()) {
    AccessScalar(RefExpr);
    return true;
  }

  // Don't add it if its one of an work-item indexes
  if (key.compare("__kernel_local_id") == 0)
    return true;

  // Also don't add it if it is a function parameter
  for (std::vector<std::string>::iterator PI = ParamVars.begin(),
       PE = ParamVars.end(); PI != PE; ++PI) {
    if ((*PI).compare(key) == 0)
      return true;
  }

  // First, get the location of the declartion of this variable. It will be
  // inside a while loop, but we have to figure out which one.
  NamedDecl *Decl = RefExpr->getDecl();

  // Get the location of the variable declaration, and see if there's any
  // barrier calls between the reference and declaration.
  SourceLocation DeclLoc = Decl->getLocStart();

  for (std::vector<CallExpr*>::iterator CI = BarrierCalls.begin(),
       CE = BarrierCalls.end(); CI != CE; ++CI) {

    SourceLocation BarrierLoc = (*CI)->getLocStart();
    if ((DeclLoc < BarrierLoc) && (BarrierLoc < RefLoc)) {
      CreateLocalVariable(RefExpr, true);
    }
  }

  return true;
}

//template <typename T>
bool WorkitemCoarsen::ThreadSerialiser::VisitFunctionDecl(FunctionDecl *f) {
  // Only function definitions (with bodies), not declarations.
  if (f->hasBody()) {

    // Collect the parameter names so we don't later try to localize them
    for (FunctionDecl::param_iterator PI = f->param_begin(),
         PE = f->param_end(); PI != PE; ++PI) {
      ParamVars.push_back((*PI)->getName().str());
    }

    FuncStart = f->getBody()->getLocStart().getLocWithOffset(1);
    Stmt *FuncBody = f->getBody();
    FuncBodyStart = FuncBody->getLocStart().getLocWithOffset(2);
  }
  return true;
}

template <typename T>
WorkitemCoarsen::OpenCLCompiler<T>::OpenCLCompiler(unsigned x,
                                                unsigned y,
                                                unsigned z) {

  // CompilerInstance will hold the instance of the Clang compiler for us,
  // managing the various objects needed to run the compiler.
  TheCompInst.createDiagnostics(0, 0);

  // Initialize target info with the default triple for our platform.
  IntrusiveRefCntPtr<TargetOptions> TO(new TargetOptions);
  TO.getPtr()->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI = TargetInfo::CreateTargetInfo(
  TheCompInst.getDiagnostics(), *TO);
  TheCompInst.setTarget(TI);

  // Set the compiler up to handle OpenCL
  TheCompInst.getHeaderSearchOpts().AddPath(LIBCLC_INCLUDE_DIR,
                                            clang::frontend::Angled,
                                            false, false, false);
  TheCompInst.getHeaderSearchOpts().AddPath(
    CLANG_INCLUDE_DIR, clang::frontend::Angled,
    false, false, false);
  TheCompInst.getHeaderSearchOpts().ResourceDir = CLANG_RESOURCE_DIR;
  TheCompInst.getHeaderSearchOpts().UseBuiltinIncludes = true;
  TheCompInst.getHeaderSearchOpts().UseStandardSystemIncludes = false;

  TheCompInst.getPreprocessorOpts().Includes.push_back("clc/clc.h");
  TheCompInst.getPreprocessorOpts().addMacroDef(
    "cl_clang_storage_class_specifiers");
  TheCompInst.getInvocation().setLangDefaults(TheCompInst.getLangOpts(),
                                              clang::IK_OpenCL);

  TheCompInst.createFileManager();
  FileMgr = &(TheCompInst.getFileManager());
  TheCompInst.createSourceManager(*FileMgr);
  SourceMgr = &(TheCompInst.getSourceManager());
  TheCompInst.createPreprocessor();
  TheCompInst.createASTContext();

  // A Rewriter helps us manage the code rewriting task.
  TheRewriter.setSourceMgr(*SourceMgr, TheCompInst.getLangOpts());

  // Create an AST consumer instance which is going to get called by
  // ParseAST.
  TheConsumer = new OpenCLASTConsumer(TheRewriter, x, y, z);
}

template <typename T>
WorkitemCoarsen::OpenCLCompiler<T>::~OpenCLCompiler() {
  delete TheConsumer;
}

template <typename T>
void WorkitemCoarsen::OpenCLCompiler<T>::setFile(std::string input) {
  // Set the main file handled by the source manager to the input file.
  const FileEntry *FileIn = FileMgr->getFile(input.c_str());
  SourceMgr->createMainFileID(FileIn);
  TheCompInst.getDiagnosticClient().BeginSourceFile(
  TheCompInst.getLangOpts(),
  &TheCompInst.getPreprocessor());
}

template <typename T>
void WorkitemCoarsen::OpenCLCompiler<T>::Parse() {
  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(TheCompInst.getPreprocessor(), TheConsumer,
           TheCompInst.getASTContext());
}

