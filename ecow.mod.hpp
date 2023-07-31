#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class mod : public seq {
  strvec m_impls;
  strvec m_parts;

  [[nodiscard]] auto part_prefix() const { return name() + ":"; }
  [[nodiscard]] auto part_name(const std::string &who) const {
    return part_prefix() + who;
  }
  [[nodiscard]] auto part_file(const std::string &who) const {
    return name() + "-" + who;
  }

  [[nodiscard]] auto clang_part(const std::string &who) const {
    return impl::clang{who + ".cppm", pcm_name(who)}
        .add_arg("--precompile")
        .add_include_dirs(include_dirs())
        .with_deps();
  }
  [[nodiscard]] auto clang_impl(const std::string &who) const {
    return impl::clang{who + ".cpp", obj_name(who)}
        .add_arg("-c")
        .add_include_dirs(include_dirs())
        .with_deps();
  }

  void compile_part(const std::string &who) const {
    clang_part(who).run();
    impl::clang{pcm_name(who), obj_name(who)}.add_arg("-c").run();
  }
  void compile_impl(const std::string &who) const { clang_impl(who).run(); }

  void build_part_after_deps(const std::string &who) const {
    const auto &dps = deps::dependency_map[part_name(who)];
    for (auto &p : m_parts) {
      if (dps.contains(part_name(p))) {
        build_part_after_deps(p);
      }
    }
    compile_part(part_file(who));
  }

  [[nodiscard]] strset auto_parts() const {
    strset res{};
    auto pp = part_prefix();
    for (auto &d : deps::dependency_map[name()]) {
      if (!d.starts_with(pp))
        continue;

      auto p = d.substr(pp.size());
      auto pf = part_file(p) + ".cppm";
      if (!std::filesystem::exists(pf))
        continue;

      res.insert(p);
    }
    for (auto &p : m_parts) {
      res.insert(p);
    }
    return res;
  }

protected:
  void build_self() const override {
    for (const auto &p : auto_parts()) {
      build_part_after_deps(p);
    }

    compile_part(name());
    std::for_each(m_impls.begin(), m_impls.end(),
                  [this](auto w) { return compile_impl(w); });
    seq::build_self();
  }
  void calculate_self_deps() override {
    clang_part(name()).generate_deps();

    for (auto &w : auto_parts()) {
      clang_part(part_file(w)).generate_deps();
    }

    std::for_each(m_impls.begin(), m_impls.end(),
                  [&](auto w) { clang_impl(w).generate_deps(); });
    seq::calculate_self_deps();
  }

  [[nodiscard]] pathset self_objects() const override {
    pathset res = seq::self_objects();
    res.insert(obj_name(name()));
    for (auto &w : auto_parts()) {
      res.insert(obj_name(part_file(w)));
    }
    std::for_each(m_impls.begin(), m_impls.end(),
                  [&](auto w) { res.insert(obj_name(w)); });
    return res;
  }

public:
  using seq::add_unit;
  using seq::seq;

  void add_impl(std::string impl) { m_impls.push_back(impl); }
  void add_part(std::string part) { m_parts.push_back(part); }

  [[nodiscard]] const auto &module_name() const { return name(); }
};
} // namespace ecow
