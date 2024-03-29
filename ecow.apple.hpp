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

  void array(const std::string &key, auto &&...v) {
    o << "<key>" << key << "</key><array>\n";
    ((o << "<string>" << v << "</string>"), ...);
    o << "</array>\n";
  }
  void boolean(const std::string &key, bool v) {
    o << "<key>" << key << "</key>";
    o << (v ? "<true/>" : "<false/>");
    o << "\n";
  }
  void date(const std::string &key) {
    time_t now;
    time(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&now));
    o << "<key>" << key << "</key><date>" << buf << "</date>\n";
  }
  void dictionary(const std::string &key, auto &&fn) {
    o << "<key>" << key << "</key><dict>\n";
    fn(dict{o});
    o << "</dict>\n";
  }
  void integer(const std::string &key, int value) {
    o << "<key>" << key << "</key><integer>" << value << "</integer>\n";
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

protected:
  [[nodiscard]] auto bundle_path(const std::string &name) const {
    return build_path() / app_path() / (name + ".app");
  }

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
    fn(plist::dict{o});
    o << R"(</dict>
</plist>
)";
  }

  void build_common_app_plist(const std::string &name, plist::dict &d) const {
    d.string("CFBundleDevelopmentRegion", "en");
    d.string("CFBundleDisplayName", name);
    d.string("CFBundleExecutable", name);
    d.string("CFBundleIdentifier", "br.com.tpk." + name);
    d.string("CFBundleInfoDictionaryVersion", "6.0");
    d.string("CFBundlePackageType", "APPL");
    d.string("CFBundleShortVersionString", "1.0.0");
    d.string("CFBundleVersion", "1.0.0");
    d.string("DTPlatformName", m_sdk);
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
  [[nodiscard]] std::string exe_name(const std::string &name) const override {
    return name;
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
  [[nodiscard]] static std::string env(const char *key) {
    const auto v = std::getenv(key);
    return (v == nullptr) ? "TBD" : std::string{v};
  }

  [[nodiscard]] static std::string team_id() { return env("ECOW_IOS_TEAM"); }

  void gen_app_plist(const std::string &name) const {
    gen_plist(bundle_path(name) / "Info.plist", [&](auto d) {
      build_common_app_plist(name, d);
      d.array("CFBundleSupportedPlatforms", "iPhoneOS");
      d.string("MinimumOSVersion", "13.0");
      d.boolean("LSRequiresIPhoneOS", true);
      d.integer("UIDeviceFamily", 1); // iPhone
    });
  }

  void gen_archive_plist(const std::string &name) const {
    gen_plist(build_path() / "export.xcarchive/Info.plist", [&](auto d) {
      d.dictionary("ApplicationProperties", [&](auto dd) {
        dd.string("ApplicationPath", "Applications/" + name + ".app");
        dd.array("Architectures", "arm64");
        dd.string("CFBundleIdentifier", "br.com.tpk." + name);
        dd.string("CFBundleShortVersionString", "1.0.0");
        dd.string("CFBundleVersion", "0");
        dd.string("SigningIdentity", env("ECOW_IOS_SIGN_ID"));
        dd.string("Team", team_id());
      });
      d.integer("ArchiveVersion", 1);
      d.date("CreationDate");
      d.string("Name", name);
      d.string("SchemeName", name);
    });
  }

  void gen_export_plist(const std::string &name) const {
    gen_plist(build_path() / "export.plist", [&](auto d) {
      d.string("method", "ad-hoc");
      d.string("teamID", team_id());
      d.string("thinning", "&lt;none&gt;");
      d.dictionary("provisioningProfiles", [&](auto dd) {
        dd.string("br.com.tpk." + name, env("ECOW_IOS_PROV_PROF"));
      });
    });
  }

  void code_sign(const std::string &name) const {
    const auto app = build_path() / app_path() / name;
    const auto cmd =
        "codesign -f -s " + team_id() + " " + app.string() + ".app";

    std::cerr << "codesigning " << name << std::endl;
    if (std::system(cmd.c_str()) != 0) {
      throw command_failed(cmd);
    }
  }

  void run_export(const std::string &name) const {
    const auto xcarchive = build_path() / "export.xcarchive";
    const auto exp_path = build_path() / "export";
    const auto exp_plist = build_path() / "export.plist";
    const auto cmd = "xcodebuild -exportArchive -archivePath " +
                     xcarchive.string() + " -exportPath " + exp_path.string() +
                     " -exportOptionsPlist " + exp_plist.string();

    std::cerr << "exporting " << name << std::endl;
    if (std::system(cmd.c_str()) != 0) {
      throw command_failed(cmd);
    }
  }

protected:
  [[nodiscard]] std::string app_path() const override {
    return "export.xcarchive/Products/Applications/";
  }
  [[nodiscard]] std::string exe_path() const override { return ""; }
  [[nodiscard]] std::string res_path() const override { return ""; }

public:
  explicit iphone_target() : apple_target("iphoneos") {
    add_flags("-target", triple());
    add_ldflags("-Wl,-rpath,@executable_path");
  }
  void bundle(const std::string &name, const unit &u) const override {
    gen_app_plist(name);
    gen_export_plist(name);
    gen_archive_plist(name);
    if (team_id() == "TBD") {
      std::cerr << "skipping code sign/export" << std::endl;
    } else {
      code_sign(name);
      run_export(name);
    }
  }

  [[nodiscard]] std::string triple() const override {
    return "arm64-apple-ios13.0";
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
    add_flags("-target", triple());
    add_ldflags("-Wl,-rpath,@executable_path");
  }
  void bundle(const std::string &name, const unit &u) const override {
    gen_plist(bundle_path(name) / "Info.plist", [&](auto d) {
      build_common_app_plist(name, d);
      d.boolean("LSRequiresIPhoneOS", true);
    });
  }

  [[nodiscard]] std::string triple() const override {
    return "x86_64-apple-ios13.0-simulator";
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

  [[nodiscard]] std::string triple() const override {
    return "x86_64-apple-macosx11.6.0";
  }

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

std::filesystem::path clang_dir() {
  return std::filesystem::path{impl::popen("brew --prefix llvm@16")};
}
} // namespace ecow::impl
