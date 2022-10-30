#include <format>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

[[nodiscard]] inline bool run_clang(const std::string &args) {
  constexpr const auto clang_fmt =
      "clang++ -std=c++20 -fprebuilt-module-path=. {}";
  const auto cmd = std::format(clang_fmt, args);
  std::cerr << cmd << std::endl;
  return std::system(cmd.c_str()) == 0;
}

class unit {
  std::string m_name;

public:
  explicit unit(std::string name) : m_name{name} {}

  [[nodiscard]] constexpr const auto &name() const noexcept { return m_name; }

  [[nodiscard]] virtual bool build() {
    constexpr const auto obj_fmt = "-c {0:}.cpp -o {0:}.o";
    return run_clang(std::format(obj_fmt, name()));
  }
  [[nodiscard]] virtual std::vector<std::string> objects() const {
    std::vector<std::string> res{};
    res.push_back(name());
    return res;
  }
};

class mod : public unit {
  static constexpr const auto pcm_fmt = "--precompile {0:}.cppm -o {0:}.pcm";
  static constexpr const auto obj_fmt = "-c {0:}.pcm -o {0:}.o";
  static constexpr const auto impl_fmt =
      "-fmodule-file={0}.pcm -c {1:}.cpp -o {1:}.o";

  std::vector<std::string> m_impls;
  std::vector<std::string> m_parts;

  [[nodiscard]] static bool compile(const std::string &who) {
    return run_clang(std::format(pcm_fmt, who)) &&
           run_clang(std::format(obj_fmt, who));
  }
  [[nodiscard]] auto part_name(const std::string &who) const {
    return std::format("{}-{}", name(), who);
  }

  [[nodiscard]] auto parts() const {
    return m_parts |
           std::views::transform([this](auto p) { return part_name(p); });
  }

public:
  using unit::unit;
  void add_impl(std::string impl) { m_impls.push_back(impl); }
  void add_part(std::string part) { m_parts.push_back(part); }

  [[nodiscard]] bool build() override {
    if (!std::ranges::all_of(objects(), &mod::compile))
      return false;

    for (const auto &i : m_impls) {
      // fails to import std, for reasons
      // if (!run_clang(std::format(impl_fmt, m_name, i))) return false;
    }

    return true;
  }
  [[nodiscard]] virtual std::vector<std::string> objects() const override {
    std::vector<std::string> res{};
    std::ranges::copy(parts(), std::back_inserter(res));
    res.push_back(name());
    return res;
  }
};

class exe : public unit {
  std::vector<std::unique_ptr<unit>> m_units;

public:
  using unit::unit;
  template <typename Tp = unit> Tp *add_unit(const std::string &name) {
    Tp *res = new Tp{name};
    m_units.push_back(std::unique_ptr<unit>(res));
    return res;
  }

  [[nodiscard]] bool build() override {
    if (!std::ranges::all_of(m_units, &unit::build))
      return false;

    // auto objs = objects() | std::views::join;

    // const auto cmd = std::format("clang++ -o {}.exe ", name()) + objs;
    // std::cerr << cmd << std::endl;
    // return std::system(cmd.c_str()) == 0;
    return true;
  }
  [[nodiscard]] virtual std::vector<std::string> objects() const override {
    auto all =
        m_units | std::views::transform(&unit::objects) | std::views::join;
    std::vector<std::string> res{};
    std::ranges::copy(all, std::back_inserter(res));
    return res;
  }
};

int main() {
  exe a{"a"};

  auto *m = a.add_unit<mod>("m");
  m->add_part("interface_part");
  m->add_part("impl_part");
  m->add_impl("impl");

  a.add_unit<>("user");
  return a.build() ? 0 : 1;
}
