#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class mod : public seq {
  strvec m_impls;
  strvec m_parts;

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
    const auto who_fn = std::filesystem::current_path() / pcm_name(who);
    const auto &dps = deps::dependency_map[who_fn];
    for (auto &p : m_parts) {
      const auto p_fn = std::filesystem::current_path() / (p + ".cppm");
      if (dps.contains(p_fn)) {
        build_part_after_deps(p);
      }
    }
    compile_part(who);
  }

protected:
  void build_self() const override {
    for (const auto &p : m_parts) {
      build_part_after_deps(p);
    }

    compile_part(name());
    std::for_each(m_impls.begin(), m_impls.end(),
                  [this](auto w) { return compile_impl(w); });
    seq::build_self();
  }
  void create_self_cdb(std::ostream &o) const override {
    std::for_each(m_parts.begin(), m_parts.end(),
                  [&](auto w) { clang_part(w).create_cdb(o); });
    clang_part(name()).create_cdb(o);
    std::for_each(m_impls.begin(), m_impls.end(),
                  [&](auto w) { clang_impl(w).create_cdb(o); });
    seq::create_self_cdb(o);
  }

  [[nodiscard]] pathset self_objects() const override {
    pathset res = seq::self_objects();
    res.insert(obj_name(name()));
    std::for_each(m_parts.begin(), m_parts.end(),
                  [&](auto w) { res.insert(obj_name(w)); });
    std::for_each(m_impls.begin(), m_impls.end(),
                  [&](auto w) { res.insert(obj_name(w)); });
    return res;
  }

public:
  using seq::add_unit;
  using seq::seq;

  void add_impl(std::string impl) { m_impls.push_back(impl); }
  void add_part(std::string part) { m_parts.push_back(name() + "-" + part); }

  auto main_cpp_file() {
    return std::filesystem::current_path() / (name() + ".cppm");
  }
  auto main_pcm_file() {
    return std::filesystem::current_path() / (pcm_name(name()));
  }
};
} // namespace ecow
