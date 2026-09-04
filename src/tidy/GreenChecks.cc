// green-tidy plugin checks.
#include "GreenChecks.h"

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Regex.h"

#include <vector>

using namespace clang;
using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{

// Shared helpers ------------------------------------------------------------

static bool globMatch(llvm::StringRef Pattern, llvm::StringRef Path)
{
    std::string Regex;
    for (size_t i = 0; i < Pattern.size(); ++i)
    {
        char c = Pattern[i];
        if (c == '*')
        {
            if (i + 1 < Pattern.size() && Pattern[i + 1] == '*')
            {
                Regex += ".*";
                ++i;
            }
            else
            {
                Regex += "[^/]*";
            }
        }
        else if (c == '?')
        {
            Regex += "[^/]";
        }
        else if (c == '.' || c == '[' || c == ']' || c == '(' || c == ')' ||
                 c == '+' || c == '^' || c == '$' || c == '\\' || c == '{' ||
                 c == '}')
        {
            Regex += '\\';
            Regex += c;
        }
        else
        {
            Regex += c;
        }
    }
    return llvm::Regex(Regex).match(Path);
}

// -- Ownership ---------------------------------------------------------------

// Canonicalize a filesystem path with real_path. Returns empty on failure
// (e.g. virtual, builtin, or scratch buffer paths), which the caller treats
// as "not a real source file".
static std::string canonicalizePath(llvm::StringRef Path)
{
    if (Path.empty())
        return "";
    llvm::SmallString<256> Canon;
    std::error_code EC = llvm::sys::fs::real_path(Path, Canon);
    if (EC)
        return "";
    return Canon.str().str();
}

static void addCanonicalList(llvm::StringRef List,
                             llvm::SmallVectorImpl<std::string> &Out)
{
    SmallVector<llvm::StringRef, 8> Parts;
    List.split(Parts, ',');
    for (llvm::StringRef P : Parts)
    {
        P = P.trim();
        if (P.empty())
            continue;
        std::string C = canonicalizePath(P);
        if (!C.empty())
            Out.push_back(C);
    }
}

// Component-aware containment: Path lies beneath Root when Path == Root or
// Path starts with Root + "/". Both inputs must be canonical absolute paths.
static bool beneathPath(llvm::StringRef Path, llvm::StringRef Root)
{
    if (Path == Root)
        return true;
    if (Root == "/")
        return Path.starts_with("/");
    return Path.starts_with((Root + "/").str());
}

static bool beneathAny(llvm::StringRef Path,
                       const llvm::SmallVectorImpl<std::string> &Roots)
{
    for (const std::string &R : Roots)
        if (beneathPath(Path, R))
            return true;
    return false;
}

Ownership::Ownership(llvm::StringRef Roots, llvm::StringRef Exclude)
    : RootsStr(Roots.str()), ExcludeStr(Exclude.str())
{
}

void Ownership::configure(const SourceManager &SM)
{
    if (Configured)
        return;
    Configured = true;
    addCanonicalList(RootsStr, Roots);
    addCanonicalList(ExcludeStr, Exclude);
    FileID Main = SM.getMainFileID();
    if (Main.isValid())
    {
        SourceLocation Start = SM.getLocForStartOfFile(Main);
        llvm::StringRef MainFile = SM.getFilename(Start);
        if (!MainFile.empty())
            MainDir = canonicalizePath(llvm::sys::path::parent_path(MainFile));
    }
}

bool Ownership::isOwned(SourceLocation Loc, const SourceManager &SM) const
{
    if (Loc.isInvalid())
        return false;
    // Resolve macro locations to the file that actually spelled the tokens.
    llvm::StringRef File = SM.getFilename(SM.getSpellingLoc(Loc));
    if (File.empty())
        return false; // virtual/builtin/scratch buffer: not owned
    std::string Canon = canonicalizePath(File);
    if (Canon.empty())
        return false; // could not resolve to a real source file
    if (beneathAny(Canon, Exclude))
        return false;
    if (!Roots.empty())
        return beneathAny(Canon, Roots);
    if (MainDir.empty())
        return false;
    return beneathPath(Canon, MainDir);
}

// -- MacroRegistry -----------------------------------------------------------

const MacroRegistry::Entry *
MacroRegistry::findGoverning(SourceLocation Loc, const SourceManager &SM) const
{
    SourceLocation Cur = Loc;
    while (Cur.isMacroID())
    {
        SourceLocation Spelling = SM.getSpellingLoc(Cur);
        if (Spelling.isFileID())
        {
            for (const Entry &E : Entries)
            {
                if (E.DefBegin.isValid() &&
                    SM.isPointWithin(Spelling, E.DefBegin, E.DefEnd))
                    return &E;
            }
        }
        SourceLocation Caller = SM.getImmediateMacroCallerLoc(Cur);
        if (Caller == Cur)
            break;
        Cur = Caller;
    }
    return nullptr;
}

bool isCompatibilityPath(SourceLocation Loc, const SourceManager &SM,
                         llvm::StringRef Compatibility)
{
    if (Compatibility.empty() || Loc.isInvalid() || Loc.isMacroID())
        return false;
    llvm::SmallString<256> Path(SM.getFilename(Loc));
    if (Path.empty())
        return false;
    SmallVector<llvm::StringRef, 8> CompVec;
    Compatibility.split(CompVec, ',');
    for (llvm::StringRef C : CompVec)
    {
        C = C.trim();
        if (C.empty())
            continue;
        if (globMatch(C, Path))
            return true;
    }
    return false;
}

void PureRegistry::addConfigured(llvm::StringRef List)
{
    SmallVector<llvm::StringRef, 8> NamesVec;
    List.split(NamesVec, ',');
    for (llvm::StringRef N : NamesVec)
    {
        N = N.trim();
        if (!N.empty())
            Names.insert(N);
    }
}

// -- Marker PPCallbacks ------------------------------------------------------

namespace
{
class GreenPurePPCallbacks final : public PPCallbacks
{
  public:
    GreenPurePPCallbacks(PureRegistry *Pure, const SourceManager *SM)
        : Pure(Pure), SM(*SM)
    {
    }
    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                      SourceRange Range, const MacroArgs *Args) override
    {
        if (MacroNameTok.getIdentifierInfo() &&
            MacroNameTok.getIdentifierInfo()->getName() == "GREEN_PURE")
        {
            Pure->recordMarkerLine(
                SM.getSpellingLineNumber(MacroNameTok.getLocation()));
        }
    }

  private:
    PureRegistry *Pure;
    const SourceManager &SM;
};
} // namespace

