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

  void gen_app_plist(const std::string &name) const {
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
    } else if (sdk == "iphonesimulator"s) {
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

    gen_app_plist(name);
    gen_export_plist(name);
    gen_archive_plist(name);
    run_export();
  }
};

class apple_target : public target {
  std::vector<std::string> m_cflags{};
  std::vector<std::string> m_ldflags{};
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

  void gen_app_plist(const std::string &name) const {
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

  void add_cflags(const auto &...as) { (m_cflags.push_back(as), ...); }
  void add_ldflags(const auto &...as) { (m_ldflags.push_back(as), ...); }
  void add_flags(const auto &...as) {
    add_cflags(as...);
    add_ldflags(as...);
  }

public:
  explicit apple_target(const std::string &sdk) : m_sdk{sdk} {
    add_flags("--sysroot", impl::popen("xcrun --show-sdk-path --sdk " + sdk));
  }
  [[nodiscard]] std::string cxxflags() const override {
    std::ostringstream o{};
    for (const auto &s : m_cflags)
      o << " " << s;
    return o.str();
  }
  [[nodiscard]] std::string ldflags() const override {
    std::ostringstream o{};
    for (const auto &s : m_ldflags)
      o << " " << s;
    return o.str();
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
} // namespace ecow::impl
