#pragma once

#include "ecow.target.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace ecow::impl::plist {
class dict {
  std::ostream &o;

public:
  explicit constexpr dict(std::ostream &o) : o{o} {}

  void boolean(const std::string &key, bool v) {
    o << "<key>" << key << "</key>";
    o << (v ? "<true/>" : "<false/>");
    o << "\n";
  }
  void string(const std::string &key, const std::string &value) {
    o << "<key>" << key << "</key><string>" << value << "</string>\n";
  }
};
} // namespace ecow::impl::plist
namespace ecow::impl {
[[nodiscard]] static inline std::string popen(const std::string &cmd) {
  auto f = ::popen(cmd.c_str(), "r");
  char buf[1024];
  std::string res{fgets(buf, 1024, f)};
  if (res.back() == '\n')
    res.pop_back();
  return res;
}

class apple_target : public target {
  std::string m_sdk;

  [[nodiscard]] auto bundle_path(const std::string &name) const {
    return build_path() / app_path() / (name + ".app");
  }

protected:
  [[nodiscard]] virtual std::string app_path() const = 0;
  [[nodiscard]] virtual std::string exe_path() const = 0;
  [[nodiscard]] virtual std::string res_path() const = 0;

  [[nodiscard]] std::string build_subfolder() const override { return m_sdk; }

  void gen_plist(const std::filesystem::path &path, auto fn) const {
    std::cerr << "generating " << path.string() << std::endl;
    std::ofstream o{path};
    o << R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
)";
    fn(o);
    o << R"(</dict>
</plist>
)";
  }

  void gen_app_plist(const std::string &name) const {
    gen_plist(bundle_path(name) / "Info.plist", [&](auto &o) {
      plist::dict d{o};
      d.string("CFBundleDevelopmentRegion", "en");
      d.string("CFBundleDisplayName", name);
      d.string("CFBundleExecutable", name);
      d.string("CFBundleIdentifier", "br.com.tpk." + name);
      d.string("CFBundleInfoDictionaryVersion", "6.0");
      d.string("CFBundlePackageType", "APPL");
      d.string("CFBundleShortVersionString", "1.0.0");
      d.string("CFBundleVersion", "1.0.0");
      d.string("DTPlatformName", m_sdk);
      d.boolean("LSRequiresIPhoneOS", true);
    });
  }

public:
  explicit apple_target(const std::string &sdk) : m_sdk{sdk} {
    add_flags("--sysroot", impl::popen("xcrun --show-sdk-path --sdk " + sdk));
  }

  [[nodiscard]] std::string
  app_exe_name(const std::string &name) const override {
    auto path = app_path() + name + ".app/" + exe_path();
    std::filesystem::create_directories(build_folder() + path);
    return path + name;
  }
  [[nodiscard]] std::filesystem::path
  resource_path(const std::string &name) const override {
    const auto res = bundle_path(name) / res_path();
    std::filesystem::create_directories(res);
    return res;
  }

  [[nodiscard]] virtual bool supports(features f) const override {
    switch (f) {
    case application:
    case native:
    case objective_c:
    case posix:
      return true;
    default:
      return false;
    }
  }
};

class iphone_target : public apple_target {
  void gen_archive_plist(const std::string &name) const {
    gen_plist(build_path() / "export.xcarchive/Info.plist", [&](auto &o) {
      o << R"(
    <key>ApplicationProperties</key>
    <dict>
      <key>ApplicationPath</key>
      <string>Applications/)"
        << name << R"(.app</string>
      <key>Architectures</key>
      <array>
        <string>armv7</string>
        <string>arm64</string>
      </array>
      <key>CFBundleIdentifier</key>
      <string>br.com.tpk.)"
        << name << R"(</string>
      <key>CFBundleShortVersionString</key>
      <string>1.0.0</string>
      <key>CFBundleVersion</key>
      <string>0</string>
      <key>SigningIdentity</key>
      <string>TBD</string>
      <key>Team</key>
      <string>TBD</string>
    </dict>
    <key>ArchiveVersion</key>
    <integer>1</integer>
    <key>CreationDate</key>
    <date>2023-03-30T00:00:00Z</date>
    <key>Name</key>
    <string>)"
        << name << R"(</string>
    <key>SchemeName</key>
    <string>)"
        << name << R"(</string>
)";
    });
  }

  void gen_export_plist(const std::string &name) const {
    gen_plist(build_path() / "export.plist", [&](auto &o) {
      o << R"(
    <key>method</key>
    <string>development</string>
    <key>teamID</key>
    <string>TBD</string>
    <key>thinning</key>
    <string>&lt;none&gt;</string>
    <key>provisioningProfiles</key>
    <dict>
      <key>br.com.tpk.)"
        << name << R"(</key>
      <string>TBD</string>
    </dict>)";
    });
  }

  void run_export() const {
    // xcodebuild -exportArchive -archivePath export.xcarchive -exportPath ...
    // -exportOptionsPlist export.plist
  }

protected:
  [[nodiscard]] std::string app_path() const override {
    return "export.xcarchive/Products/Applications/";
  }
  [[nodiscard]] std::string exe_path() const override { return ""; }
  [[nodiscard]] std::string res_path() const override { return ""; }

public:
  explicit iphone_target() : apple_target("iphoneos") {
    add_flags("-target", "arm64-apple-ios13.0");
    add_ldflags("-Wl,-rpath,@executable_path");
  }
  void bundle(const std::string &name, const unit &u) const override {
    gen_app_plist(name);
    gen_export_plist(name);
    gen_archive_plist(name);
    run_export();
  }

  [[nodiscard]] virtual bool supports(features f) const override {
    switch (f) {
    case uikit:
      return true;
    default:
      return apple_target::supports(f);
    }
  }
};

class iphonesimulator_target : public apple_target {
protected:
  [[nodiscard]] std::string app_path() const override { return ""; }
  [[nodiscard]] std::string exe_path() const override { return ""; }
  [[nodiscard]] std::string res_path() const override { return ""; }

public:
  explicit iphonesimulator_target() : apple_target("iphonesimulator") {
    add_flags("-target", "x86_64-apple-ios13.0-simulator");
    add_ldflags("-Wl,-rpath,@executable_path");
  }
  void bundle(const std::string &name, const unit &u) const override {
    gen_app_plist(name);
  }

  [[nodiscard]] virtual bool supports(features f) const override {
    switch (f) {
    case uikit:
      return true;
    default:
      return apple_target::supports(f);
    }
  }
};

class host_target : public apple_target {
protected:
  [[nodiscard]] std::string app_path() const override { return ""; }
  [[nodiscard]] std::string exe_path() const override {
    return "Contents/MacOS/";
  }
  [[nodiscard]] std::string res_path() const override {
    return "Contents/Resources/";
  }

public:
  explicit host_target() : apple_target("macosx") {
    add_flags("-mmacosx-version-min=11.6");
  }
  void bundle(const std::string &name, const unit &u) const override {}

  [[nodiscard]] virtual bool supports(features f) const override {
    switch (f) {
    case cocoa:
    case host:
      return true;
    default:
      return apple_target::supports(f);
    }
  }
};
} // namespace ecow::impl
