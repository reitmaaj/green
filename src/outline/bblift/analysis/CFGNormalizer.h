#ifndef BBLIFT_ANALYSIS_CFGNORMALIZER_H
#define BBLIFT_ANALYSIS_CFGNORMALIZER_H

#include "Diagnostics.h"
#include "model/FunctionModel.h"

#include <clang/AST/Decl.h>
#include <clang/Analysis/CFG.h>
#include <llvm/Support/raw_ostream.h>

#include <vector>

namespace bblift {

// Synthetic program-counter state reached by every return and by falling off
// the end of a void function (spec section 24).
inline constexpr BlockId kDone = ~0u;

struct NormalizationResult {
  FunctionModel model;
  OptDiagnostic diagnostic;
};

// Converts a clang::CFG into the internal FunctionModel. Never fails
// silently: any unrecognized element or unsupported control-flow construct
// produces a diagnostic (spec section 11).
NormalizationResult normalize(const clang::FunctionDecl *FD,
                              const clang::CFG *Cfg, clang::ASTContext &Ctx);

// Deterministic textual dump of the normalized model for diagnostics and
// golden tests (spec section 38).
void dumpNormalizedCFG(const FunctionModel &Model, llvm::raw_ostream &OS);

} // namespace bblift

#endif // BBLIFT_ANALYSIS_CFGNORMALIZER_H
