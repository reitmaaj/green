// green driver: guide lines printed with failing cells and format findings so
// automated fixers understand the enforced profile. Pure string builders.

#ifndef GREEN_DRIVER_GUIDE_H
#define GREEN_DRIVER_GUIDE_H

#include <string>

namespace green
{

// Printed above a failing matrix cell's compiler diagnostics.
std::string matrixCellGuide(const std::string &CellLabel);

// Printed above clang-format diagnostics for a non-canonical file.
std::string formatFailGuide(const std::string &File);

} // namespace green

#endif // GREEN_DRIVER_GUIDE_H