// -- effective-parent walking ------------------------------------------------

static const Stmt *stripWrappers(const Stmt *N)
{
    const Stmt *Cur = N;
    for (;;)
    {
        if (isa<ParenExpr>(Cur) || isa<ImplicitCastExpr>(Cur) ||
            isa<ExprWithCleanups>(Cur) || isa<ConstantExpr>(Cur))
        {
            if (const auto *P = dyn_cast<ParenExpr>(Cur))
            {
                Cur = P->getSubExpr();
                continue;
            }
            if (const auto *C = dyn_cast<ImplicitCastExpr>(Cur))
            {
                Cur = C->getSubExpr();
                continue;
            }
            if (const auto *C = dyn_cast<ExprWithCleanups>(Cur))
            {
                Cur = C->getSubExpr();
                continue;
            }
            if (const auto *C = dyn_cast<ConstantExpr>(Cur))
            {
                Cur = C->getSubExpr();
                continue;
            }
            break;
        }
        break;
    }
    return Cur;
}

static const Stmt *effectiveParent(const Stmt *N, ASTContext &Ctx)
{
    auto Parents = Ctx.getParents(*N);
    if (Parents.empty())
        return nullptr;
    return Parents[0].get<Stmt>();
}

// A placement-transparent wrapper around an effect result: parentheses or an
// implicit/explicit cast merely reinterpret its type. This transparency
// implies no approval of an explicit cast; green-cast-boundary alone judges
// cast admissibility. Value/unary operations that compute from or consume the
// result are NOT transparent.
static bool isTransparentEffectWrapper(const Stmt *N)
{
    return isa<ParenExpr>(N) || isa<ImplicitCastExpr>(N) ||
           isa<ExprWithCleanups>(N) || isa<ConstantExpr>(N) ||
           isa<CStyleCastExpr>(N);
}

// The outermost placement-transparent expression that still names the same
// effect result: the call plus any enclosing parens/implicit-explicit casts.
// Placement analysis then operates on this outer node, so that
// `p = allocate();` (where allocate returns void * and Clang inserts an
// implicit void * -> struct foo * conversion) is a valid result binding.
static const Stmt *outerTransparentEffect(const Stmt *Call, ASTContext &Ctx)
{
    const Stmt *Cur = Call;
    for (;;)
    {
        const Stmt *Next = effectiveParent(Cur, Ctx);
        if (!Next || !isTransparentEffectWrapper(Next))
            return Cur;
        Cur = Next;
    }
}

// True when the node forms a complete transition: it is an entire statement,
// an entire branch/body of a selection/iteration statement, or an entire
// for-clause (init or inc). This AST fork folds expression statements, so the
// node's effective parent is a statement container rather than an ExprStmt.
static bool isCompleteStatement(const Stmt *Node, ASTContext &Ctx)
{
    const Stmt *N = stripWrappers(Node);
    const Stmt *P = effectiveParent(N, Ctx);
    if (!P)
        return false;
    if (isa<CompoundStmt>(P) || isa<LabelStmt>(P))
        return true;
    if (const auto *F = dyn_cast<ForStmt>(P))
        return N == F->getInit() || N == F->getInc();
    if (const auto *If = dyn_cast<IfStmt>(P))
        return N == If->getThen() || N == If->getElse();
    if (const auto *Wh = dyn_cast<WhileStmt>(P))
        return N == Wh->getBody();
    if (const auto *Do = dyn_cast<DoStmt>(P))
        return N == Do->getBody();
    if (const auto *CS = dyn_cast<CaseStmt>(P))
        return N == CS->getSubStmt();
    if (const auto *DS = dyn_cast<DefaultStmt>(P))
        return N == DS->getSubStmt();
    return false;
}

// True when Node *itself* (without unwrapping transparent wrappers) forms an
// entire statement or a complete for-clause. Used on the outermost
// placement-transparent expression so that a wrapped call that is the whole
// statement - e.g. `(f());` - counts as a complete transition, while a call
// buried inside a larger expression still does not.
static bool isWholeStatement(const Stmt *Node, ASTContext &Ctx)
{
    const Stmt *P = effectiveParent(Node, Ctx);
    if (!P)
        return false;
    if (isa<CompoundStmt>(P) || isa<LabelStmt>(P))
        return true;
    if (const auto *F = dyn_cast<ForStmt>(P))
        return Node == F->getInit() || Node == F->getInc();
    if (const auto *If = dyn_cast<IfStmt>(P))
        return Node == If->getThen() || Node == If->getElse();
    if (const auto *Wh = dyn_cast<WhileStmt>(P))
        return Node == Wh->getBody();
    if (const auto *Do = dyn_cast<DoStmt>(P))
        return Node == Do->getBody();
    if (const auto *CS = dyn_cast<CaseStmt>(P))
        return Node == CS->getSubStmt();
    if (const auto *DS = dyn_cast<DefaultStmt>(P))
        return Node == DS->getSubStmt();
    return false;
}

// -- HiddenControlCheck ------------------------------------------------------

namespace
{
// Records project-owned macros (object-like and function-like) so that
// control/transition operators originating from a macro expansion can be
// attributed back to the defining macro.
class GreenMacroRecordCallbacks final : public PPCallbacks
{
  public:
    GreenMacroRecordCallbacks(MacroRegistry *Macros, Ownership *Owned,
                              const SourceManager *SM)
        : Macros(Macros), Owned(Owned), SM(*SM)
    {
    }
    void MacroDefined(const Token &MacroNameTok,
                      const MacroDirective *MD) override
    {
        const MacroInfo *MI = MD ? MD->getMacroInfo() : nullptr;
        if (!MI)
            return;
        SourceLocation DefLoc = MacroNameTok.getLocation();
        if (!Owned->isOwned(DefLoc, SM))
            return;
        llvm::StringRef Name = MacroNameTok.getIdentifierInfo()
                                   ? MacroNameTok.getIdentifierInfo()->getName()
                                   : llvm::StringRef();
        Macros->record(Name, MI->getDefinitionLoc(), MI->getDefinitionEndLoc(),
                       !MI->isFunctionLike());
    }

  private:
    MacroRegistry *Macros;
    Ownership *Owned;
    const SourceManager &SM;
};
} // namespace

void HiddenControlCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(
        binaryOperator(hasAnyOperatorName("&&", "||", ",")).bind("op"), this);
    Finder->addMatcher(conditionalOperator().bind("cond"), this);
}

void HiddenControlCheck::registerPPCallbacks(const SourceManager &SM,
                                             Preprocessor *PP,
                                             Preprocessor *ModuleExpanderPP)
{
    Owned.configure(SM);
    PP->addPPCallbacks(
        std::make_unique<GreenMacroRecordCallbacks>(&Macros, &Owned, &SM));
}

void HiddenControlCheck::check(const MatchFinder::MatchResult &Result)
{
    Owned.configure(*Result.SourceManager);
    const SourceManager &SM = *Result.SourceManager;
    if (const auto *Op = Result.Nodes.getNodeAs<BinaryOperator>("op"))
    {
        SourceLocation Loc = Op->getOperatorLoc();
        if (Macros.findGoverning(Loc, SM))
            return; // control originates from an owned macro;
                    // green-preprocessor reports it
        SourceLocation UseSite = SM.getExpansionRange(Loc).getBegin();
        if (!Owned.isOwned(UseSite, SM))
            return;
        llvm::StringRef Sym = Op->getOpcodeStr();
        if (Op->getOpcode() == BO_Comma)
            Sym = ",";
        diag(Loc, "hidden control operator '" + std::string(Sym) + "'",
             DiagnosticIDs::Error);
    }
    if (const auto *Cond = Result.Nodes.getNodeAs<ConditionalOperator>("cond"))
    {
        SourceLocation Loc = Cond->getQuestionLoc();
        if (Macros.findGoverning(Loc, SM))
            return;
        SourceLocation UseSite = SM.getExpansionRange(Loc).getBegin();
        if (!Owned.isOwned(UseSite, SM))
            return;
        diag(Loc, "hidden control operator '?:'", DiagnosticIDs::Error);
    }
}

// -- TransitionBoundaryCheck -------------------------------------------------

void TransitionBoundaryCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(
        binaryOperator(
            anyOf(isAssignmentOperator(),
                  hasAnyOperatorName("=", "+=", "-=", "*=", "/=", "%=", "<<=",
                                     ">>=", "&=", "^=", "|=")))
            .bind("assign"),
        this);
    Finder->addMatcher(
        unaryOperator(anyOf(hasOperatorName("++"), hasOperatorName("--")))
            .bind("update"),
        this);
}

void TransitionBoundaryCheck::registerPPCallbacks(
    const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP)
{
    Owned.configure(SM);
    PP->addPPCallbacks(
        std::make_unique<GreenMacroRecordCallbacks>(&Macros, &Owned, &SM));
}

void TransitionBoundaryCheck::check(const MatchFinder::MatchResult &Result)
{
    ASTContext &Ctx = *Result.Context;
    const SourceManager &SM = *Result.SourceManager;
    Owned.configure(SM);
    if (const auto *Op = Result.Nodes.getNodeAs<BinaryOperator>("assign"))
    {
        SourceLocation Loc = Op->getOperatorLoc();
        if (Macros.findGoverning(Loc, SM))
            return; // transition originates from an owned macro;
                    // green-preprocessor reports it
        SourceLocation UseSite = SM.getExpansionRange(Loc).getBegin();
        if (!Owned.isOwned(UseSite, SM))
            return;
        if (!isCompleteStatement(Op, Ctx))
            diag(Loc, "assignment must form a complete transition",
                 DiagnosticIDs::Error);
    }
    if (const auto *Op = Result.Nodes.getNodeAs<UnaryOperator>("update"))
    {
        SourceLocation Loc = Op->getOperatorLoc();
        if (Macros.findGoverning(Loc, SM))
            return; // transition originates from an owned macro;
                    // green-preprocessor reports it
        SourceLocation UseSite = SM.getExpansionRange(Loc).getBegin();
        if (!Owned.isOwned(UseSite, SM))
            return;
        if (Op->isPostfix())
            diag(Loc, "postfix update is not accepted; use the prefix form",
                 DiagnosticIDs::Error);
        else if (!isCompleteStatement(Op, Ctx))
            diag(Loc, "update must form a complete transition",
                 DiagnosticIDs::Error);
    }
}

// -- EffectBoundaryCheck -----------------------------------------------------

void EffectBoundaryCheck::registerPPCallbacks(const SourceManager &SM,
                                              Preprocessor *PP,
                                              Preprocessor *ModuleExpanderPP)
{
    Pure.addConfigured(Options.get("PureFunctions", ""));
    PP->addPPCallbacks(std::make_unique<GreenPurePPCallbacks>(&Pure, &SM));
}

void EffectBoundaryCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(callExpr().bind("call"), this);
    Finder->addMatcher(functionDecl().bind("marked"), this);
}

void EffectBoundaryCheck::check(const MatchFinder::MatchResult &Result)
{
    Owned.configure(*Result.SourceManager);
    // Record GREEN_PURE-marked functions so calls to them are treated as pure.
    if (const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("marked"))
    {
        unsigned FuncLine =
            Result.SourceManager->getSpellingLineNumber(FD->getBeginLoc());
        if (Pure.markerLine() != 0 && FuncLine == Pure.markerLine() + 1)
        {
            Pure.clearMarker();
            Pure.addMarked(FD->getName());
        }
        return;
    }
    const auto *Call = Result.Nodes.getNodeAs<CallExpr>("call");
    if (!Call)
        return;
    const FunctionDecl *FD = Call->getDirectCallee();
    if (FD && Pure.isPure(FD->getName()))
        return; // proven pure; belongs to the term language
    // An indirect call (no direct callee) is conservatively effectful: it is
    // never proven pure and must obey the same transition discipline.
    SourceLocation Loc = Call->getBeginLoc();
    if (!Owned.isOwned(Loc, *Result.SourceManager))
        return;
    // Placement-transparent result: the call plus any wrapping parens/casts.
    const Stmt *ResultExpr = outerTransparentEffect(Call, *Result.Context);
    if (isWholeStatement(ResultExpr, *Result.Context))
        return; // complete transition or for init/inc clause
    // effect call with result binding: sole RHS of a complete assignment
    if (const auto *BO = dyn_cast_or_null<BinaryOperator>(
            effectiveParent(ResultExpr, *Result.Context)))
    {
        if (BO->isAssignmentOp() && BO->getRHS() == ResultExpr &&
            isCompleteStatement(BO, *Result.Context))
            return;
    }
    diag(Loc, "effectful call must form a complete transition",
         DiagnosticIDs::Error);
}

