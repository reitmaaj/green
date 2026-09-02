#ifndef BBLIFT_EMIT_NAME_GENERATOR_H
#define BBLIFT_EMIT_NAME_GENERATOR_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>

#include <string>

namespace bblift {

struct GeneratedNames {
  std::string prefix;     // bblift_<sanitized-function>
  std::string pc_type;    // enum bblift_f_pc
  std::string frame_type; // struct bblift_f_frame
  std::string frame_var;  // s
  std::string done;       // BBLIFT_F_DONE

  std::string state(unsigned Id) const;  // BBLIFT_F_B3
  std::string helper(unsigned Id) const; // bblift_f_b3
};

// Derives collision-resistant generated names for a function (spec
// section 15). The prefix is checked against every identifier in the
// translation unit and adjusted until it does not collide.
GeneratedNames generateNames(const clang::FunctionDecl *FD,
                             clang::ASTContext &Ctx);

} // namespace bblift

#endif // BBLIFT_EMIT_NAME_GENERATOR_H
