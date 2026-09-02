#ifndef BBLIFT_OPTIONS_H
#define BBLIFT_OPTIONS_H

#include <string>

namespace bblift {

struct Options {
  std::string function;
  std::string function_regex;
  bool all_functions = true;

  bool dump_cfg = false;
  bool dump_normalized_cfg = false;
  bool check = false;
  bool dry_run = false;
  bool emit_replacements = false;
  bool fail_on_skip = false;
  bool headers = false;
  bool verify_compile = false;
};

} // namespace bblift

#endif // BBLIFT_OPTIONS_H
