#pragma once

#include <cstdlib>
#include <string>

namespace ecow::impl {
[[nodiscard]] static inline std::string popen(const std::string &cmd) {
  auto f = ::popen(cmd.c_str(), "r");
  char buf[1024];
  std::string res{fgets(buf, 1024, f)};
  if (res.back() == '\n')
    res.pop_back();
  return res;
}

class target {
  std::string m_extra_cflags{};

public:
  target() : target("macosx") {}
  target(const char *sdk) {
    using namespace std::string_literals;
    m_extra_cflags =
        "-isysroot " + impl::popen("xcrun --show-sdk-path --sdk "s + sdk);
    if (sdk == "iphoneos"s) {
      m_extra_cflags += " -target arm64-apple-ios13.0";
    }
  }
  [[nodiscard]] std::string cxx() const {
    return "/usr/local/opt/llvm/bin/clang++ " + m_extra_cflags;
  }
  [[nodiscard]] std::string ld() const { return "clang++ " + m_extra_cflags; }
};
} // namespace ecow::impl
