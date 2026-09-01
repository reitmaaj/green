#ifndef GREEN_DRIVER_CONFIG_H
#define GREEN_DRIVER_CONFIG_H

#include <string>
#include <vector>

namespace green
{

struct Config
{
    int Version = 0;
    std::string GccCompileDB;
    std::string ClangCompileDB;
    std::vector<std::string> ProjectRoots;
    std::vector<std::string> Exclude;
    std::vector<std::string> CompatibilityPaths;
    std::vector<std::string> PureFunctions;
    std::string BaseDir; // directory containing green.yaml (absolute)
};

// Parse green.yaml. Returns false and sets Error on malformed/unknown input.
bool parseConfigFile(const std::string &Path, Config &Out, std::string &Error);

} // namespace green

#endif // GREEN_DRIVER_CONFIG_H
