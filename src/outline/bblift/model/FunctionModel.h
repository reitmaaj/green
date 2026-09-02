#ifndef BBLIFT_MODEL_FUNCTION_MODEL_H
#define BBLIFT_MODEL_FUNCTION_MODEL_H

#include "model/NormalizedCFG.h"

#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>

#include <string>
#include <vector>

namespace bblift {

using BlockId = unsigned;

struct SourceUnit {
  const clang::Stmt *root = nullptr;
  clang::CharSourceRange range;
};

struct Block {
  BlockId id = 0;
  std::vector<SourceUnit> units;
  Terminator term;
};

struct Local {
  const clang::ValueDecl *decl = nullptr;
  std::string field_name;
  clang::QualType type;
};

struct FunctionModel {
  const clang::FunctionDecl *decl = nullptr;
  std::vector<Local> frame_values;
  std::vector<Block> blocks;
  BlockId entry = 0;
  bool returns_void = true;
};

} // namespace bblift

#endif // BBLIFT_MODEL_FUNCTION_MODEL_H