// -- PureContractCheck -------------------------------------------------------

void PureContractCheck::registerPPCallbacks(const SourceManager &SM,
                                            Preprocessor *PP,
                                            Preprocessor *ModuleExpanderPP)
{
    Pure.addConfigured(Options.get("PureFunctions", ""));
    PP->addPPCallbacks(std::make_unique<GreenPurePPCallbacks>(&Pure, &SM));
}

void PureContractCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(functionDecl().bind("func"), this);
}

namespace
{
// True when the lvalue names storage visible outside the function: a
// file-scope or static variable, or a memory location reached through a
// pointer (dereference, subscript, or member of a pointer).
static bool isExternallyVisibleLvalue(const Expr *E)
{
    E = E->IgnoreParenImpCasts();
    if (const auto *DRE = dyn_cast<DeclRefExpr>(E))
    {
        const VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl());
        if (!VD)
            return true;
        if (VD->hasLocalStorage())
            return false; // automatic local variable
        return true;      // file-scope or static
    }
    if (const auto *ME = dyn_cast<MemberExpr>(E))
    {
        return isExternallyVisibleLvalue(ME->getBase());
    }
    if (const auto *UE = dyn_cast<UnaryOperator>(E))
    {
        if (UE->getOpcode() == UO_Deref)
            return true;
        return isExternallyVisibleLvalue(UE->getSubExpr());
    }
    if (const auto *Arr = dyn_cast<ArraySubscriptExpr>(E))
        return isExternallyVisibleLvalue(Arr->getBase());
    return true;
}

class EffectFinder : public RecursiveASTVisitor<EffectFinder>
{
  public:
    bool HasMutation = false;
    bool HasEffect = false;
    bool HasVolatile = false;
    PureRegistry *Pure = nullptr;

    bool VisitBinaryOperator(BinaryOperator *BO)
    {
        if (BO->isAssignmentOp() && isExternallyVisibleLvalue(BO->getLHS()))
            HasMutation = true;
        return true;
    }
    bool VisitUnaryOperator(UnaryOperator *UO)
    {
        if (UO->isIncrementDecrementOp() &&
            isExternallyVisibleLvalue(UO->getSubExpr()))
            HasMutation = true;
        if (UO->getOpcode() == UO_Deref && UO->getType().isVolatileQualified())
            HasVolatile = true;
        return true;
    }
    bool VisitMemberExpr(MemberExpr *ME)
    {
        if (ME->getType().isVolatileQualified())
            HasVolatile = true;
        return true;
    }
    bool VisitCallExpr(CallExpr *CE)
    {
        const FunctionDecl *FD = CE->getDirectCallee();
        if (!FD)
        {
            HasEffect = true;
            return true;
        }
        if (!Pure->isPure(FD->getName()))
            HasEffect = true;
        return true;
    }
    bool VisitImplicitCastExpr(ImplicitCastExpr *IC)
    {
        if (IC->getCastKind() == CK_LValueToRValue &&
            IC->getSubExpr()->getType().isVolatileQualified())
            HasVolatile = true;
        return true;
    }
};
} // namespace

void PureContractCheck::check(const MatchFinder::MatchResult &Result)
{
    const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
    if (!FD)
        return;
    unsigned FuncLine =
        Result.SourceManager->getSpellingLineNumber(FD->getBeginLoc());
    unsigned MarkerLine = Pure.markerLine();
    if (MarkerLine == 0 || FuncLine != MarkerLine + 1)
    {
        // This function is not marked; nothing to validate.
        return;
    }
    Pure.clearMarker();
    if (!FD->hasBody())
        return;
    // Validate the marked definition is pure.
    EffectFinder Finder;
    Finder.Pure = &Pure;
    Finder.TraverseStmt(FD->getBody());
    if (Finder.HasMutation)
        diag(FD->getLocation(),
             "false PURE annotation: body contains an assignment or update",
             DiagnosticIDs::Error);
    if (Finder.HasEffect)
        diag(FD->getLocation(),
             "false PURE annotation: body contains an effectful call",
             DiagnosticIDs::Error);
    if (Finder.HasVolatile)
        diag(FD->getLocation(),
             "false PURE annotation: body contains a volatile access",
             DiagnosticIDs::Error);
}

// -- CastBoundaryCheck -------------------------------------------------------

void CastBoundaryCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(cStyleCastExpr().bind("cast"), this);
}

void CastBoundaryCheck::check(const MatchFinder::MatchResult &Result)
{
    const auto *Cast = Result.Nodes.getNodeAs<CStyleCastExpr>("cast");
    if (!Cast)
        return;
    SourceLocation Loc = Cast->getBeginLoc();
    Owned.configure(*Result.SourceManager);
    if (!Owned.isOwned(Loc, *Result.SourceManager))
        return;
    llvm::StringRef Compatibility = Options.get("CompatibilityPaths", "");
    if (isCompatibilityPath(Loc, *Result.SourceManager, Compatibility))
        return;

    QualType Src = Cast->getSubExpr()->getType().getCanonicalType();
    QualType Dst = Cast->getType().getCanonicalType();
    QualType SrcU = Src.getUnqualifiedType();
    QualType DstU = Dst.getUnqualifiedType();

    if (Src == Dst)
    {
        diag(Loc, "cast from a type to its same canonical type is redundant",
             DiagnosticIDs::Error);
        return;
    }
    if (SrcU == DstU)
    {
        if (Src.isMoreQualifiedThan(Dst, *Result.Context))
            diag(Loc, "cast discards const/volatile qualification",
                 DiagnosticIDs::Error);
        return; // adding qualification or nothing else to check
    }

    bool SrcPtr = SrcU->isAnyPointerType();
    bool DstPtr = DstU->isAnyPointerType();
    bool SrcInt = SrcU->isIntegerType();
    bool DstInt = DstU->isIntegerType();

    // integer <-> pointer representation escape. A null pointer constant
    // (integer literal 0 cast to a pointer type) is not an escape; it is the
    // canonical way to form a null constant and is governed by green-null.
    bool SrcIsNullConst =
        DstPtr && isa<IntegerLiteral>(Cast->getSubExpr()->IgnoreParenCasts());
    if ((SrcInt && DstPtr) || (SrcPtr && DstInt))
    {
        if (!SrcIsNullConst)
            diag(Loc, "representation escape: integer/pointer cast",
                 DiagnosticIDs::Error);
        return;
    }
    if (SrcPtr && DstPtr)
    {
        const PointerType *SP = SrcU->getAs<PointerType>();
        const PointerType *DP = DstU->getAs<PointerType>();
        QualType SPPRaw = SP->getPointeeType();
        QualType DPPRaw = DP->getPointeeType();
        QualType SPP = SPPRaw.getUnqualifiedType();
        QualType DPP = DPPRaw.getUnqualifiedType();
        bool SPFn = SPP->isFunctionType();
        bool DPFn = DPP->isFunctionType();
        if (SPFn != DPFn)
        {
            diag(Loc, "representation escape: object/function pointer cast",
                 DiagnosticIDs::Error);
            return;
        }
        if (SPP->isVoidType() || DPP->isVoidType())
        {
            // void * conversions are allowed in both directions, whether
            // written explicitly or implied. No diagnostic.
            return;
        }
        if (SPP == DPP && SPPRaw.isMoreQualifiedThan(DPPRaw, *Result.Context))
        {
            diag(Loc, "cast discards const/volatile qualification",
                 DiagnosticIDs::Error);
            return;
        }
        // Unrelated object-pointer conversions are allowed. Access through
        // the converted pointer remains subject to C's effective-type,
        // aliasing, and alignment rules (the compiler's concern, not this
        // cast check's).
    }
}

