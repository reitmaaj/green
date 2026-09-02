#ifndef BBLIFT_ANALYSIS_VARIABLE_ANALYSIS_H
#define BBLIFT_ANALYSIS_VARIABLE_ANALYSIS_H

#include "Diagnostics.h"
#include "model/FunctionModel.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>

#include <vector>

namespace bblift {

// Classifies parameters and locals and fills model.frame_values (spec
// section 7). Version 1 makes every parameter and ordinary automatic scalar
// local frame-resident (spec section 7.1, recommended rule).
OptDiagnostic classifyVariables(const clang::FunctionDecl *FD,
                                clang::ASTContext &Ctx, FunctionModel &Model);

// Maps a ValueDecl to its frame-field name; returns nullptr when the
// declaration is not frame-resident.
const Local *frameFieldOf(const FunctionModel &Model,
                          const clang::ValueDecl *D);

} // namespace bblift

#endif // BBLIFT_ANALYSIS_VARIABLE_ANALYSIS_H
