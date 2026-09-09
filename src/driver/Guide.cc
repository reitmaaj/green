// green driver: guide lines printed with failing cells and format findings so
// automated fixers understand the enforced profile. Pure string builders.
#include "Guide.h"

namespace green
{

namespace
{
std::string isC23(const std::string &CellLabel)
{
    return CellLabel.find("C23") != std::string::npos ? "-std=c23" : "-std=c89";
}
} // namespace

std::string matrixCellGuide(const std::string &CellLabel)
{
    return CellLabel +
           " matrix cell FAILED -- the source must parse and "
           "type-check warning-clean as strict C89 and strict C23 under this "
           "cell (" +
           isC23(CellLabel) +
           ") with the enforced green baseline "
           "(-pedantic-errors -Wall -Wextra -Werror -Wconversion "
           "-Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes "
           "-Wold-style-definition -Wundef -Wshadow -Wformat=2 -Wcast-qual "
           "-fsyntax-only); the same source must pass with GCC and Clang "
           "under both -std=c89 and -std=c23, so fix the source (never the "
           "flags) and re-run green check; the compiler diagnostics below "
           "localize each violation:";
}

std::string formatFailGuide(const std::string &File)
{
    return "format violation: " + File +
           " is not in the canonical green clang-format profile (Allman "
           "braces, four-space indentation, 80-column lines); reformat the "
           "file with that profile (clang-format -i with the installed "
           "clang-format.yaml) and re-run green check; the clang-format "
           "diagnostics below identify every non-canonical line:";
}

} // namespace green