// -- NullCheck ---------------------------------------------------------------

void NullCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(
        cStyleCastExpr(hasType(isAnyPointer()),
                       hasSourceExpression(integerLiteral(equals(0))))
            .bind("null"),
        this);
    Finder->addMatcher(
        implicitCastExpr(hasImplicitDestinationType(isAnyPointer()),
                         hasSourceExpression(integerLiteral(equals(0))))
            .bind("null"),
        this);
}

void NullCheck::check(const MatchFinder::MatchResult &Result)
{
    const Expr *E = Result.Nodes.getNodeAs<Expr>("null");
    if (!E)
        return;
    SourceLocation Loc = E->getExprLoc();
    if (E->getExprLoc().isMacroID())
        return; // expansion of NULL is permitted
    Owned.configure(*Result.SourceManager);
    if (!Owned.isOwned(Loc, *Result.SourceManager))
        return;
    diag(Loc, "use NULL for a null pointer constant", DiagnosticIDs::Error);
}

// -- ReservedSuffixCheck -----------------------------------------------------

static bool isReservedSuffix(llvm::StringRef Name)
{
    return Name.size() > 2 && Name.ends_with("_t");
}

void ReservedSuffixCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(typedefDecl().bind("typedef"), this);
    Finder->addMatcher(recordDecl().bind("tag"), this);
    Finder->addMatcher(enumDecl().bind("tag"), this);
}

void ReservedSuffixCheck::check(const MatchFinder::MatchResult &Result)
{
    Owned.configure(*Result.SourceManager);
    llvm::StringRef Compatibility = Options.get("CompatibilityPaths", "");
    llvm::StringRef Name;
    SourceLocation Loc;
    bool Defined = false;

    if (const auto *TD = Result.Nodes.getNodeAs<TypedefDecl>("typedef"))
    {
        Name = TD->getName();
        Loc = TD->getLocation();
        Defined = true;
    }
    else if (const auto *RD = Result.Nodes.getNodeAs<RecordDecl>("tag"))
    {
        if (!RD->isThisDeclarationADefinition() || !RD->getIdentifier())
            return;
        Name = RD->getName();
        Loc = RD->getLocation();
        Defined = true;
    }
    else if (const auto *ED = Result.Nodes.getNodeAs<EnumDecl>("tag"))
    {
        if (!ED->isThisDeclarationADefinition() || !ED->getIdentifier())
            return;
        Name = ED->getName();
        Loc = ED->getLocation();
        Defined = true;
    }

    if (!Defined || Name.empty() || !isReservedSuffix(Name))
        return;
    if (Reported.count(Name))
        return; // one report per reserved name per translation unit
    if (!Owned.isOwned(Loc, *Result.SourceManager))
        return;
    if (isCompatibilityPath(Loc, *Result.SourceManager, Compatibility))
        return;
    Reported.insert(Name);
    diag(Loc, "type name ends with the reserved '_t' suffix",
         DiagnosticIDs::Error);
}

// -- DeclarationCheck --------------------------------------------------------

void DeclarationCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(declStmt().bind("stmt"), this);
    Finder->addMatcher(functionDecl().bind("func"), this);
    Finder->addMatcher(recordDecl().bind("record"), this);
    Finder->addMatcher(translationUnitDecl().bind("tu"), this);
}

static bool isMultiDeclarator(const DeclStmt *S)
{
    unsigned Count = 0;
    for (const Decl *D : S->decls())
    {
        if (isa<VarDecl>(D) && !isa<ParmVarDecl>(D))
            ++Count;
        if (isa<FieldDecl>(D))
            ++Count;
    }
    return Count > 1;
}

