#ifndef BBLIFT_MODEL_NORMALIZED_CFG_H
#define BBLIFT_MODEL_NORMALIZED_CFG_H

#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>

#include <vector>

namespace bblift {

enum class TerminatorKind {
  Fallthrough,
  Branch,
  Switch,
  Goto,
  Return,
  Unreachable,
};

enum class EdgeKind {
  Fallthrough,
  True,
  False,
  Case,
  Default,
  Goto,
  Break,
  Continue,
  Done,
};

struct Edge {
  unsigned destination;
  EdgeKind kind;
  // Case value expression for Case edges (spec section 34); nullptr for
  // Default and other edge kinds.
  const clang::Expr *case_value = nullptr;
};

struct Terminator {
  TerminatorKind kind = TerminatorKind::Fallthrough;
  const clang::Expr *condition = nullptr;
  const clang::Stmt *source = nullptr;
  std::vector<Edge> edges;
};

struct NormalizedBlock {
  unsigned id = 0;
  std::vector<const clang::Stmt *> body;
  Terminator terminator;
  std::vector<unsigned> predecessors;
  std::vector<unsigned> successors;
};

} // namespace bblift

#endif // BBLIFT_MODEL_NORMALIZED_CFG_H
