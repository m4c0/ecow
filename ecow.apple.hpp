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
  std::string m_extra_ldflags{};
  std::string m_exe_path{"Contents/MacOS/"};
  std::string m_res_path{"Contents/Resources/"};
  std::string m_build_folder{};
  std::string m_app_path{};
  features m_main_api{cocoa};

  [[nodiscard]] auto bundle_path(const std::string &name) const {
    return build_path() / m_app_path / (name + ".app");
  }

  void gen_plist(const std::filesystem::path &path, auto fn) const {
    std::cerr << "generating " << path.string() << std::endl;
    std::ofstream o{path};
    o << R"(
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
  <dict>
)";
    fn(o);
    o << R"(
  </dict>
</plist>
)";
  }

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
      m_extra_ldflags = " -Wl,-rpath,@executable_path";
      m_exe_path = "";
      m_res_path = "";
      m_main_api = uikit;
      m_app_path = "export.xcarchive/Product/Applications/";
    } else if (sdk == "iphonesimulator"s) {
      m_extra_cflags += " -target x86_64-apple-ios13.0-simulator";
      m_extra_ldflags = " -Wl,-rpath,@executable_path";
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
  [[nodiscard]] std::string ldflags() const override {
    return m_extra_cflags + m_extra_ldflags;
  }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    auto path = m_app_path + name + ".app/" + m_exe_path;
    std::filesystem::create_directories(build_folder() + path);
    return path + name;
  }
  [[nodiscard]] std::filesystem::path
  resource_path(const std::string &name) const override {
    const auto res = bundle_path(name) / m_res_path;
    std::filesystem::create_directories(res);
    return res;
  }

  [[nodiscard]] virtual bool supports(features f) const override {
    switch (f) {
    case cocoa:
    case uikit:
      return f == m_main_api;
    case host:
      return m_main_api == cocoa;
    case application:
    case native:
    case objective_c:
    case posix:
      return true;
    default:
      return false;
    }
  }

  void bundle(const std::string &name, const unit &u) const override {
    if (m_main_api == cocoa)
      return;

    gen_plist(bundle_path(name) / "Info.plist", [&](auto &o) {
      o << R"(
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleDisplayName</key>
    <string>)"
        << name << R"(</string>
    <key>CFBundleExecutable</key>
    <string>)"
        << name << R"(</string>
    <key>CFBundleIdentifier</key>
    <string>br.com.tpk.)"
        << name << R"(</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>LSRequiresIPhoneOS</key>
    <true/>
)";
    });
  }
};
} // namespace ecow::impl
