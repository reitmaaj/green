#ifndef BBLIFT_EMIT_NAME_UTIL_H
#define BBLIFT_EMIT_NAME_UTIL_H

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSet.h>

#include <cctype>
#include <string>

namespace bblift {

// Replaces every non-alphanumeric character (other than '_') with '_', and
// returns "fn" when the result would be empty (spec section 15).
inline std::string sanitize(llvm::StringRef Name) {
  std::string Out;
  Out.reserve(Name.size());
  for (unsigned char C : Name) {
    Out.push_back((std::isalnum(C) || C == '_') ? static_cast<char>(C) : '_');
  }
  if (Out.empty()) {
    Out = "fn";
  }
  return Out;
}

// Uppercases every ASCII character in Name.
inline std::string upper(llvm::StringRef Name) {
  std::string Out = Name.str();
  for (char &C : Out) {
    C = static_cast<char>(std::toupper(static_cast<unsigned char>(C)));
  }
  return Out;
}

// Returns Base, or Base_N (N = 1, 2, ...) when Base or any of its upper, pc,
// or frame variants collides with an identifier in Taken.
inline std::string nonCollidingPrefix(llvm::StringRef Base,
                                      const llvm::StringSet<> &Taken) {
  auto Collides = [&](const std::string &C) {
    return Taken.count(C) || Taken.count(upper(C)) || Taken.count(C + "_pc") ||
           Taken.count(C + "_frame");
  };
  std::string Candidate = Base.str();
  if (!Collides(Candidate)) {
    return Candidate;
  }
  unsigned Suffix = 1;
  while (Collides(Candidate + "_" + std::to_string(Suffix))) {
    ++Suffix;
  }
  return Candidate + "_" + std::to_string(Suffix);
}

} // namespace bblift

#endif // BBLIFT_EMIT_NAME_UTIL_H