void DeclarationCheck::check(const MatchFinder::MatchResult &Result)
{
    Owned.configure(*Result.SourceManager);
    if (const auto *S = Result.Nodes.getNodeAs<DeclStmt>("stmt"))
    {
        if (Owned.isOwned(S->getBeginLoc(), *Result.SourceManager) &&
            isMultiDeclarator(S))
            diag(S->getBeginLoc(),
                 "one declaration must declare exactly one object",
                 DiagnosticIDs::Error);
    }
    if (const auto *TU = Result.Nodes.getNodeAs<TranslationUnitDecl>("tu"))
    {
        // File-scope objects: a multi-declarator shares one type token, so two
        // consecutive file-scope VarDecls with the same type-source begin are a
        // single declaration declaring more than one object.
        SourceLocation PrevTypeBegin;
        for (const Decl *D : TU->decls())
        {
            if (const auto *VD = dyn_cast<VarDecl>(D))
            {
                if (!Owned.isOwned(VD->getBeginLoc(), *Result.SourceManager))
                    continue;
                SourceLocation TypeBegin =
                    VD->getTypeSourceInfo()->getTypeLoc().getBeginLoc();
                if (TypeBegin.isValid() && TypeBegin == PrevTypeBegin)
                    diag(VD->getLocation(),
                         "one declaration must declare exactly one object",
                         DiagnosticIDs::Error);
                PrevTypeBegin = TypeBegin;
            }
        }
        return;
    }
    if (const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("func"))
    {
        if (!Owned.isOwned(FD->getBeginLoc(), *Result.SourceManager))
            return;
        bool KnR = false;
        bool EmptyParens = false;
        if (FD->getTypeSourceInfo())
        {
            auto TSI = FD->getTypeSourceInfo()->getTypeLoc();
            if (auto FTL = TSI.getAsAdjusted<FunctionTypeLoc>())
            {
                // A K&R definition declares its typed parameters after the
                // declarator, so their source locations fall outside the
                // parameter-parentheses range.
                SourceRange Parens = FTL.getParensRange();
                if (Parens.isValid())
                {
                    for (const ParmVarDecl *P : FD->parameters())
                        if (P && !Result.SourceManager->isPointWithin(
                                     P->getLocation(), Parens.getBegin(),
                                     Parens.getEnd()))
                            KnR = true;
                    // Empty parameter parentheses "f()" (not "f(void)") are
                    // unspecified parameters; detected by source spelling so
                    // the rule applies in both C89 and C23.
                    CharSourceRange CSR =
                        CharSourceRange::getTokenRange(Parens);
                    StringRef Text =
                        Lexer::getSourceText(CSR, *Result.SourceManager,
                                             Result.Context->getLangOpts());
                    if (Text.starts_with("("))
                        Text = Text.drop_front();
                    if (Text.ends_with(")"))
                        Text = Text.drop_back();
                    if (Text.trim().empty())
                        EmptyParens = true;
                }
            }
        }
        bool WrittenNoProto =
            FD->getTypeSourceInfo() &&
            FD->getTypeSourceInfo()->getType()->isFunctionNoProtoType();
        if (FD->getType()->isFunctionNoProtoType() || WrittenNoProto || KnR ||
            EmptyParens)
            diag(FD->getLocation(),
                 "prototype-form functions are mandatory; use (void) for no "
                 "parameters",
                 DiagnosticIDs::Error);
    }
    if (const auto *RD = Result.Nodes.getNodeAs<RecordDecl>("record"))
    {
        if (!Owned.isOwned(RD->getBeginLoc(), *Result.SourceManager))
            return;
        SourceLocation PrevTypeBegin;
        for (const FieldDecl *F : RD->fields())
        {
            SourceLocation TypeBegin =
                F->getTypeSourceInfo()->getTypeLoc().getBeginLoc();
            if (TypeBegin.isValid() && TypeBegin == PrevTypeBegin)
                diag(F->getLocation(),
                     "one declaration must declare exactly one object",
                     DiagnosticIDs::Error);
            PrevTypeBegin = TypeBegin;
        }
    }
}

// -- FallthroughCheck --------------------------------------------------------

void FallthroughCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(switchStmt().bind("sw"), this);
}

void FallthroughCheck::check(const MatchFinder::MatchResult &Result)
{
    const auto *SW = Result.Nodes.getNodeAs<SwitchStmt>("sw");
    if (!SW)
        return;
    Owned.configure(*Result.SourceManager);
    if (!Owned.isOwned(SW->getBeginLoc(), *Result.SourceManager))
        return;
    const LangOptions &LO = Result.Context->getLangOpts();
    std::vector<const SwitchCase *> Cases;
    for (const SwitchCase *C = SW->getSwitchCaseList(); C;
         C = C->getNextSwitchCase())
        Cases.push_back(C);
    std::sort(Cases.begin(), Cases.end(),
              [&](const SwitchCase *A, const SwitchCase *B)
              {
                  return Result.SourceManager->isBeforeInTranslationUnit(
                      A->getBeginLoc(), B->getBeginLoc());
              });
    for (size_t i = 0; i + 1 < Cases.size(); ++i)
    {
        const SwitchCase *Cur = Cases[i];
        const SwitchCase *Next = Cases[i + 1];
        SourceRange CaseRange(Cur->getBeginLoc(), Next->getBeginLoc());
        CharSourceRange CSR = CharSourceRange::getTokenRange(CaseRange);
        StringRef Text = Lexer::getSourceText(CSR, *Result.SourceManager, LO);
        bool Terminated = Text.find("break") != StringRef::npos ||
                          Text.find("return") != StringRef::npos;
        bool Marked = Text.contains("/* fall through */");
        if (!Terminated && !Marked)
            diag(Cur->getBeginLoc(),
                 "implicit fallthrough; add 'break' or the exact "
                 "marker '/* fall through */'",
                 DiagnosticIDs::Error);
    }
}

// -- PreprocessorCheck -------------------------------------------------------

namespace
{
class GreenPreprocessorCallbacks final : public PPCallbacks
{
  public:
    GreenPreprocessorCallbacks(PreprocessorCheck *Owner, Ownership *Owned,
                               MacroRegistry *Macros, const SourceManager *SM)
        : Owner(Owner), Owned(Owned), Macros(Macros), SM(*SM)
    {
    }

    void MacroDefined(const Token &MacroNameTok,
                      const MacroDirective *MD) override
    {
        const MacroInfo *MI = MD ? MD->getMacroInfo() : nullptr;
        if (!MI)
            return;
        SourceLocation Loc = MacroNameTok.getLocation();
        if (!Owned->isOwned(Loc, SM))
            return; // only project-owned macros are constrained
        bool ObjectLike = !MI->isFunctionLike();
        if (MI->isFunctionLike())
        {
            Owner->diag(Loc,
                        "function-like macros are forbidden; use a "
                        "function or an object-like constant",
                        DiagnosticIDs::Error);
            return;
        }
        // Token-pasting / stringification macros remain forbidden.
        for (const Token &Tok : MI->tokens())
        {
            if (Tok.is(tok::hashhash) || Tok.is(tok::hash))
            {
                Owner->diag(Loc,
                            "token-manipulation macros (pasting or "
                            "stringification) are forbidden",
                            DiagnosticIDs::Error);
                return;
            }
        }
        // Record owned object-like macros so that a control/transition
        // operator originating from their expansion can be attributed back.
        Macros->record(MacroNameTok.getIdentifierInfo()
                           ? MacroNameTok.getIdentifierInfo()->getName()
                           : llvm::StringRef(),
                       MI->getDefinitionLoc(), MI->getDefinitionEndLoc(),
                       ObjectLike);
    }

  private:
    PreprocessorCheck *Owner;
    Ownership *Owned;
    MacroRegistry *Macros;
    const SourceManager &SM;
};
} // namespace

void PreprocessorCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(binaryOperator(anyOf(hasAnyOperatorName("&&", "||", ","),
                                            isAssignmentOperator()))
                           .bind("bin"),
                       this);
    Finder->addMatcher(
        unaryOperator(anyOf(hasOperatorName("++"), hasOperatorName("--")))
            .bind("un"),
        this);
    Finder->addMatcher(conditionalOperator().bind("cond"), this);
    Finder->addMatcher(returnStmt().bind("ret"), this);
    Finder->addMatcher(gotoStmt().bind("goto"), this);
    Finder->addMatcher(breakStmt().bind("brk"), this);
    Finder->addMatcher(continueStmt().bind("cnt"), this);
    Finder->addMatcher(ifStmt().bind("if"), this);
    Finder->addMatcher(forStmt().bind("for"), this);
    Finder->addMatcher(whileStmt().bind("while"), this);
    Finder->addMatcher(doStmt().bind("do"), this);
    Finder->addMatcher(switchStmt().bind("switch"), this);
}

void PreprocessorCheck::registerPPCallbacks(const SourceManager &SM,
                                            Preprocessor *PP,
                                            Preprocessor *ModuleExpanderPP)
{
    Owned.configure(SM);
    PP->addPPCallbacks(std::make_unique<GreenPreprocessorCallbacks>(
        this, &Owned, &Macros, &SM));
}

// Attribute a forbidden control/transition construct back to a project-owned
// object-like macro when the governing token originates from its expansion.
void PreprocessorCheck::check(const MatchFinder::MatchResult &Result)
{
    Owned.configure(*Result.SourceManager);
    const SourceManager &SM = *Result.SourceManager;
    SourceLocation Loc;
    if (const auto *B = Result.Nodes.getNodeAs<BinaryOperator>("bin"))
        Loc = B->getOperatorLoc();
    else if (const auto *U = Result.Nodes.getNodeAs<UnaryOperator>("un"))
        Loc = U->getOperatorLoc();
    else if (const auto *C =
                 Result.Nodes.getNodeAs<ConditionalOperator>("cond"))
        Loc = C->getQuestionLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("ret"))
        Loc = S->getBeginLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("goto"))
        Loc = S->getBeginLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("brk"))
        Loc = S->getBeginLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("cnt"))
        Loc = S->getBeginLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("if"))
        Loc = S->getBeginLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("for"))
        Loc = S->getBeginLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("while"))
        Loc = S->getBeginLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("do"))
        Loc = S->getBeginLoc();
    else if (const auto *S = Result.Nodes.getNodeAs<Stmt>("switch"))
        Loc = S->getBeginLoc();
    else
        return;
    if (Loc.isInvalid())
        return;
    const MacroRegistry::Entry *M = Macros.findGoverning(Loc, SM);
    if (!M || !M->IsObjectLike)
        return;
    if (!M->DefBegin.isValid())
        return;
    diag(M->DefBegin,
         "object-like macro '" + M->Name +
             "' hides control/transition structure",
         DiagnosticIDs::Error);
}

// -- ToolchainBranchingCheck -------------------------------------------------

namespace
{
class GreenBranchingCallbacks final : public PPCallbacks
{
  public:
    GreenBranchingCallbacks(ToolchainBranchingCheck *Owner, Ownership *Owned,
                            const SourceManager *SM, const LangOptions *LO)
        : Owner(Owner), Owned(Owned), SM(*SM), LO(*LO)
    {
    }

    void check(SourceLocation DirectiveLoc, SourceRange ConditionRange)
    {
        if (!Owned->isOwned(DirectiveLoc, SM))
            return;
        if (ConditionRange.isInvalid())
            return;
        CharSourceRange CSR = CharSourceRange::getCharRange(ConditionRange);
        StringRef Text = Lexer::getSourceText(CSR, SM, LO);
        static const char *Banned[] = {
            "__GNUC__", "__clang__",      "__STDC_VERSION__", "__STDC__",
            "_MSC_VER", "__GNUC_MINOR__", "__clang_major__"};
        for (const char *B : Banned)
        {
            if (Text.find(B) != StringRef::npos)
            {
                Owner->diag(DirectiveLoc,
                            "compiler/version-conditional preprocessing "
                            "selects a dialect rather than inhabiting the "
                            "intersection",
                            DiagnosticIDs::Error);
                return;
            }
        }
    }

    void If(SourceLocation Loc, SourceRange ConditionRange,
            ConditionValueKind CVK) override
    {
        check(Loc, ConditionRange);
    }
    void Elif(SourceLocation Loc, SourceRange ConditionRange,
              ConditionValueKind CVK, SourceLocation Loc2) override
    {
        check(Loc, ConditionRange);
    }
    void Ifdef(SourceLocation Loc, const Token &MacroNameTok,
               const MacroDefinition &MD) override
    {
        if (isBanned(MacroNameTok))
            report(Loc);
    }
    void Ifndef(SourceLocation Loc, const Token &MacroNameTok,
                const MacroDefinition &MD) override
    {
        if (isBanned(MacroNameTok))
            report(Loc);
    }

  private:
    bool isBanned(const Token &Tok) const
    {
        const IdentifierInfo *II = Tok.getIdentifierInfo();
        if (!II)
            return false;
        StringRef N = II->getName();
        return N == "__GNUC__" || N == "__clang__" || N == "__STDC_VERSION__" ||
               N == "__STDC__" || N == "_MSC_VER";
    }
    void report(SourceLocation Loc)
    {
        if (!Owned->isOwned(Loc, SM))
            return;
        Owner->diag(Loc,
                    "compiler/version-conditional preprocessing selects a "
                    "dialect rather than inhabiting the intersection",
                    DiagnosticIDs::Error);
    }

    ToolchainBranchingCheck *Owner;
    Ownership *Owned;
    const SourceManager &SM;
    const LangOptions &LO;
};
} // namespace

void ToolchainBranchingCheck::registerPPCallbacks(
    const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP)
{
    Owned.configure(SM);
    PP->addPPCallbacks(std::make_unique<GreenBranchingCallbacks>(
        this, &Owned, &SM, &getLangOpts()));
}

void ToolchainBranchingCheck::checkCondition(SourceRange ConditionRange,
                                             const SourceManager &SM)
{
    (void)ConditionRange;
    (void)SM;
}

