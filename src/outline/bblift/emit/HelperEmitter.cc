#include "emit/HelperEmitter.h"
#include "analysis/CFGNormalizer.h"
#include "analysis/Predicates.h"
#include "emit/ExpressionRewriter.h"
#include "emit/NameGenerator.h"
#include "emit/TextUtil.h"
#include "emit/TypePrinter.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <llvm/Support/raw_ostream.h>

namespace bblift {

namespace {

std::optional<std::string> emitUnit(const SourceUnit &U,
                                    const FunctionModel &Model,
                                    const RewriteContext &Ctx) {
  const clang::Stmt *Root = U.root;
  if (const auto *DS = llvm::dyn_cast<clang::DeclStmt>(Root)) {
    return lowerDeclStmt(DS, Model, Ctx);
  }
  if (llvm::isa<clang::NullStmt>(Root)) {
    return std::string();
  }
  if (const auto *ES = llvm::dyn_cast<clang::Expr>(Root)) {
    std::optional<std::string> Text = rewriteExprText(ES, Model, Ctx);
    if (!Text) {
      return std::nullopt;
    }
    return *Text + ";";
  }
  std::optional<std::string> Text = rewriteStmtText(Root, Model, Ctx);
  if (!Text) {
    return std::nullopt;
  }
  return *Text + ";";
}

std::optional<std::string> emitTransfer(const Terminator &Term,
                                        const FunctionModel &Model,
                                        const GeneratedNames &Names,
                                        const RewriteContext &Ctx) {
  std::string Out;
  llvm::raw_string_ostream OS(Out);
  switch (Term.kind) {
  case TerminatorKind::Fallthrough: {
    if (Term.edges.size() == 1) {
      OS << "return " << stateName(Names, Term.edges[0].destination) << ";\n";
    } else if (Term.edges.empty()) {
      OS << "return " << Names.done << ";\n";
    } else {
      return std::nullopt;
    }
    break;
  }
  case TerminatorKind::Branch: {
    if (Term.edges.size() != 2) {
      return std::nullopt;
    }
    std::optional<std::string> Cond =
        rewriteExprText(Term.condition, Model, Ctx);
    if (!Cond) {
      return std::nullopt;
    }
    std::string C = *Cond;
    if (needsParensInCondition(Term.condition)) {
      C = "(" + C + ")";
    }
    std::string TrueState = stateName(Names, Term.edges[0].destination);
    std::string FalseState = stateName(Names, Term.edges[1].destination);
    if (Term.edges[0].kind != EdgeKind::True) {
      std::swap(TrueState, FalseState);
    }
    // Green forbids the `?:` operator (hidden control). Emit the branch as an
    // explicit else-less transfer with mandatory braces instead.
    OS << "if (" << C << ")\n";
    OS << "{\n";
    OS << "    return " << TrueState << ";\n";
    OS << "}\n";
    OS << "return " << FalseState << ";\n";
    break;
  }
  case TerminatorKind::Switch: {
    std::optional<std::string> Cond =
        rewriteExprText(Term.condition, Model, Ctx);
    if (!Cond) {
      return std::nullopt;
    }
    OS << "switch (" << *Cond << ") {\n";
    for (const Edge &E : Term.edges) {
      std::string Target = stateName(Names, E.destination);
      if (E.kind == EdgeKind::Case) {
        std::optional<std::string> Value =
            rewriteExprText(E.case_value, Model, Ctx);
        if (!Value) {
          return std::nullopt;
        }
        OS << "case " << *Value << ":\n";
        OS << "  return " << Target << ";\n";
      } else if (E.kind == EdgeKind::Default) {
        OS << "default:\n";
        OS << "  return " << Target << ";\n";
      }
    }
    OS << "}\n";
    break;
  }
  case TerminatorKind::Goto: {
    if (Term.edges.size() != 1) {
      return std::nullopt;
    }
    OS << "return " << stateName(Names, Term.edges[0].destination) << ";\n";
    break;
  }
  case TerminatorKind::Return: {
    if (Model.returns_void) {
      OS << "return " << Names.done << ";\n";
      break;
    }
    std::optional<std::string> Value;
    if (const auto *RS = llvm::dyn_cast<clang::ReturnStmt>(Term.source)) {
      if (RS->getRetValue() != nullptr) {
        Value = rewriteExprText(RS->getRetValue(), Model, Ctx);
        if (!Value) {
          return std::nullopt;
        }
      }
    }
    if (Value) {
      OS << "s->result = " << *Value << ";\n";
    }
    OS << "return " << Names.done << ";\n";
    break;
  }
  case TerminatorKind::Unreachable:
    OS << "return " << Names.done << ";\n";
    break;
  }
  return Out;
}

std::optional<std::string> emitHelper(const Block &B,
                                      const FunctionModel &Model,
                                      const GeneratedNames &Names,
                                      const RewriteContext &Ctx) {
  std::string Out;
  llvm::raw_string_ostream OS(Out);
  OS << helperHeader(Names, B.id);
  for (const SourceUnit &U : B.units) {
    std::optional<std::string> Unit = emitUnit(U, Model, Ctx);
    if (!Unit) {
      return std::nullopt;
    }
    if (!Unit->empty()) {
      OS << indentUnit(*Unit, "  ") << "\n";
    }
  }
  std::optional<std::string> Transfer = emitTransfer(B.term, Model, Names, Ctx);
  if (!Transfer) {
    return std::nullopt;
  }
  OS << indentUnit(*Transfer, "  ");
  OS << "}\n\n";
  return Out;
}

std::optional<std::string> emitEnum(const FunctionModel &Model,
                                    const GeneratedNames &Names) {
  std::string Out;
  llvm::raw_string_ostream OS(Out);
  std::string Joined;
  for (const Block &B : Model.blocks) {
    if (!Joined.empty()) {
      Joined += ", ";
    }
    Joined += Names.state(B.id);
  }
  if (!Joined.empty()) {
    Joined += ", ";
  }
  Joined += Names.done;
  std::string OneLine = Names.pc_type + " { " + Joined + " };";
  if (OneLine.size() <= kColumnLimit) {
    OS << OneLine << "\n";
    return Out;
  }
  OS << Names.pc_type << " {\n";
  for (const Block &B : Model.blocks) {
    OS << "  " << Names.state(B.id) << ",\n";
  }
  OS << "  " << Names.done << "\n";
  OS << "};\n";
  return Out;
}

std::optional<std::string> emitFrame(const FunctionModel &Model,
                                     const GeneratedNames &Names,
                                     const clang::ASTContext &Ctx) {
  std::string Out;
  llvm::raw_string_ostream OS(Out);
  OS << Names.frame_type << " {\n";
  for (const Local &L : Model.frame_values) {
    clang::QualType T = L.type;
    T.removeLocalConst();
    OS << "  " << declaratorName(T, L.field_name, Ctx) << ";\n";
  }
  if (!Model.returns_void) {
    clang::QualType T = Model.decl->getReturnType();
    T.removeLocalConst();
    OS << "  " << declaratorName(T, "result", Ctx) << ";\n";
  }
  OS << "};\n";
  return Out;
}

// Emits the original function's prototype so generated helpers can reference
// the function itself (self-recursion) before its definition appears (spec
// section 30/31).
std::string emitPrototype(const clang::FunctionDecl *FD,
                          const clang::ASTContext &Ctx) {
  std::string Out;
  llvm::raw_string_ostream OS(Out);
  if (FD->getStorageClass() == clang::SC_Static) {
    OS << "static ";
  }
  OS << declaratorName(FD->getReturnType(), FD->getName(), Ctx);
  OS << "(";
  bool First = true;
  for (const clang::ParmVarDecl *P : FD->parameters()) {
    if (!First) {
      OS << ", ";
    }
    First = false;
    OS << printType(P->getType(), Ctx);
  }
  OS << ");\n";
  return Out;
}

} // namespace

std::optional<std::string> emitSupport(const FunctionModel &Model,
                                       const GeneratedNames &Names,
                                       const RewriteContext &Ctx) {
  const clang::ASTContext &AC = *Ctx.Ctx;
  std::optional<std::string> Enum = emitEnum(Model, Names);
  std::optional<std::string> Frame = emitFrame(Model, Names, AC);
  if (!Enum || !Frame) {
    return std::nullopt;
  }
  std::string Out;
  llvm::raw_string_ostream OS(Out);
  OS << emitPrototype(Model.decl, AC) << "\n";
  OS << *Enum << "\n" << *Frame << "\n";
  for (const Block &B : Model.blocks) {
    std::optional<std::string> Helper = emitHelper(B, Model, Names, Ctx);
    if (!Helper) {
      return std::nullopt;
    }
    OS << *Helper;
  }
  return Out;
}

std::optional<std::string> emitDispatcherBody(const FunctionModel &Model,
                                              const GeneratedNames &Names,
                                              const RewriteContext &Ctx) {
  (void)Ctx;
  std::string Out;
  llvm::raw_string_ostream OS(Out);
  OS << "{\n";
  OS << "  " << Names.frame_type << " s;\n";
  OS << "  " << Names.pc_type << " pc;\n\n";
  for (const Local &L : Model.frame_values) {
    if (const auto *PV = llvm::dyn_cast<clang::ParmVarDecl>(L.decl)) {
      OS << "  s." << L.field_name << " = " << PV->getName() << ";\n";
    }
  }
  OS << "  pc = "
     << (Model.entry == kDone ? Names.done : Names.state(Model.entry))
     << ";\n\n";
  OS << "  for (;;) {\n";
  OS << "    switch (pc) {\n";
  for (const Block &B : Model.blocks) {
    OS << "    case " << Names.state(B.id) << ":\n";
    OS << "      pc = " << Names.helper(B.id) << "(&s);\n";
    OS << "      break;\n";
  }
  OS << "    case " << Names.done << ":\n";
  if (Model.returns_void) {
    OS << "      return;\n";
  } else {
    OS << "      return s.result;\n";
  }
  OS << "    }\n";
  OS << "  }\n";
  OS << "}";
  return Out;
}

} // namespace bblift
