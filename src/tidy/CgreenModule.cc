// green-tidy clang-tidy module: registers the green semantic checks.
#include "GreenChecks.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"

namespace clang
{
namespace tidy
{

class GreenModule : public ClangTidyModule
{
  public:
    void addCheckFactories(ClangTidyCheckFactories &Factories) override
    {
        Factories.registerCheck<HiddenControlCheck>("green-hidden-control");
        Factories.registerCheck<TransitionBoundaryCheck>(
            "green-transition-boundary");
        Factories.registerCheck<EffectBoundaryCheck>("green-effect-boundary");
        Factories.registerCheck<PureContractCheck>("green-pure-contract");
        Factories.registerCheck<CastBoundaryCheck>("green-cast-boundary");
        Factories.registerCheck<NullCheck>("green-null");
        Factories.registerCheck<DeclarationCheck>("green-declaration");
        Factories.registerCheck<FallthroughCheck>("green-fallthrough");
        Factories.registerCheck<PreprocessorCheck>("green-preprocessor");
        Factories.registerCheck<ToolchainBranchingCheck>(
            "green-toolchain-branching");
    }
};

static ClangTidyModuleRegistry::Add<GreenModule>
    X("green-module", "green C89 \xE2\x88\xA9 C23 source-profile checks");

} // namespace tidy
} // namespace clang