// -- FlatCheck ---------------------------------------------------------------
namespace
{

// Maximum inline glue statements allowed in one straight-line run of a
// controller (K). One delegation per decision site; more must be a worker.
constexpr unsigned kFlatK = 1;

bool isControlStmt(const Stmt *S)
{
    if (!S)
        return false;
    return isa<IfStmt>(S) || isa<WhileStmt>(S) || isa<DoStmt>(S) ||
           isa<ForStmt>(S) || isa<SwitchStmt>(S) || isa<GotoStmt>(S) ||
           isa<IndirectGotoStmt>(S) || isa<LabelStmt>(S) || isa<CaseStmt>(S) ||
           isa<DefaultStmt>(S);
}

bool isTerminator(const Stmt *S)
{
    if (!S)
        return false;
    return isa<ReturnStmt>(S) || isa<BreakStmt>(S) || isa<ContinueStmt>(S);
}
// A call, literal (optionally signed), or plain identifier load counts as a
// "simple" RHS; any operator-built value does not. An explicit cast to a null
// constant (NULL) is also simple.
bool isSimpleExpr(const Expr *E)
{
    E = E->IgnoreParenImpCasts();
    if (const auto *CE = dyn_cast<CStyleCastExpr>(E))
        if (isa<IntegerLiteral>(CE->getSubExpr()->IgnoreParenImpCasts()))
            E = CE->getSubExpr();
    if (const auto *UO = dyn_cast<UnaryOperator>(E))
        if (UO->getOpcode() == UO_Minus || UO->getOpcode() == UO_Plus)
            E = UO->getSubExpr()->IgnoreParenImpCasts();
    if (isa<CallExpr>(E) || isa<IntegerLiteral>(E) || isa<FloatingLiteral>(E) ||
        isa<CharacterLiteral>(E) || isa<StringLiteral>(E) ||
        isa<DeclRefExpr>(E))
        return true;
    return false;
}

// Glue: a discarded call, a prefix ++/--, an assignment/binding of a call or a
// constant. Anything else is inline computation that belongs in a worker.
enum class FlatClass
{
    NotActionable,
    Glue,
    Work
};

FlatClass classifyExpr(const Expr *E)
{
    E = E->IgnoreParenImpCasts();
    if (isa<CallExpr>(E))
        return FlatClass::Glue;
    if (const auto *UO = dyn_cast<UnaryOperator>(E))
    {
        if (UO->getOpcode() == UO_PreInc || UO->getOpcode() == UO_PreDec)
            return FlatClass::Glue;
    }
    if (const auto *BO = dyn_cast<BinaryOperator>(E))
    {
        if (BO->isAssignmentOp())
        {
            if (BO->getOpcode() == BO_Assign && isSimpleExpr(BO->getRHS()))
                return FlatClass::Glue;
            return FlatClass::Work; // computed RHS, or compound assignment
        }
        return FlatClass::Work; // value-producing expression
    }
    return FlatClass::Work;
}

FlatClass classifyStmt(const Stmt *S)
{
    if (isa<Expr>(S)) // expression statement (clang has no ExprStmt node)
        return classifyExpr(cast<const Expr>(S));
    if (const auto *DS = dyn_cast<DeclStmt>(S))
    {
        for (const Decl *D : DS->decls())
        {
            if (const auto *VD = dyn_cast<VarDecl>(D))
            {
                if (const Expr *Init = VD->getInit())
                    return isSimpleExpr(Init) ? FlatClass::Glue
                                              : FlatClass::Work;
            }
        }
        return FlatClass::NotActionable; // plain declaration, typedef, etc.
    }
    return FlatClass::NotActionable;
}
struct FlatReport
{
    bool hasWork = false; // inline computation inside a nested block
    bool hasRun = false;  // a run of > K glue statements inside a nested block
};

// Applies the "thin" rule to a nested block (a control/switch body, a bare
// {} block): no inline computation, and at most kFlatK glue statements per
// straight-line run.
static void scanNestedBlock(const CompoundStmt *CS, FlatReport &R)
{
    unsigned run = 0;
    for (const Stmt *S : CS->body())
    {
        if (isControlStmt(S) || isTerminator(S) || isa<CompoundStmt>(S))
        {
            run = 0; // a decision site, transfer, or block boundary
            continue;
        }
        switch (classifyStmt(S))
        {
        case FlatClass::Work:
            R.hasWork = true;
            run = 0;
            break;
        case FlatClass::Glue:
            ++run;
            if (run > kFlatK)
                R.hasRun = true;
            break;
        default:
            break;
        }
    }
}

// The function body is the single free (top-level) scope. Every other compound
// in the subtree is a nested block and must be thin. RecursiveASTVisitor is
// used so descent is safe; the first compound visited is the root.
class FlatScanner : public RecursiveASTVisitor<FlatScanner>
{
  public:
    explicit FlatScanner(FlatReport &R) : R(R)
    {
    }

    bool VisitCompoundStmt(CompoundStmt *CS)
    {
        if (!rootSeen)
        {
            rootSeen = true; // top-level scope: inline work is allowed
            return true;
        }
        scanNestedBlock(CS, R);
        return true;
    }

  private:
    bool rootSeen = false;
    FlatReport &R;
};

} // namespace

void FlatCheck::registerMatchers(MatchFinder *Finder)
{
    Finder->addMatcher(functionDecl(isDefinition()).bind("fn"), this);
}

void FlatCheck::check(const MatchFinder::MatchResult &Result)
{
    Owned.configure(*Result.SourceManager);
    const SourceManager &SM = *Result.SourceManager;
    const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("fn");
    if (!FD)
        return;
    SourceLocation Loc = FD->getLocation();
    SourceLocation UseSite = SM.getExpansionRange(Loc).getBegin();
    if (!Owned.isOwned(UseSite, SM))
        return;
    const auto *Body = dyn_cast_or_null<CompoundStmt>(FD->getBody());
    if (!Body || Body->body().empty())
        return;
    FlatReport Report;
    FlatScanner Scanner(Report);
    Scanner.TraverseStmt(const_cast<CompoundStmt *>(Body));
    if (Report.hasWork || Report.hasRun)
    {
        std::string Msg;
        if (Report.hasWork)
            Msg = "inline computation inside a control/block body; extract it "
                  "into a worker function";
        else
            Msg = "more than one inline statement inside a control/block body; "
                  "extract it into a worker function";
        diag(Loc, Msg, DiagnosticIDs::Error);
    }
}

} // namespace tidy
} // namespace clang
