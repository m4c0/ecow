#pragma once
#include <set>
#include <string>

namespace ecow::llvm {
struct input {
  std::string from;
  std::string to;
  std::string clang_exe;
  std::string triple;
  std::vector<std::string> cmd_line;
};
struct deps {
  std::set<std::string> deps;
  bool success;
};

deps find_deps(const input &in, const std::string &depfile,
               bool must_recompile);
bool compile(const input &in);
} // namespace ecow::llvm
