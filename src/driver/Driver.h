#ifndef GREEN_DRIVER_DRIVER_H
#define GREEN_DRIVER_DRIVER_H

#include "CompileDB.h"
#include "Config.h"

#include <string>
#include <vector>

namespace green
{

// Exit statuses.
constexpr int EXIT_GREEN = 0;
constexpr int EXIT_VIOLATION = 1;
constexpr int EXIT_CONFIG = 2;
constexpr int EXIT_UNAVAILABLE = 3;
constexpr int EXIT_INTERNAL = 4;

// Loads both compile databases from the config and verifies their .c sets
// match. Returns false and sets Error on configuration failure.
bool loadDatabases(const Config &Cfg, std::vector<CompileEntry> &Gcc,
                   std::vector<CompileEntry> &Clang, std::string &Error);

// Resolve the four tools on PATH.
bool resolveTools(std::string &Gcc, std::string &Clang, std::string &Tidy,
                  std::string &Format);

} // namespace green

#endif // GREEN_DRIVER_DRIVER_H
