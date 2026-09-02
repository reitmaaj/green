#include "verify/StructuralVerifier.h"
#include "analysis/CFGNormalizer.h"

#include <clang/AST/Decl.h>
#include <llvm/ADT/DenseSet.h>

namespace bblift {

OptDiagnostic verifyModel(const FunctionModel &Model) {
  if (Model.entry != kDone) {
    bool EntryFound = false;
    for (const Block &B : Model.blocks) {
      if (B.id == Model.entry) {
        EntryFound = true;
        break;
      }
    }
    if (!EntryFound) {
      return Diagnostic{SkipReason::InternalInvariant,
                        "entry block does not exist",
                        Model.decl->getLocation()};
    }
  }
  llvm::DenseSet<unsigned> Seen;
  for (const Block &B : Model.blocks) {
    if (!Seen.insert(B.id).second) {
      return Diagnostic{SkipReason::InternalInvariant,
                        "duplicate emitted block ID " + std::to_string(B.id),
                        Model.decl->getLocation()};
    }
    for (const Edge &E : B.term.edges) {
      if (E.destination == kDone) {
        continue;
      }
      bool Found = false;
      for (const Block &T : Model.blocks) {
        if (T.id == E.destination) {
          Found = true;
          break;
        }
      }
      if (!Found) {
        return Diagnostic{SkipReason::InternalInvariant,
                          "transfer targets unknown block " +
                              std::to_string(E.destination),
                          Model.decl->getLocation()};
      }
    }
  }
  return std::nullopt;
}

} // namespace bblift
