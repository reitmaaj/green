#include "emit/NameGenerator.h"

#include "emit/NameUtil.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <llvm/ADT/StringSet.h>

namespace bblift {

namespace {

void collectIdentifiers(const clang::Stmt *S, llvm::StringSet<> &Out);

void collectIdentifiers(const clang::Decl *D, llvm::StringSet<> &Out) {
  if (D == nullptr) {
    return;
  }
  if (const auto *ND = llvm::dyn_cast<clang::NamedDecl>(D)) {
    if (const clang::IdentifierInfo *II = ND->getIdentifier()) {
      Out.insert(II->getName());
    }
  }
  if (const auto *FD = llvm::dyn_cast<clang::FunctionDecl>(D)) {
    if (FD->getBody()) {
      collectIdentifiers(FD->getBody(), Out);
    }
    for (const clang::Decl *P : FD->parameters()) {
      collectIdentifiers(P, Out);
    }
    for (const clang::Decl *RD : FD->decls()) {
      collectIdentifiers(RD, Out);
    }
  } else if (const auto *RD = llvm::dyn_cast<clang::RecordDecl>(D)) {
    for (const clang::Decl *F : RD->decls()) {
      collectIdentifiers(F, Out);
    }
  } else if (const auto *ED = llvm::dyn_cast<clang::EnumDecl>(D)) {
    for (const clang::Decl *C : ED->enumerators()) {
      collectIdentifiers(C, Out);
    }
  } else if (const auto *NS = llvm::dyn_cast<clang::NamespaceDecl>(D)) {
    for (const clang::Decl *C : NS->decls()) {
      collectIdentifiers(C, Out);
    }
  }
}

void collectIdentifiers(const clang::Stmt *S, llvm::StringSet<> &Out) {
  if (S == nullptr) {
    return;
  }
  for (const clang::Stmt *Child : S->children()) {
    collectIdentifiers(Child, Out);
  }
}

} // namespace

std::string GeneratedNames::state(unsigned Id) const {
  return upper(prefix) + "_B" + std::to_string(Id);
}

std::string GeneratedNames::helper(unsigned Id) const {
  return prefix + "_b" + std::to_string(Id);
}

GeneratedNames generateNames(const clang::FunctionDecl *FD,
                             clang::ASTContext &Ctx) {
  llvm::StringSet<> Taken;
  for (const clang::Decl *D : Ctx.getTranslationUnitDecl()->decls()) {
    collectIdentifiers(D, Taken);
  }

  GeneratedNames Names;
  Names.prefix = nonCollidingPrefix("bblift_" + sanitize(FD->getName()), Taken);
  Names.pc_type = "enum " + Names.prefix + "_pc";
  Names.frame_type = "struct " + Names.prefix + "_frame";
  Names.frame_var = "s";
  Names.done = upper(Names.prefix) + "_DONE";
  return Names;
}

} // namespace bblift
