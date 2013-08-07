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
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
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

bool WorkitemCoarsen::CreateWorkgroup(std::string &Filename, std::string
                                      &kernel) {
#ifdef DEBUGCL
  std::cerr << "CreateWorkgroup\n";
#endif
  OrigFilename = Filename;
  KernelName = kernel;

  // Firstly, we need to expand the macros within the source code because
  // it is not possible to rewrite their locations.
  std::string SourceContainer;
  llvm::raw_string_ostream ExpandedSource(SourceContainer);
  if(!ExpandMacros()) {
    std::cerr << "!ERROR : Failed to expand macros" << std::endl;
    return false;
  }

  // Initialise the kernel with the while loop(s) and check for barriers
  OpenCLCompiler<KernelInitialiser> InitCompiler(LocalX, LocalY, LocalZ,
                                                 KernelName);
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

// Use a RewriteAction to expand all macros within the original source file
bool WorkitemCoarsen::ExpandMacros() {
  std::string log;
  llvm::raw_string_ostream macro_log(log);
  CompilerInstance CI;
  RewriteMacrosAction act;

  CI.getTargetOpts().Triple = "le1";
  CI.getFrontendOpts().Inputs.push_back(FrontendInputFile(OrigFilename,
                                                          IK_OpenCL));
  // Overwrite original
  CI.getFrontendOpts().OutputFile = OrigFilename;

  CI.getHeaderSearchOpts().AddPath(LIBCLC_INCLUDE_DIR,
                                   frontend::Angled,
                                   false, false, false);
  CI.getHeaderSearchOpts().AddPath(CLANG_INCLUDE_DIR, frontend::Angled,
                                   false, false, false);
  CI.getHeaderSearchOpts().ResourceDir = CLANG_RESOURCE_DIR;
  CI.getPreprocessorOpts().Includes.push_back("clc/clc.h");
  CI.getPreprocessorOpts().addMacroDef(
    "cl_clang_storage_class_specifiers");
  CI.getInvocation().setLangDefaults(CI.getLangOpts(), IK_OpenCL);

  CI.createDiagnostics(0, NULL, new clang::TextDiagnosticPrinter(
      macro_log, &CI.getDiagnosticOpts()));

  if (!CI.ExecuteAction(act)) {
    std::cerr << "RewriteMacrosAction failed:" << std::endl
      << log;
    return false;
  }
  return true;
}

bool WorkitemCoarsen::HandleBarriers() {
#ifdef DEBUGCL
  std::cerr << "HandleBarriers\n";
#endif
  OpenCLCompiler<ThreadSerialiser> SerialCompiler(LocalX, LocalY, LocalZ,
                                                  KernelName);
  SerialCompiler.setFile(InitKernelFilename);
  SerialCompiler.Parse();
  if (SerialCompiler.needsScalarFixes())
    SerialCompiler.FixAllScalarAccesses();

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

template <typename T>
WorkitemCoarsen::ASTVisitorBase<T>::ASTVisitorBase(Rewriter &R,
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

template <typename T>
//void OpenCLCompiler<T>::KernelInitialiser::CloseLoop(SourceLocation Loc) {
void WorkitemCoarsen::ASTVisitorBase<T>::CloseLoop(SourceLocation Loc) {
  TheRewriter.InsertText(Loc, CloseWhile.str(), true, true);
}

template <typename T>
void WorkitemCoarsen::ASTVisitorBase<T>::OpenLoop(SourceLocation Loc) {
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

void WorkitemCoarsen::ThreadSerialiser::FixAllScalarAccesses() {
#ifdef DEBUGCL
  std::cerr << "FixAllScalarAccesses" << std::endl;
#endif
  // Iterate through the map of statments which contain barriers and find the
  // declarations that need to replicated. We do this by checking which
  // of the contained DeclRefExprs are separated from their DeclStmt by a
  // barrier.
  for (std::map<Stmt*, std::vector<DeclStmt*> >::iterator MapI
       = ScopedDeclStmts.begin(), MapE = ScopedDeclStmts.end(); MapI != MapE;
       ++MapI) {

    Stmt *ScopingStmt = MapI->first;
    std::vector<DeclStmt*> DeclStmts = MapI->second;

    // Get source range
    SourceLocation ScopeStart = ScopingStmt->getLocStart();
    SourceLocation ScopeEnd = ScopingStmt->getLocEnd();

    // For each DeclStmt, look at all it's Refs and check whether the
    // location is separated by the location of the barrier
    for (std::vector<DeclStmt*>::iterator DI = DeclStmts.begin(),
         DE = DeclStmts.end(); DI != DE; ++DI) {

      if ((*DI) == NULL)
        continue;

      // Only attempt to replicate the DeclStmt and its references once!
      bool hasReplicated = false;
      if (hasReplicated)
        break;

      NamedDecl *ND = cast<NamedDecl>((*DI)->getSingleDecl());
      std::string RefName = ND->getName().str();
      if (AllRefs.find(RefName) == AllRefs.end())
        continue;

      SourceLocation DeclLoc = (*DI)->getLocStart();
      std::vector<DeclRefExpr*> DeclRefs = AllRefs[RefName];

      for (std::vector<DeclRefExpr*>::iterator RI = DeclRefs.begin(),
           RE = DeclRefs.end(); RI != RE; ++RI) {

        if ((*RI) == NULL)
          continue;

        if (hasReplicated)
          break;

        SourceLocation RefLoc = (*RI)->getLocStart();

        // Look through all the barrier locations to find the one(s) contained
        // within this statement
        for (std::vector<SourceLocation>::iterator BI =
             BarrierLocations.begin(), BE = BarrierLocations.end(); BI != BE;
             ++BI) {

          SourceLocation BarrierLoc = *BI;
          if ((ScopeStart < BarrierLoc) && (BarrierLoc < ScopeEnd)) {
#ifdef DEBUGCL
            std::cerr << "Found barrier within this scope" << std::endl;
#endif
            // Check if there is a barrier between the Stmt and Ref, if so
            // ScalarReplicate will visit all the refs, so break from the
            // loop.
            if ((DeclLoc < BarrierLoc) && (BarrierLoc < RefLoc)) {

              // Before replicated, we need to check whether the scope is in our
              // first defined while loop, because if it is, the declarations
              // need to be outside the loop, at the start of the kernel.
              if (isa<WhileStmt>(ScopingStmt)) {
#ifdef DEBUGCL
                std::cerr << "Scope is a while" << std::endl;
#endif
                WhileStmt *theWhile = cast<WhileStmt>(ScopingStmt);
                if (isa<DeclRefExpr>(theWhile->getCond())) {
                  DeclRefExpr *RefExpr = cast<DeclRefExpr>(theWhile->getCond());
                  std::string condVar =
                    RefExpr->getDecl()->getName().str();
#ifdef DEBUGCL
                  std::cerr << ", Conditional = " << condVar << std::endl;
#endif
                  if (condVar.compare("__kernel_local_id") == 0)
                    ScopeStart = FuncStart;
                }
              }
              ScalarReplicate(ScopeStart, *DI, &DeclRefs);
              hasReplicated = true;
              break;
            }
          }
        }
      }
    }
  }
}

// Run this after parsing is complete, to access all the referenced variables
// within the DeclStmt which need to be replicated
void
WorkitemCoarsen::ThreadSerialiser::ScalarReplicate(SourceLocation InsertLoc,
                                                   DeclStmt *theDecl,
                                    std::vector<DeclRefExpr*> *theRefs) {
#ifdef DEBUGCL
  std::cerr << "ScalarReplicate: " << std::endl;
#endif
  NamedDecl *ND = cast<NamedDecl>(theDecl->getSingleDecl());
  std::string varName = ND->getName().str();
  std::string type = cast<ValueDecl>(ND)->getType().getAsString();

#ifdef DEBUGCL
  std::cerr << varName << std::endl;
#endif

  std::stringstream NewDecl;
  NewDecl << type << " " << varName;

  if (LocalX != 0)
    NewDecl << "[" << LocalX << "]";
  if (LocalY > 1)
    NewDecl << "[" << LocalY << "]";
  if (LocalZ > 1)
    NewDecl << "[" << LocalZ << "]";

  NewDecl << ";\n";
  TheRewriter.InsertText(InsertLoc, NewDecl.str(), true, true);

  // If the statement wasn't initialised, remove the whole statement
  if (theDecl->child_begin() == theDecl->child_end())
    TheRewriter.RemoveText(theDecl->getSourceRange());
  else {
    // Otherwise, just remove the type definition and make the initialisation
    // access a scalar.
    TheRewriter.RemoveText(ND->getLocStart(), type.length());
    AccessScalar(theDecl->getSingleDecl());
  }

  // Then visit all the references to turn them into scalar accesses as well.
  for (std::vector<DeclRefExpr*>::iterator RI = theRefs->begin(),
       RE = theRefs->end(); RI != RE; ++RI)
    AccessScalar(*RI);
}

void WorkitemCoarsen::ThreadSerialiser::CreateLocalVariable(DeclRefExpr *Ref,
                                                            bool ScalarRepl) {

  // Shouldn't have to use this now
  return;

  ////////////////////////////////////////////////////////

  NamedDecl *ND = cast<NamedDecl>(Ref->getDecl());
  std::string varName = ND->getName().str();

  // Do not create local variables for variables who have limited scope,
  // such as one declared in loop headers
  if (ScopedVariables.find(varName) != ScopedVariables.end())
    return;

  // Also don't add it if it is a function parameter
  for (std::vector<std::string>::iterator PI = ParamVars.begin(),
       PE = ParamVars.end(); PI != PE; ++PI) {
    if ((*PI).compare(varName) == 0)
      return;
  }

  // Don't add function calls
  if (isa<FunctionDecl>(Ref->getDecl()))
    return;

  // Don't create locals of our local id!
  if (varName.compare("__kernel_local_id") == 0)
    return;

#ifdef DEBUGCL
  std::cerr << "Creating local variable for " << varName << std::endl;
#endif

  std::string type = cast<ValueDecl>(ND)->getType().getAsString();
  std::stringstream NewDecl;
  NewDecl << type << " " << varName;

  // For variables that are live across loop boundaries, we use scalar
  // replication to hold the values of each work-item. This requires us
  // to retrospectively look back at the previous references and turn them
  // into array accesses as well as just declaring the variables as an array.

  // If barriers are inside of loops, variables within the loops will also need
  // scalar replication.
  if (ScalarRepl) {
    if (NewScalarRepls.find(varName) == NewScalarRepls.end()) {
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

      NewDecl << ";\n";
      TheRewriter.InsertText(FuncStart, NewDecl.str(), true, true);

      /*
      // Visit all the references of this variable
      DeclRefSet varRefs = AllRefs[varName];
      for (std::vector<DeclRefExpr*>::iterator RI = varRefs.begin(),
           RE = varRefs.end(); RI != RE; ++RI) {
        AccessScalar(*RI);
      }*/
    }
  }
  else {
    if (NewLocalDecls.find(varName) == NewLocalDecls.end()) {
      NewLocalDecls.insert(std::make_pair(varName, ND));
      NewDecl << ";\n";
      TheRewriter.InsertText(FuncStart, NewDecl.str(), true, true);
    }
  }


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

// FIXME Scalar access only works for x dimension values!
void WorkitemCoarsen::ThreadSerialiser::AccessScalar(Decl *decl) {
  NamedDecl *ND = cast<NamedDecl>(decl);
#ifdef DEBUGCL
  std::cerr << "Creating scalar access for " << ND->getName().str()
    << std::endl;
#endif
  unsigned offset = ND->getName().str().length();
  offset += cast<ValueDecl>(ND)->getType().getAsString().length();
  // increment because of a space between type and name
  ++offset;
  SourceLocation Loc = ND->getLocStart().getLocWithOffset(offset);
  if(TheRewriter.InsertText(Loc, "[__kernel_local_id[0]]", true))
    std::cerr << "ERROR - location not writable!\n";
}

// FIXME Scalar access only works for x dimension values!
void WorkitemCoarsen::ThreadSerialiser::AccessScalar(DeclRefExpr *Ref) {
#ifdef DEBUGCL
  std::cerr << "Creating scalar access for "
      << Ref->getDecl()->getName().str() << std::endl;
#endif
  unsigned offset = Ref->getDecl()->getName().str().length();
  SourceLocation Loc = Ref->getLocEnd().getLocWithOffset(offset);
  if(TheRewriter.InsertText(Loc, "[__kernel_local_id[0]]", true))
    std::cerr << "ERROR - location not writable! Offset = "
      << offset << std::endl;
}

// Used to find variables in loop headers to keep from scalar replication. To
// only be used when the loop has a barrier within.
void WorkitemCoarsen::ThreadSerialiser::FindScopedVariables(Stmt *s) {
  if (isa<DeclStmt>(s)) {
    DeclStmt *Decl = cast<DeclStmt>(s);
    if (Decl->isSingleDecl()) {
      NamedDecl *ND = cast<NamedDecl>(Decl->getSingleDecl());
      std::string key = ND->getName().str();
      if (ScopedVariables.find(key) == ScopedVariables.end())
        ScopedVariables.insert(std::make_pair(key, ND));
    }
  }
  else {
    for (Stmt::child_iterator FI = s->child_begin(),
         FE = s->child_end(); FI != FE; ++FI) {

      if (*FI == NULL)
        continue;

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
}

SourceLocation
WorkitemCoarsen::ThreadSerialiser::GetOffsetInto(SourceLocation Loc) {
  int offset = Lexer::MeasureTokenLength(Loc,
                                   TheRewriter.getSourceMgr(),
                                   TheRewriter.getLangOpts()) + 3;
  return Loc.getLocWithOffset(offset);
}

SourceLocation
WorkitemCoarsen::ThreadSerialiser::GetOffsetOut(SourceLocation Loc) {
  int offset = Lexer::MeasureTokenLength(Loc,
                                         TheRewriter.getSourceMgr(),
                                         TheRewriter.getLangOpts()) + 2;
  return Loc.getLocWithOffset(offset);
}

void WorkitemCoarsen::ThreadSerialiser::FindRefsToReplicate(Stmt *s) {

  // Shouldn't need this now
  return;

  ///////////////////////////////////////////////////

  for (Stmt::child_iterator DI = s->child_begin(),
       DE = s->child_end(); DI != DE; ++DI) {

    if (*DI == NULL)
      continue;

    // Before we're traversing a node, we need to identify DeclStmts because
    // CreateLocalVariable checks our list of discovered Stmts.
    // TODO Remove this necessity by creating local variables, and removing
    // the old declarations at the same time that we 'FixAllScalarAccesses'.
    if(isa<DeclStmt>(*DI)) {
      DeclStmt *declStmt = cast<DeclStmt>(*DI);
      std::string name =
        cast<NamedDecl>(declStmt->getSingleDecl())->getName().str();
      DeclStmts.insert(std::make_pair(name, declStmt));
    }

    if (!isa<DeclRefExpr>(*DI)) {
#ifdef DEBUGCL
//      std::cerr << "Stmt isn't reference" << std::endl;
#endif
      // Don't replicate calls, but check parameters
      //if (isa<CallExpr>(*DI))
        //FindRefsToReplicate(*((*DI)->child_begin()));
      //else
        FindRefsToReplicate(*DI);
    }
    else {
      DeclRefExpr *RefExpr = cast<DeclRefExpr>(*DI);
      CreateLocalVariable(RefExpr, true);
    }
  }
}

// Whether a loop contains a barrier, nested or not, we follow these steps:
// - close the main loop before this loop starts
// - open a main loop at the start of the body of the loop
// - close the main loop at the end of the body of the loop
// - open the main loop when the for loop exits
void WorkitemCoarsen::ThreadSerialiser::HandleBarrierInLoop(ForStmt *Loop) {
#ifdef DEBUGCL
  std::cerr << "HandleBarrierInLoop\n";
#endif
  SourceLocation ForLoc = Loop->getLocStart();

  /*
  // Check whether we've already handled this loop.
  if (LoopsWithoutBarrier.find(ForLoc) != LoopsWithoutBarrier.end())
    return;
  if (LoopsWithBarrier.find(ForLoc) != LoopsWithBarrier.end())
    return;*/

  Stmt *LoopBody = Loop->getBody();

  CloseLoop(Loop->getLocStart());
  OpenLoop(GetOffsetInto(LoopBody->getLocStart()));
  CloseLoop(LoopBody->getLocEnd());
  OpenLoop(GetOffsetOut(Loop->getLocEnd()));

  FindScopedVariables(Loop->getInit());
  FindScopedVariables(Loop->getCond());
  FindScopedVariables(Loop->getInc());
  //FindRefsToReplicate(LoopBody);
  MapNewLocalVars(Loop);
}

// Return true if the Stmt contains a child that is a barrier call. It does not
// traverse the children's children.
static bool isBarrierContained(Stmt *s) {
#ifdef DEBUGCL
  std::cerr << "isBarrierContained" << std::endl;
#endif

  for (Stmt::child_iterator SI = s->child_begin(), SE = s->child_end();
       SI != SE; ++SI) {
    if (isa<CallExpr>(*SI)) {
      FunctionDecl *FD = (cast<CallExpr>(*SI))->getDirectCallee();
      std::string name = FD->getNameInfo().getName().getAsString();
      if (name.compare("barrier") == 0)
        return true;
    }
  }
  return false;
}

// Return true for any statements which could hold DeclStmts for the current
// scope
static bool isEnclosingStmt(Stmt *s) {
  if (isa<IfStmt>(s))
    return true;
  return false;
}

// Pass a Stmt that creates it's own scope, then iterate over it's children to
// find any DeclStmts within it. The Stmt is expected to be a loop.
void WorkitemCoarsen::ThreadSerialiser::MapNewLocalVars(Stmt *s) {
#ifdef DEBUGCL
  std::cerr << "MapNewLocalVars" << std::endl;
#endif
  Stmt *Body = NULL;
  if (isa<WhileStmt>(s))
    Body = (cast<WhileStmt>(s))->getBody();
  else if (isa<ForStmt>(s))
    Body = (cast<ForStmt>(s))->getBody();

  std::vector<DeclStmt*> theDeclStmts;
  for (Stmt::child_iterator SI = Body->child_begin(), SE = Body->child_end();
       SI != SE; ++SI) {

    if (isa<DeclStmt>(*SI)) {
      DeclStmt *declStmt = cast<DeclStmt>(*SI);
      theDeclStmts.push_back(declStmt);
    }
    //else if (isEnclosingStmt(*SI))
      //MapNewLocalVars(*SI);

  }

  if (!theDeclStmts.empty())
    ScopedDeclStmts.insert(std::make_pair(s, theDeclStmts));
}

/*
// Recursive function to whether the loop contains a barrier
bool WorkitemCoarsen::ThreadSerialiser::BarrierInLoop(ForStmt* s) {
  Stmt* ForBody = cast<ForStmt>(s)->getBody();
  SourceLocation ForLoc = s->getLocStart();
  bool FoundBarriers = false;

  for (Stmt::child_iterator FI = ForBody->child_begin(),
       FE = ForBody->child_end(); FI != FE; ++FI) {

    // Recursively visit inner loops
    if (ForStmt* nested = dyn_cast_or_null<ForStmt>(*FI)) {
      if (BarrierInLoop(nested)) {
#ifdef DEBUGCL
        std::cerr << "Added loop to LoopsWithBarriers" << std::endl;
#endif
        HandleBarrierInLoop(s);
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
        HandleBarrierInLoop(s);

        //if (LoopsToDistribute.empty()) {
#ifdef DEBUGCL
        std::cerr << "Adding loop to LoopsWithBarrier" << std::endl;
#endif
          LoopsWithBarrier.insert(std::make_pair(ForLoc, s));
          //LoopsToDistribute.push_back(s);
        //}
        // FIXME When does LoopsToDistribute get cleared? Do we need to do the
        // following checking..?
        //else if (LoopsToDistribute.front() != s){ }
        LoopBarrier = Call;
        LoopBarriers.insert(std::make_pair(Call->getLocStart(), Call));
        FoundBarriers = true;
      }
    }
  }
  if (!FoundBarriers) {
#ifdef DEBUGCL
    std::cerr << "Adding loop to LoopsWithoutBarrier" << std::endl;
#endif
    LoopsWithoutBarrier.insert(std::make_pair(ForLoc, s));
  }

  return FoundBarriers;
}*/

// We visit loops to find barriers within them, and to parallelise the code
// within the loop if found
bool WorkitemCoarsen::ThreadSerialiser::VisitForStmt(Stmt *s) {
#ifdef DEBUGCL
  std::cerr << "VisitForStmt\n";
#endif
  ForStmt* ForLoop = cast<ForStmt>(s);
  SourceLocation ForLoc = ForLoop->getLocStart();

  if(isBarrierContained(ForLoop->getBody()))
    HandleBarrierInLoop(ForLoop);

  return true;

  ////////////////////////////////////////////////////////////////

  // Check whether we've already visited the loop
  if (LoopsWithBarrier.find(ForLoc) != LoopsWithBarrier.end())
    return true;
  else if (LoopsWithoutBarrier.find(ForLoc) != LoopsWithoutBarrier.end())
    return true;

  BarrierInLoop(ForLoop);
  return true;

}

bool WorkitemCoarsen::ThreadSerialiser::VisitWhileStmt(Stmt *s) {
  WhileStmt *While = cast<WhileStmt>(s);
  if (isBarrierContained(While->getBody()))
    MapNewLocalVars(While);
  return true;
}

// Remove barrier calls and modify calls to kernel builtin functions.
//template <typename T>
bool WorkitemCoarsen::ThreadSerialiser::VisitCallExpr(Expr *s) {
#ifdef DEBUGCL
  std::cerr << "VisitCallExpr: ";
#endif
  CallExpr *Call = cast<CallExpr>(s);
  FunctionDecl* FD = Call->getDirectCallee();
  DeclarationName DeclName = FD->getNameInfo().getName();
  std::string FuncName = DeclName.getAsString();

#ifdef DEBUGCL
  std::cerr << FuncName << std::endl;
#endif

  if (FuncName.compare("barrier") == 0) {
#ifdef DEBUGCL
    std::cerr << "Found Barrier\n";
#endif
    BarrierLocations.push_back(Call->getLocStart());

    BarrierCalls.push_back(Call);
    TheRewriter.InsertText(Call->getLocStart(), "//", true, true);
    CloseLoop(Call->getLocEnd().getLocWithOffset(2));
    OpenLoop(Call->getLocEnd().getLocWithOffset(2));
  }
  return true;
}

// TODO Need to handle continues and breaks
bool WorkitemCoarsen::ThreadSerialiser::WalkUpFromUnaryContinueStmt(
  UnaryOperator *S) {
  return true;
}

bool WorkitemCoarsen::ThreadSerialiser::VisitDeclStmt(Stmt *s) {
  DeclStmt *DS = cast<DeclStmt>(s);
  if (DS->isSingleDecl()) {
    NamedDecl *ND = cast<NamedDecl>(DS->getSingleDecl());
    std::string key = ND->getName().str();
    DeclStmts.insert(std::make_pair(key, DS));
  }
  return true;
}

// Create maps of all the references in the tree
bool WorkitemCoarsen::ThreadSerialiser::VisitDeclRefExpr(Expr *expr) {
  DeclRefExpr *RefExpr = cast<DeclRefExpr>(expr);
  std::string key = RefExpr->getDecl()->getName().str();

  // Don't add it if its one of an work-item indexes
  if (key.compare("__kernel_local_id") == 0)
    return true;

  // Don't add it if it is a function parameter
  for (std::vector<std::string>::iterator PI = ParamVars.begin(),
       PE = ParamVars.end(); PI != PE; ++PI) {
    if ((*PI).compare(key) == 0)
      return true;
  }

  // Either create a new set for the Ref, or add it an the existing one
  if (AllRefs.find(key) == AllRefs.end()) {
    DeclRefSet refset;
    refset.push_back(RefExpr);
    AllRefs.insert(std::make_pair(key, refset));
  }
  else {
    AllRefs[key].push_back(RefExpr);
  }

  return true;

  //////////////////////////////////////////////////////

  // If we've already added it, don't do it again
  if (NewScalarRepls.find(key) != NewScalarRepls.end()) {
    //AccessScalar(RefExpr);
    return true;
  }

  /*
  // Also don't add it if it is a function parameter
  for (std::vector<std::string>::iterator PI = ParamVars.begin(),
       PE = ParamVars.end(); PI != PE; ++PI) {
    if ((*PI).compare(key) == 0)
      return true;
  }*/

  // First, get the location of the declartion of this variable. It will be
  // inside a while loop, but we have to figure out which one.
  NamedDecl *Decl = RefExpr->getDecl();
  SourceLocation RefLoc = RefExpr->getLocStart();

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

/*
bool WorkitemCoarsen::ThreadSerialiser::HandleTopLevelDecl(DeclGroupRef DR) {
  if (isa<FunctionDecl>(DR))
    std::cerr << "Found FunctionDecl as TopLevelDecl\n";
  for (DeclGroupRef::iterator b = DR.begin(), e = DR.end();
       b != e; ++b)
    Visitor.TraverseDecl(*b);
  return true;
}*/

template <typename T>
WorkitemCoarsen::OpenCLCompiler<T>::OpenCLCompiler(unsigned x,
                                                   unsigned y,
                                                   unsigned z,
                                                   std::string &name) {

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
  TheConsumer = new OpenCLASTConsumer(TheRewriter, x, y, z, name);
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
  TheCompInst.getDiagnosticClient().BeginSourceFile(TheCompInst.getLangOpts(),
                                              &TheCompInst.getPreprocessor());
}

template <typename T>
void WorkitemCoarsen::OpenCLCompiler<T>::expandMacros(llvm::raw_ostream *source)
{
  clang::RewriteMacrosInInput(TheCompInst.getPreprocessor(), source);
}

template <typename T>
void WorkitemCoarsen::OpenCLCompiler<T>::Parse() {
  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(TheCompInst.getPreprocessor(), TheConsumer,
           TheCompInst.getASTContext());
}

