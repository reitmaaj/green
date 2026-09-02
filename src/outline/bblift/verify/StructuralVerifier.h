#ifndef BBLIFT_VERIFY_STRUCTURAL_VERIFIER_H
#define BBLIFT_VERIFY_STRUCTURAL_VERIFIER_H

#include "Diagnostics.h"
#include "model/FunctionModel.h"

namespace bblift {

// Enforces the mandatory structural invariants before emission (spec
// section 43). A returned diagnostic is an internal-invariant violation and
// must abort the transformation, never produce best-effort output.
OptDiagnostic verifyModel(const FunctionModel &Model);

} // namespace bblift

#endif // BBLIFT_VERIFY_STRUCTURAL_VERIFIER_H
