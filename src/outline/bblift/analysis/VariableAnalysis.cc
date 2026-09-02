#include "analysis/VariableAnalysis.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <llvm/ADT/DenseMap.h>

#include <cctype>

namespace bblift {

namespace {

std::string sanitizeIdentifier(llvm::StringRef Name) {
  std::string Out;
  Out.reserve(Name.size());
  for (unsigned char C : Name) {
    Out.push_back((std::isalnum(C) || C == '_') ? static_cast<char>(C) : '_');
  }
  if (Out.empty() || std::isdigit(static_cast<unsigned char>(Out[0]))) {
    Out.insert(Out.begin(), '_');
  }
  return Out;
}

void collectLocals(const clang::Stmt *S, unsigned &Index,
                   FunctionModel &Model) {
  if (S == nullptr) {
    return;
  }
  if (const auto *DS = llvm::dyn_cast<clang::DeclStmt>(S)) {
    for (const clang::Decl *D : DS->decls()) {
      if (const auto *VD = llvm::dyn_cast<clang::VarDecl>(D)) {
        Local L;
        L.decl = VD;
        L.type = VD->getType();
        L.field_name = "v_" + std::to_string(Index) + "_" +
                       sanitizeIdentifier(VD->getName());
        Model.frame_values.push_back(L);
        ++Index;
      }
    }
  }
  for (const clang::Stmt *Child : S->children()) {
    collectLocals(Child, Index, Model);
  }
}

} // namespace

OptDiagnostic classifyVariables(const clang::FunctionDecl *FD,
                                clang::ASTContext &Ctx, FunctionModel &Model) {
  (void)Ctx;
  unsigned ParamIndex = 0;
  for (const clang::ParmVarDecl *P : FD->parameters()) {
    Local L;
    L.decl = P;
    L.type = P->getType();
    L.field_name = "p_" + std::to_string(ParamIndex) + "_" +
                   sanitizeIdentifier(P->getName());
    Model.frame_values.push_back(L);
    ++ParamIndex;
  }
  unsigned LocalIndex = 0;
  collectLocals(FD->getBody(), LocalIndex, Model);
  return std::nullopt;
}

const Local *frameFieldOf(const FunctionModel &Model,
                          const clang::ValueDecl *D) {
  for (const Local &L : Model.frame_values) {
    if (L.decl == D) {
      return &L;
    }
  }
  return nullptr;
}

} // namespace bblift
