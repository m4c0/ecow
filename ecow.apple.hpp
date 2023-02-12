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
  std::string m_exe_path{"Contents/MacOS/"};
  std::string m_res_path{"Contents/Resources/"};
  std::string m_build_folder{};
  features m_main_api{cocoa};

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
        "--sysroot " + impl::popen("xcrun --show-sdk-path --sdk " + sdk);
    if (sdk == "iphoneos"s) {
      m_extra_cflags += " -target arm64-apple-ios13.0";
      m_exe_path = "";
      m_res_path = "";
      m_main_api = uikit;
    } else if (sdk == "iphonesimulator"s) {
      m_extra_cflags += " -target arm64-apple-ios13.0-simulator";
      m_exe_path = "";
      m_res_path = "";
      m_main_api = uikit;
    } else if (sdk == "macosx") {
      m_extra_cflags += " -mmacosx-version-min=11.6";
    } else {
      throw std::runtime_error("invalid target");
    }
  }

  [[nodiscard]] std::string cxxflags() const override { return m_extra_cflags; }
  [[nodiscard]] std::string ld() const override {
    return cxx() + " " + m_extra_cflags;
  }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    auto path = name + ".app/" + m_exe_path;
    std::filesystem::create_directories(build_folder() + path);
    return path + name;
  }
  [[nodiscard]] std::filesystem::path
  resource_path(const std::string &name) const override {
    const auto res =
        std::filesystem::path{build_folder()} / (name + ".app") / m_res_path;
    std::filesystem::create_directories(res);
    return res;
  }

  [[nodiscard]] virtual bool supports(features f) const override {
    switch (f) {
    case cocoa:
    case uikit:
      return f == m_main_api;
    case objective_c:
    case posix:
      return true;
    default:
      return false;
    }
  }
};
} // namespace ecow::impl
