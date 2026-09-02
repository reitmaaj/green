#ifndef BBLIFT_ANALYSIS_ELIGIBILITY_H
#define BBLIFT_ANALYSIS_ELIGIBILITY_H

#include "Diagnostics.h"

#include <clang/AST/Decl.h>

namespace bblift {

// Rejects unsupported AST constructs and attributes before any expensive
// transformation (spec section 27). A returned diagnostic names the reason;
// nullopt means the function is eligible to proceed.
OptDiagnostic checkEligibility(const clang::FunctionDecl *FD);

} // namespace bblift

#endif // BBLIFT_ANALYSIS_ELIGIBILITY_H
