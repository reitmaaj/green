#include "frontend/Consumer.h"

#include "RunContext.h"
#include "analysis/CFGBuilder.h"
#include "analysis/CFGNormalizer.h"
#include "analysis/Eligibility.h"
#include "analysis/SourceSafety.h"
#include "analysis/VariableAnalysis.h"
#include "emit/ExpressionRewriter.h"
#include "emit/HelperEmitter.h"
#include "emit/NameGenerator.h"
#include "emit/ReplacementBuilder.h"
#include "frontend/FunctionCollector.h"
#include "verify/StructuralVerifier.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

namespace bblift {

namespace {

void reportSkip(RunContext &Ctx, const clang::SourceManager &SM,
                const clang::FunctionDecl *FD, const Diagnostic &D) {
  SkipReport Report;
  Report.function = FD->getNameAsString();
  Report.location = D.location.isValid() ? D.location : FD->getLocation();
  Report.reason = "skip";
  Report.message = D.message;
  Ctx.skips.push_back(Report);

  clang::SourceLocation Expansion = SM.getExpansionLoc(Report.location);
  llvm::errs() << SM.getFilename(Expansion) << ":"
               << SM.getExpansionLineNumber(Report.location) << ":"
               << SM.getExpansionColumnNumber(Report.location) << ": skipped "
               << Report.function << ": " << D.message << "\n";
}

void rewriteFunction(clang::CompilerInstance &CI, RunContext &Ctx,
                     const clang::FunctionDecl *FD, FunctionModel &Model,
                     clang::ASTContext &AC) {
  clang::SourceManager &SM = CI.getSourceManager();
  const clang::LangOptions &LO = CI.getLangOpts();
  const Options &Opts = *Ctx.options;

  if (const OptDiagnostic D = classifyVariables(FD, AC, Model)) {
    reportSkip(Ctx, SM, FD, *D);
    return;
  }
  if (const OptDiagnostic D = verifyModel(Model)) {
    reportSkip(Ctx, SM, FD, *D);
    return;
  }

  GeneratedNames Names = generateNames(FD, AC);
  RewriteContext RCtx{&SM, &LO, &AC};

  for (const Block &B : Model.blocks) {
    for (const SourceUnit &U : B.units) {
      if (const OptDiagnostic D = checkSourceRange(U.range, SM, LO, Opts)) {
        reportSkip(Ctx, SM, FD, *D);
        return;
      }
    }
  }

  clang::CharSourceRange BodyRange = clang::CharSourceRange::getCharRange(
      FD->getBody()->getBeginLoc(), FD->getBody()->getEndLoc());
  if (const OptDiagnostic D = checkSourceRange(BodyRange, SM, LO, Opts)) {
    reportSkip(Ctx, SM, FD, *D);
    return;
  }
  clang::CharSourceRange InsertRange = clang::CharSourceRange::getCharRange(
      FD->getBeginLoc(), FD->getBeginLoc());
  if (const OptDiagnostic D = checkSourceRange(InsertRange, SM, LO, Opts)) {
    reportSkip(Ctx, SM, FD, *D);
    return;
  }

  std::optional<std::string> Support = emitSupport(Model, Names, RCtx);
  if (!Support) {
    reportSkip(Ctx, SM, FD,
               Diagnostic{SkipReason::InternalInvariant,
                          "failed to emit support", FD->getLocation()});
    return;
  }
  std::optional<std::string> Dispatcher =
      emitDispatcherBody(Model, Names, RCtx);
  if (!Dispatcher) {
    reportSkip(Ctx, SM, FD,
               Diagnostic{SkipReason::InternalInvariant,
                          "failed to emit dispatcher", FD->getLocation()});
    return;
  }

  std::string File = SM.getFilename(FD->getBeginLoc()).str();
  clang::tooling::Replacements &Replaces = Ctx.replacements[File];
  if (const OptDiagnostic D = buildFunctionEdits(FD, Model, Names, *Support,
                                                 *Dispatcher, SM, Replaces)) {
    reportSkip(Ctx, SM, FD, *D);
    return;
  }
}

} // namespace

void LiftingConsumer::HandleTranslationUnit(clang::ASTContext &AC) {
  const Options &Opts = *Context.options;
  clang::SourceManager &SM = CI.getSourceManager();

  for (const clang::FunctionDecl *FD : collectFunctions(AC, Opts)) {
    if (const OptDiagnostic D = checkEligibility(FD)) {
      reportSkip(Context, SM, FD, *D);
      continue;
    }
    std::unique_ptr<clang::CFG> Cfg = buildCFG(FD, AC);
    if (Cfg == nullptr) {
      reportSkip(Context, SM, FD,
                 Diagnostic{SkipReason::UnsupportedCFGElement,
                            "failed to build CFG", FD->getLocation()});
      continue;
    }
    if (Opts.dump_cfg) {
      Cfg->print(llvm::outs(), AC.getLangOpts(), /*ShowColors=*/false);
    }
    NormalizationResult Normalized = normalize(FD, Cfg.get(), AC);
    if (Normalized.diagnostic) {
      reportSkip(Context, SM, FD, *Normalized.diagnostic);
      continue;
    }
    if (Opts.dump_normalized_cfg) {
      dumpNormalizedCFG(Normalized.model, llvm::outs());
    }
    if (!Opts.dump_cfg && !Opts.dump_normalized_cfg) {
      rewriteFunction(CI, Context, FD, Normalized.model, AC);
    }
  }
}

} // namespace bblift
