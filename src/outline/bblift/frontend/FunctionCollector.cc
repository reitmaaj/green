#include "frontend/FunctionCollector.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Regex.h>

#include <string>

namespace bblift {

namespace {

bool isDefinition(const clang::FunctionDecl *FD) {
  return FD->isThisDeclarationADefinition() && FD->getBody() != nullptr;
}

bool matchesSelection(const clang::FunctionDecl *FD, const Options &Opts) {
  if (Opts.all_functions) {
    return true;
  }
  if (!Opts.function.empty() && FD->getName() == Opts.function) {
    return true;
  }
  if (!Opts.function_regex.empty()) {
    llvm::Regex Re(Opts.function_regex);
    if (Re.isValid() && Re.match(FD->getName())) {
      return true;
    }
  }
  return false;
}

// True when the decl is eligible to be rewritten: it lives in the main file,
// or in a project header when --headers is set. Decls from system headers are
// never rewritten, so their namespaces are not traversed either.
bool isOwnedSource(const clang::Decl *D, const clang::SourceManager &SM,
                   const Options &Opts) {
  clang::SourceLocation Loc = SM.getExpansionLoc(D->getBeginLoc());
  if (SM.isInSystemHeader(Loc)) {
    return false;
  }
  if (Opts.headers) {
    return true;
  }
  return SM.isWrittenInMainFile(Loc);
}

void collectInDeclContext(const clang::DeclContext *DC,
                          const clang::SourceManager &SM, const Options &Opts,
                          std::vector<const clang::FunctionDecl *> &Result) {
  for (const clang::Decl *D : DC->decls()) {
    if (const auto *FD = clang::dyn_cast<clang::FunctionDecl>(D)) {
      if (!isDefinition(FD)) {
        continue;
      }
      if (FD->isMain() && Opts.all_functions) {
        continue;
      }
      if (matchesSelection(FD, Opts)) {
        Result.push_back(FD);
      }
      continue;
    }
    if (!isOwnedSource(D, SM, Opts)) {
      continue;
    }
    if (const auto *NS = clang::dyn_cast<clang::NamespaceDecl>(D)) {
      collectInDeclContext(NS, SM, Opts, Result);
      continue;
    }
    if (const auto *LS = clang::dyn_cast<clang::LinkageSpecDecl>(D)) {
      collectInDeclContext(LS, SM, Opts, Result);
    }
  }
}

} // namespace

std::vector<const clang::FunctionDecl *>
collectFunctions(clang::ASTContext &Ctx, const Options &Opts) {
  std::vector<const clang::FunctionDecl *> Result;
  collectInDeclContext(Ctx.getTranslationUnitDecl(), Ctx.getSourceManager(),
                       Opts, Result);
  return Result;
}

} // namespace bblift
