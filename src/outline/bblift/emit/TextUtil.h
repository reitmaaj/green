#ifndef BBLIFT_EMIT_TEXT_UTIL_H
#define BBLIFT_EMIT_TEXT_UTIL_H

#include "analysis/CFGNormalizer.h"
#include "emit/NameGenerator.h"
#include "emit/TypePrinter.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace bblift {

// Column limit used for generated one-line declarations and helper headers.
inline constexpr unsigned kColumnLimit = 80;

// Prefixes every non-blank line of Text with Prefix, preserving line breaks.
inline std::string indentUnit(llvm::StringRef Text, llvm::StringRef Prefix) {
  std::string Out;
  llvm::raw_string_ostream OS(Out);
  bool AtLineStart = true;
  for (char C : Text) {
    if (AtLineStart && C != '\n') {
      OS << Prefix;
      AtLineStart = false;
    }
    OS << C;
    if (C == '\n') {
      AtLineStart = true;
    }
  }
  return OS.str();
}

// Returns the program-counter state name for a destination, mapping kDone to
// the generated DONE state.
inline std::string stateName(const GeneratedNames &Names, unsigned Dest) {
  return Dest == kDone ? Names.done : Names.state(Dest);
}

// Emits a declarator name bound to its type exactly as clang-format
// (PointerAlignment=Right) would: a pointer type keeps the name glued to the
// trailing '*', every other type is space-separated.
inline std::string declaratorName(clang::QualType T, llvm::StringRef Name,
                                  const clang::ASTContext &Ctx) {
  std::string Type = printType(T, Ctx);
  if (!Type.empty() && Type.back() == '*') {
    return Type + Name.str();
  }
  return Type + " " + Name.str();
}

// Emits the helper signature on one line when it fits within the column limit;
// otherwise it wraps before the helper name, and finally inside the parameter
// list, mirroring clang-format's breaking of a single-parameter signature.
inline std::string helperHeader(const GeneratedNames &Names, unsigned Id) {
  std::string Param = Names.frame_type + " *s) {";
  std::string One =
      "static " + Names.pc_type + " " + Names.helper(Id) + "(" + Param;
  if (One.size() <= kColumnLimit) {
    return One + "\n";
  }
  std::string Two = Names.helper(Id) + "(" + Param;
  if (Two.size() <= kColumnLimit) {
    return "static " + Names.pc_type + "\n" + Two + "\n";
  }
  return "static " + Names.pc_type + "\n" + Names.helper(Id) + "(\n" + "    " +
         Param + "\n";
}

// A single non-overlapping replacement within a source slice. Offsets are
// relative to the slice base.
struct Edit {
  unsigned Offset;
  unsigned Length;
  std::string Text;
};

// Applies a list of edits to Source. Edits are sorted by offset and applied
// left-to-right; returns nullopt if any edit overlaps or falls out of bounds.
inline std::optional<std::string> applyEdits(llvm::StringRef Source,
                                             std::vector<Edit> Edits) {
  std::sort(Edits.begin(), Edits.end(),
            [](const Edit &A, const Edit &B) { return A.Offset < B.Offset; });
  std::string Out;
  size_t Pos = 0;
  for (const Edit &E : Edits) {
    if (E.Offset < Pos || E.Offset + E.Length > Source.size()) {
      return std::nullopt;
    }
    Out.append(Source.data() + Pos, E.Offset - Pos);
    Out.append(E.Text);
    Pos = E.Offset + E.Length;
  }
  Out.append(Source.data() + Pos, Source.size() - Pos);
  return Out;
}

} // namespace bblift

#endif // BBLIFT_EMIT_TEXT_UTIL_H
