#pragma once

#include "ecow.target.hpp"

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

class host_target : public target {
  std::string m_extra_cflags{};
  std::string m_extra_path{"Contents/MacOS"};
  std::string m_build_folder{};

protected:
  [[nodiscard]] std::string build_subfolder() const override {
    return m_build_folder;
  }

public:
  host_target() : host_target("macosx") {}
  host_target(const std::string &sdk) {
    using namespace std::string_literals;
    m_build_folder = sdk;
    m_extra_cflags =
        "-isysroot " + impl::popen("xcrun --show-sdk-path --sdk " + sdk);
    if (sdk == "iphoneos"s) {
      m_extra_cflags += " -target arm64-apple-ios13.0";
      m_extra_path = "";
    }
  }

  [[nodiscard]] std::string cxx() const override {
    return "/usr/local/opt/llvm/bin/clang++ " + m_extra_cflags;
  }
  [[nodiscard]] std::string ld() const override {
    return "clang++ " + m_extra_cflags;
  }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    auto path = name + ".app/" + m_extra_path;
    std::filesystem::create_directories(build_folder() + path);
    return path + "/" + name;
  }

  [[nodiscard]] virtual bool supports(features f) const override {
    switch (f) {
    case objective_c:
      return true;
    default:
      return false;
    }
  }
};
} // namespace ecow::impl
