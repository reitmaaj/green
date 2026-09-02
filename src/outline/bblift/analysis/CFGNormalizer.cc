#include "analysis/CFGNormalizer.h"

#include "analysis/Predicates.h"

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>

#include <algorithm>

namespace bblift {

namespace {

clang::CharSourceRange stmtRange(const clang::Stmt *S, clang::ASTContext &Ctx) {
  return clang::Lexer::getAsCharRange(
      S->getSourceRange(), Ctx.getSourceManager(), Ctx.getLangOpts());
}

clang::SourceLocation blockLoc(const clang::CFGBlock *B) {
  if (const clang::Stmt *T = B->getTerminatorStmt()) {
    return T->getBeginLoc();
  }
  for (const clang::CFGElement &El : *B) {
    if (auto CS = El.getAs<clang::CFGStmt>()) {
      return CS->getStmt()->getBeginLoc();
    }
  }
  return clang::SourceLocation();
}

struct RawBlock {
  std::vector<const clang::Stmt *> stmts;
  std::vector<clang::CharSourceRange> ranges;
  Terminator term;
  const clang::CFGBlock *cfg = nullptr;
  bool absorbed = false;
  bool hasLabel = false;
};

} // namespace

NormalizationResult normalize(const clang::FunctionDecl *FD,
                              const clang::CFG *Cfg, clang::ASTContext &Ctx) {
  NormalizationResult Result;
  FunctionModel &Model = Result.model;
  Model.decl = FD;
  Model.returns_void = FD->getReturnType()->isVoidType();

  ParentMap PM;
  PM.build(FD->getBody(), nullptr);

  llvm::DenseMap<const clang::Stmt *, const clang::CFGBlock *> StmtBlock;
  for (const clang::CFGBlock *B : *Cfg) {
    for (const clang::CFGElement &El : *B) {
      if (auto CS = El.getAs<clang::CFGStmt>()) {
        StmtBlock[CS->getStmt()] = B;
      }
    }
  }

  std::vector<RawBlock> Raw(Cfg->getNumBlockIDs());
  for (const clang::CFGBlock *B : *Cfg) {
    if (B == &Cfg->getExit()) {
      continue;
    }
    RawBlock &RB = Raw[B->getBlockID()];
    RB.cfg = B;
    RB.hasLabel = B->getLabel() != nullptr;
    for (const clang::CFGElement &El : *B) {
      if (auto CS = El.getAs<clang::CFGStmt>()) {
        const clang::Stmt *S = unwrapLabel(CS->getStmt());
        // A return statement is a CFG element, not a block
        // terminator, in clang::CFG. It becomes the block's transfer.
        if (const auto *RS = llvm::dyn_cast<clang::ReturnStmt>(S)) {
          RB.term.kind = TerminatorKind::Return;
          RB.term.source = RS;
          continue;
        }
        RB.stmts.push_back(S);
        RB.ranges.push_back(stmtRange(S, Ctx));
        continue;
      }
      if (El.getKind() != clang::CFGElement::Initializer) {
        Result.diagnostic =
            Diagnostic{SkipReason::UnsupportedCFGElement,
                       "unexpected CFG element kind", blockLoc(B)};
        return Result;
      }
    }
    // clang::CFG emits a side-effecting subexpression (e.g. a call) as its
    // own CFGStmt element in addition to the enclosing full-expression
    // element. Emitting both would evaluate the subexpression twice, so
    // drop any element that is strictly inside another element of the
    // same block (spec section 47 atomic units).
    std::vector<const clang::Stmt *> FilteredStmts;
    std::vector<clang::CharSourceRange> FilteredRanges;
    for (size_t I = 0; I < RB.stmts.size(); ++I) {
      bool Covered = false;
      for (size_t J = 0; J < RB.stmts.size(); ++J) {
        if (I != J && PM.isDescendantOf(RB.stmts[I], RB.stmts[J])) {
          Covered = true;
          break;
        }
      }
      if (!Covered) {
        FilteredStmts.push_back(RB.stmts[I]);
        FilteredRanges.push_back(RB.ranges[I]);
      }
    }
    RB.stmts = std::move(FilteredStmts);
    RB.ranges = std::move(FilteredRanges);

    clang::CFGTerminator T = B->getTerminator();
    if (!T.isValid()) {
      continue;
    }
    const clang::Stmt *TS = T.getStmt();
    RB.term.source = TS;
    if (const auto *IS = llvm::dyn_cast<clang::IfStmt>(TS)) {
      RB.term.kind = TerminatorKind::Branch;
      RB.term.condition = IS->getCond();
    } else if (const auto *WS = llvm::dyn_cast<clang::WhileStmt>(TS)) {
      RB.term.kind = TerminatorKind::Branch;
      RB.term.condition = WS->getCond();
    } else if (const auto *FS = llvm::dyn_cast<clang::ForStmt>(TS)) {
      RB.term.kind = TerminatorKind::Branch;
      RB.term.condition = FS->getCond();
    } else if (const auto *DS = llvm::dyn_cast<clang::DoStmt>(TS)) {
      RB.term.kind = TerminatorKind::Branch;
      RB.term.condition = DS->getCond();
    } else if (const auto *SS = llvm::dyn_cast<clang::SwitchStmt>(TS)) {
      RB.term.kind = TerminatorKind::Switch;
      RB.term.condition = SS->getCond();
    } else if (llvm::isa<clang::GotoStmt>(TS)) {
      RB.term.kind = TerminatorKind::Goto;
    } else if (llvm::isa<clang::BreakStmt>(TS) ||
               llvm::isa<clang::ContinueStmt>(TS)) {
      RB.term.kind = TerminatorKind::Fallthrough;
      RB.term.source = TS;
    } else if (llvm::isa<clang::ReturnStmt>(TS)) {
      RB.term.kind = TerminatorKind::Return;
    } else if (llvm::isa<clang::IndirectGotoStmt>(TS)) {
      Result.diagnostic = Diagnostic{SkipReason::UnsupportedControlFlow,
                                     "computed goto", TS->getBeginLoc()};
      return Result;
    } else {
      Result.diagnostic =
          Diagnostic{SkipReason::UnsupportedControlFlow,
                     "unsupported terminator statement", TS->getBeginLoc()};
      return Result;
    }
  }

  // Edge classification. Branch polarity follows the documented successor
  // order [Then, Else] (CFG.h); the order is pinned by golden CFG tests.
  for (const clang::CFGBlock *B : *Cfg) {
    if (B == &Cfg->getExit()) {
      continue;
    }
    RawBlock &RB = Raw[B->getBlockID()];
    size_t SuccIndex = 0;
    for (const clang::CFGBlock *Succ : B->succs()) {
      if (Succ == nullptr) {
        continue;
      }
      Edge E;
      if (Succ == &Cfg->getExit()) {
        E.kind = EdgeKind::Done;
        E.destination = kDone;
        RB.term.edges.push_back(E);
        continue;
      }
      E.destination = Succ->getBlockID();
      switch (RB.term.kind) {
      case TerminatorKind::Branch:
        E.kind = successorPolarity(B, SuccIndex) == ExitPolarity::True
                     ? EdgeKind::True
                     : EdgeKind::False;
        break;
      case TerminatorKind::Switch: {
        const clang::Stmt *Label = Succ->getLabel();
        const auto *CS = llvm::dyn_cast_or_null<clang::CaseStmt>(Label);
        if (CS != nullptr) {
          E.kind = EdgeKind::Case;
          E.case_value = CS->getLHS();
        } else if (llvm::isa_and_nonnull<clang::DefaultStmt>(Label)) {
          E.kind = EdgeKind::Default;
        } else {
          E.kind = EdgeKind::Case;
          E.case_value = nullptr;
        }
        break;
      }
      case TerminatorKind::Goto:
        E.kind = EdgeKind::Goto;
        break;
      case TerminatorKind::Fallthrough:
        if (llvm::isa_and_nonnull<clang::BreakStmt>(RB.term.source)) {
          E.kind = EdgeKind::Break;
        } else if (llvm::isa_and_nonnull<clang::ContinueStmt>(RB.term.source)) {
          E.kind = EdgeKind::Continue;
        } else {
          E.kind = EdgeKind::Fallthrough;
        }
        break;
      case TerminatorKind::Return:
      case TerminatorKind::Unreachable:
        E.kind = EdgeKind::Fallthrough;
        break;
      }
      RB.term.edges.push_back(E);
      ++SuccIndex;
    }
  }

  // Atomic-expression region clustering (spec section 47): absorb operand
  // blocks that belong to the same full expression as a root block.
  for (RawBlock &RB : Raw) {
    if (RB.cfg == nullptr || RB.absorbed) {
      continue;
    }
    const clang::Expr *RegionExpr = nullptr;
    bool ConditionUpgrade = false;
    switch (RB.term.kind) {
    case TerminatorKind::Branch:
    case TerminatorKind::Switch:
      RegionExpr = RB.term.condition;
      ConditionUpgrade = true;
      break;
    case TerminatorKind::Return: {
      const auto *RS = llvm::dyn_cast<clang::ReturnStmt>(RB.term.source);
      RegionExpr = RS != nullptr ? RS->getRetValue() : nullptr;
      break;
    }
    case TerminatorKind::Fallthrough:
      for (auto It = RB.stmts.rbegin(); It != RB.stmts.rend(); ++It) {
        if (const auto *E = llvm::dyn_cast<clang::Expr>(*It)) {
          if (hasBranchingExpr(E)) {
            RegionExpr = E;
            break;
          }
        }
      }
      break;
    default:
      break;
    }
    if (RegionExpr == nullptr || !hasBranchingExpr(RegionExpr)) {
      continue;
    }
    const clang::Expr *FullExpr = PM.fullExprRoot(RegionExpr);
    llvm::DenseSet<unsigned> InRegion;
    for (const clang::CFGBlock *B : *Cfg) {
      if (B == &Cfg->getExit()) {
        continue;
      }
      bool Match = false;
      for (const clang::CFGElement &El : *B) {
        if (auto CS = El.getAs<clang::CFGStmt>()) {
          if (PM.isDescendantOf(CS->getStmt(), FullExpr)) {
            Match = true;
            break;
          }
        }
      }
      if (Match) {
        InRegion.insert(B->getBlockID());
      }
    }
    if (!InRegion.count(RB.cfg->getBlockID())) {
      Result.diagnostic =
          Diagnostic{SkipReason::UnsupportedCFGElement,
                     "atomic expression root not found", blockLoc(RB.cfg)};
      return Result;
    }
    for (unsigned Id : InRegion) {
      if (Id == RB.cfg->getBlockID()) {
        continue;
      }
      RawBlock &Other = Raw[Id];
      if (Other.absorbed) {
        Result.diagnostic = Diagnostic{SkipReason::InternalInvariant,
                                       "overlapping atomic-expression regions",
                                       blockLoc(RB.cfg)};
        return Result;
      }
      for (const clang::Stmt *S : Other.stmts) {
        if (!PM.isDescendantOf(S, FullExpr)) {
          Result.diagnostic = Diagnostic{
              SkipReason::UnsupportedCFGElement,
              "full expression crosses a statement boundary", S->getBeginLoc()};
          return Result;
        }
      }
      Other.absorbed = true;
    }
    if (ConditionUpgrade) {
      RB.term.condition = FullExpr;
    }
  }

  // Collapse empty fallthrough blocks (spec section 37). An empty block
  // whose only successor is the exit collapses to the DONE state.
  llvm::DenseMap<unsigned, unsigned> Collapse;
  bool Changed = true;
  while (Changed) {
    Changed = false;
    for (unsigned Id = 0; Id < Raw.size(); ++Id) {
      RawBlock &RB = Raw[Id];
      if (RB.cfg == nullptr || RB.absorbed || Collapse.count(Id)) {
        continue;
      }
      bool Collapsible = false;
      if (RB.stmts.empty() && !RB.hasLabel &&
          RB.term.kind == TerminatorKind::Fallthrough &&
          RB.term.edges.size() == 1) {
        if (RB.term.edges[0].kind == EdgeKind::Fallthrough &&
            RB.term.edges[0].destination != kDone) {
          Collapsible = true;
          Collapse[Id] = RB.term.edges[0].destination;
        } else if (RB.term.edges[0].kind == EdgeKind::Done) {
          Collapsible = true;
          Collapse[Id] = kDone;
        }
      }
      if (Collapsible) {
        Changed = true;
      }
    }
  }

  auto Resolve = [&Collapse](unsigned Id) -> unsigned {
    // Guard against kDone: it equals DenseMap's empty-key sentinel, so
    // Collapse.count(kDone) would spuriously succeed.
    while (Id != kDone && Collapse.count(Id)) {
      Id = Collapse.at(Id);
    }
    return Id;
  };

  // Representative raw block for each emitted block: collapsed blocks map
  // onto their successor, so only one raw block per emitted block remains.
  // Prefer a representative that carries content over an empty collapsed
  // shell (an empty block with a successor that was collapsed away).
  auto IsTrivial = [](const RawBlock &RB) {
    return RB.stmts.empty() && RB.term.kind == TerminatorKind::Fallthrough;
  };
  llvm::DenseMap<unsigned, unsigned> RepOf;
  for (unsigned Id = 0; Id < Raw.size(); ++Id) {
    if (Raw[Id].cfg != nullptr && !Raw[Id].absorbed) {
      unsigned R = Resolve(Id);
      if (R != kDone) {
        auto It = RepOf.find(R);
        if (It == RepOf.end() ||
            (IsTrivial(Raw[It->second]) && !IsTrivial(Raw[Id]))) {
          RepOf[R] = Id;
        }
      }
    }
  }

  // Emitted blocks: representatives of the collapse-closure.
  llvm::DenseSet<unsigned> Emitted;
  for (auto &KV : RepOf) {
    Emitted.insert(KV.first);
  }

  // Entry: the resolved CFG entry block, or DONE for an empty function.
  const clang::CFGBlock &EntryBlock = Cfg->getEntry();
  unsigned EntryRaw = EntryBlock.getBlockID() == Cfg->getExit().getBlockID()
                          ? kDone
                          : Resolve(EntryBlock.getBlockID());

  // Assign normalized IDs: reverse postorder from entry, then retained
  // unreachable blocks in deterministic CFG order (spec section 38).
  llvm::DenseMap<unsigned, unsigned> IdMap;
  llvm::DenseSet<unsigned> Visited;
  auto OrderSuccs = [&](unsigned Id) -> std::vector<unsigned> {
    std::vector<unsigned> Succs;
    for (const Edge &E : Raw[Id].term.edges) {
      if (E.destination != kDone) {
        unsigned S = Resolve(E.destination);
        if (Emitted.count(S) && !Visited.count(S)) {
          Succs.push_back(S);
        }
      }
    }
    std::sort(Succs.begin(), Succs.end());
    return Succs;
  };

  std::vector<unsigned> PostOrder;
  std::vector<std::pair<unsigned, size_t>> Stack;
  if (EntryRaw != kDone && Emitted.count(EntryRaw)) {
    Stack.push_back({EntryRaw, 0});
    Visited.insert(EntryRaw);
  }
  while (!Stack.empty()) {
    unsigned Node = Stack.back().first;
    size_t &Idx = Stack.back().second;
    std::vector<unsigned> Succs = OrderSuccs(Node);
    if (Idx < Succs.size()) {
      unsigned Next = Succs[Idx++];
      if (!Visited.count(Next)) {
        Visited.insert(Next);
        Stack.push_back({Next, 0});
      }
    } else {
      PostOrder.push_back(Node);
      Stack.pop_back();
    }
  }

  unsigned NextId = 0;
  for (auto It = PostOrder.rbegin(); It != PostOrder.rend(); ++It) {
    IdMap[*It] = NextId++;
  }
  std::vector<unsigned> Unvisited;
  for (unsigned Id : Emitted) {
    if (!IdMap.count(Id)) {
      Unvisited.push_back(Id);
    }
  }
  std::sort(Unvisited.begin(), Unvisited.end());
  for (unsigned Id : Unvisited) {
    IdMap[Id] = NextId++;
  }

  // Build the model.
  for (auto &KV : RepOf) {
    const RawBlock &RB = Raw[KV.second];
    unsigned Final = KV.first;
    auto IdIt = IdMap.find(Final);
    if (IdIt == IdMap.end()) {
      continue;
    }
    Block B;
    B.id = IdIt->second;
    for (size_t I = 0; I < RB.stmts.size(); ++I) {
      const clang::Stmt *S = RB.stmts[I];
      if (RB.term.condition != nullptr &&
          PM.isDescendantOf(S, RB.term.condition)) {
        continue;
      }
      if (RB.term.kind == TerminatorKind::Return) {
        const auto *RS = llvm::dyn_cast<clang::ReturnStmt>(RB.term.source);
        if (RS != nullptr && RS->getRetValue() != nullptr &&
            PM.isDescendantOf(S, RS->getRetValue())) {
          continue;
        }
      }
      SourceUnit U;
      U.root = S;
      U.range = RB.ranges[I];
      B.units.push_back(U);
    }
    B.term = RB.term;
    B.term.edges.clear();
    for (const Edge &E : RB.term.edges) {
      Edge NE = E;
      if (NE.destination != kDone) {
        NE.destination = IdMap.at(Resolve(NE.destination));
      }
      B.term.edges.push_back(NE);
    }
    Model.blocks.push_back(B);
  }
  std::sort(Model.blocks.begin(), Model.blocks.end(),
            [](const Block &A, const Block &B) { return A.id < B.id; });

  // A non-void function that falls off the end has undefined behavior in C
  // (spec section 24); reject it rather than emit an indeterminate result.
  if (!Model.returns_void) {
    for (const Block &B : Model.blocks) {
      if (B.term.kind == TerminatorKind::Fallthrough) {
        for (const Edge &E : B.term.edges) {
          if (E.kind == EdgeKind::Done) {
            Result.diagnostic = Diagnostic{
                SkipReason::UnsupportedControlFlow,
                "falls off the end of a non-void function",
                B.term.source != nullptr ? B.term.source->getBeginLoc()
                                         : FD->getLocation()};
            return Result;
          }
        }
      }
    }
  }

  Model.entry =
      EntryRaw != kDone && IdMap.count(EntryRaw) ? IdMap.at(EntryRaw) : kDone;

  return Result;
}

void dumpNormalizedCFG(const FunctionModel &Model, llvm::raw_ostream &OS) {
  OS << "function " << Model.decl->getName() << "\n";
  OS << "returns void: " << (Model.returns_void ? "yes" : "no") << "\n";
  OS << "entry: B" << Model.entry << "\n";
  for (const Block &B : Model.blocks) {
    OS << "\nB" << B.id << ":\n";
    for (const SourceUnit &U : B.units) {
      OS << "    unit: <" << U.root->getStmtClassName() << ">\n";
    }
    OS << "    terminator: " << static_cast<int>(B.term.kind) << "\n";
    for (const Edge &E : B.term.edges) {
      OS << "        -> "
         << (E.destination == kDone ? "DONE"
                                    : "B" + std::to_string(E.destination))
         << " kind " << static_cast<int>(E.kind) << "\n";
    }
  }
}

} // namespace bblift
