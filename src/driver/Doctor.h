#ifndef GREEN_DRIVER_DOCTOR_H
#define GREEN_DRIVER_DOCTOR_H

#include <string>

namespace green
{

// Report toolchain environment status. Returns EXIT_UNAVAILABLE when a fatal
// capability (or plugin ABI match) is missing.
int runDoctor(const std::string &Gcc, const std::string &Clang,
              const std::string &Tidy, const std::string &Format,
              const std::string &PluginPath);

} // namespace green

#endif // GREEN_DRIVER_DOCTOR_H
