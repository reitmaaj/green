#ifndef GREEN_DRIVER_FORMAT_H
#define GREEN_DRIVER_FORMAT_H

#include <string>
#include <vector>

namespace green
{

// Run clang-format with the installed green profile. Returns an exit status.
int runFormatCommand(const std::string &Format, const std::string &Profile,
                     const std::vector<std::string> &Files, bool CheckOnly);

} // namespace green

#endif // GREEN_DRIVER_FORMAT_H
