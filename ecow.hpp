#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;

protected:
  using strvec = std::vector<std::string>;

  [[nodiscard]] static inline std::string cxx() {
    if (const char *exe = std::getenv("CXX")) {
      return exe;
    }
#ifdef __APPLE__
    return "/usr/local/opt/llvm/bin/clang++ -isysroot "
           "/Applications/Xcode.app/Contents/Developer/Platforms/"
           "MacOSX.platform/Developer/SDKs/MacOSX.sdk";
#else
    return "clang++";
#endif
  }
  [[nodiscard]] static inline std::string ld() {
    if (const char *exe = std::getenv("LD")) {
      return exe;
    }
    return "clang++";
  }

  [[nodiscard]] static inline std::string ext_of(const std::string &who) {
    if (std::filesystem::exists(who + ".cpp")) {
      return ".cpp";
    }
    if (std::filesystem::exists(who + ".mm")) {
      return ".mm";
    }
    return "";
  }

  [[nodiscard]] static inline bool run_clang(const std::string &args) {
    const auto cmd =
        cxx() + " -fobjc-arc -std=c++20 -fprebuilt-module-path=. " + args;
    std::cerr << cmd << std::endl;
    return std::system(cmd.c_str()) == 0;
  }

public:
  explicit unit(std::string name) : m_name{name} {}

  [[nodiscard]] constexpr const auto &name() const noexcept { return m_name; }

  [[nodiscard]] virtual bool build() {
    using namespace std::string_literals;
    const auto ext = ext_of(m_name);
    if (ext != "") {
      return run_clang(" -c " + m_name + ext + " -o " + m_name + ".o");
    } else {
      std::cerr << "Unit not found with '.cpp' or '.mm' extension: " << m_name
                << std::endl;
      return false;
    }
  }
  [[nodiscard]] virtual strvec objects() const {
    strvec res{};
    res.push_back(name());
    return res;
  }
};

class mod : public unit {

  strvec m_impls;
  strvec m_parts;

  [[nodiscard]] static bool compile(const std::string &who) {
    using namespace std::string_literals;
    return run_clang("--precompile "s + who + ".cppm -o " + who + ".pcm") &&
           run_clang("-c "s + who + ".pcm -o " + who + ".o");
  }
  [[nodiscard]] bool compile_impl(const std::string &who) {
    using namespace std::string_literals;
    return run_clang("-fmodule-file="s + name() + ".pcm -c " + who +
                     ".cpp -o " + who + ".o");
  }

  [[nodiscard]] auto parts() const {
    strvec res;
    std::transform(m_parts.begin(), m_parts.end(), std::back_inserter(res),
                   [this](auto p) { return name() + "-" + p; });
    return res;
  }

public:
  using unit::unit;
  void add_impl(std::string impl) { m_impls.push_back(impl); }
  void add_part(std::string part) { m_parts.push_back(part); }

  [[nodiscard]] bool build() override {
    const auto p = parts();
    return std::all_of(p.begin(), p.end(),
                       [this](auto w) { return compile(w); }) &&
           compile(name()) &&
           std::all_of(m_impls.begin(), m_impls.end(),
                       [this](auto w) { return compile_impl(w); });
  }
  [[nodiscard]] strvec objects() const override {
    const auto pts = parts();

    strvec res{};
    std::copy(pts.begin(), pts.end(), std::back_inserter(res));
    std::copy(m_impls.begin(), m_impls.end(), std::back_inserter(res));
    res.push_back(name());
    return res;
  }
};

class sys : public unit {
public:
  using unit::unit;

  [[nodiscard]] bool build() override {
    return std::system(name().c_str()) == 0;
  }
  [[nodiscard]] strvec objects() const override { return strvec(); }
};

class seq : public unit {
  std::vector<std::shared_ptr<unit>> m_units;

public:
  using unit::unit;

  template <typename Tp = unit> auto add_unit(const std::string &name) {
    auto res = std::make_shared<Tp>(name);
    m_units.push_back(res);
    return res;
  }
  void add_ref(std::shared_ptr<unit> ref) { m_units.push_back(ref); }

  [[nodiscard]] virtual bool build() override {
    return std::all_of(m_units.begin(), m_units.end(),
                       [](const auto &u) { return u->build(); });
  }
  [[nodiscard]] virtual strvec objects() const override {
    strvec res{};
    for (const auto &u : m_units) {
      const auto objs = u->objects();
      std::copy(objs.begin(), objs.end(), std::back_inserter(res));
    }
    return res;
  }
};

class exe : public seq {
public:
  using seq::seq;

  [[nodiscard]] bool build() override {
    if (!seq::build())
      return false;

    std::string cmd = ld() + " -o " + name() + ".exe";
    for (const auto &o : objects()) {
      using namespace std::string_literals;
      cmd.append(" "s + o + ".o");
    }

    std::cerr << cmd << std::endl;
    return std::system(cmd.c_str()) == 0;
  }
};
} // namespace ecow
