#pragma once
#include "ecow.core.hpp"

#include <cstdlib>

namespace ecow::impl {
[[nodiscard]] static inline std::string popen(const std::string &cmd) {
  auto f = ::popen(cmd.c_str(), "r");
  char buf[1024];
  std::string res{fgets(buf, 1024, f)};
  if (res.back() == '\n')
    res.pop_back();
  return res;
}

class native_target : public target {
  std::string m_sdk_path;

public:
  native_target() : native_target("macosx") {}
  native_target(const char *sdk) {
    using namespace std::string_literals;
    m_sdk_path = impl::popen("xcrun --show-sdk-path --sdk "s + sdk);
  }
  std::string cxx() override {
    using namespace std::string_literals;
    return "/usr/local/opt/llvm/bin/clang++ -isysroot " + m_sdk_path;
  }
};

} // namespace ecow::impl
