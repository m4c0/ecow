#pragma once

#include <cstdlib>
#include <filesystem>
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
  std::string m_extra_path{"Contents/MacOS"};
  std::string m_build_folder{};

public:
  target() : target("macosx") {}
  target(const char *sdk) {
    using namespace std::string_literals;
    m_build_folder = "out/"s + sdk + "/";
    m_extra_cflags =
        "-isysroot " + impl::popen("xcrun --show-sdk-path --sdk "s + sdk);
    if (sdk == "iphoneos"s) {
      m_extra_cflags += " -target arm64-apple-ios13.0";
      m_extra_path = "";
    }
  }

  [[nodiscard]] std::string cxx() const {
    return "/usr/local/opt/llvm/bin/clang++ " + m_extra_cflags;
  }
  [[nodiscard]] std::string ld() const { return "clang++ " + m_extra_cflags; }

  [[nodiscard]] std::string app_exe_name(const std::string &name) const {
    auto path = name + ".app/" + m_extra_path;
    std::filesystem::create_directories(m_build_folder + path);
    return path + "/" + name;
  }

  [[nodiscard]] std::string build_folder() const { return m_build_folder; }
};
} // namespace ecow::impl
