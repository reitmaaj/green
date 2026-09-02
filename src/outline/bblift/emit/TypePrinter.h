#ifndef BBLIFT_EMIT_TYPE_PRINTER_H
#define BBLIFT_EMIT_TYPE_PRINTER_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>

#include <string>

namespace bblift {

// Prints a Clang semantic type with a stable PrintingPolicy (spec section 8).
std::string printType(clang::QualType T, const clang::ASTContext &Ctx);

} // namespace bblift

#endif // BBLIFT_EMIT_TYPE_PRINTER_H
