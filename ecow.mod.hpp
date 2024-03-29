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

  const auto &deps_of(const std::string &who) const {
    if (!deps::has(who)) {
      for (auto &d : clang_part(who).generate_deps()) {
        deps::add(who, d);
        deps::add(name(), d);
      }
    }
    return deps::of(who);
  }

  void calculate_deps_of(const std::string &who) {
    const auto pp = name() + ":";
    for (auto &d : deps_of(who)) {
      if (d.starts_with(pp)) {
        auto pn = name() + "-" + d.substr(pp.size());
        calculate_deps_of(pn);
      }
    }
    if (who.starts_with(pp)) {
      auto pn = who.substr(pp.size());
      if (std::find(m_parts.begin(), m_parts.end(), pn) == m_parts.end())
        add_part(pn);
    }
  }

  void build_with_deps(const std::string &who) const {
    const auto pp = name() + ":";
    for (auto &d : deps::of(who)) {
      if (d.starts_with(pp)) {
        auto pn = name() + "-" + d.substr(pp.size());
        build_with_deps(pn);
      }
    }
    compile_part(who);
  }

  void objs_with_deps(pathset &res, const std::string &who) const {
    const auto pp = name() + ":";
    for (auto &d : deps::of(who)) {
      if (d.starts_with(pp)) {
        auto pn = name() + "-" + d.substr(pp.size());
        objs_with_deps(res, pn);
      }
    }
    res.insert(obj_name(who));
  }

  [[nodiscard]] strvec auto_impls() const {
    auto impls = impl::current_target()->build_path() / (name() + ".impls");
    if (!std::filesystem::exists(impls))
      return {};

    std::ifstream in{impls};
    strvec res{};
    while (in) {
      std::string line;
      in >> line;
      if (line != "")
        res.push_back(line);
    }

    return res;
  }

  void calculate_deps_of_impl(const std::string &who) {
    for (auto &d : clang_impl(who).generate_deps()) {
      deps::add(name(), d);
    }
  }

protected:
  void build_self() const override {
    std::for_each(m_parts.begin(), m_parts.end(),
                  [this](auto w) { return compile_part(name() + "-" + w); });
    build_with_deps(name());
    std::for_each(m_impls.begin(), m_impls.end(),
                  [this](auto w) { return compile_impl(w); });
    for (const auto &w : auto_impls()) {
      compile_impl(w);
    }
    seq::build_self();
  }
  void calculate_self_deps() override {
    for (const auto &impl : m_impls) {
      calculate_deps_of_impl(impl);
    }
    // TODO: auto_impls only works after first build
    for (const auto &impl : auto_impls()) {
      calculate_deps_of_impl(impl);
    }
    calculate_deps_of(name());

    std::for_each(m_parts.begin(), m_parts.end(),
                  [this](auto w) { calculate_deps_of(name() + "-" + w); });
  }

  [[nodiscard]] pathset self_objects() const override {
    pathset res = seq::self_objects();
    objs_with_deps(res, name());
    for (auto &w : m_parts) {
      res.insert(obj_name(name() + "-" + w));
    }
    std::for_each(m_impls.begin(), m_impls.end(),
                  [&](auto w) { res.insert(obj_name(w)); });
    for (const auto &w : auto_impls()) {
      res.insert(obj_name(w));
    }
    return res;
  }

public:
  using seq::add_unit;
  using seq::seq;

  void add_impl(std::string impl) { m_impls.push_back(impl); }
  void add_part(std::string part) { m_parts.push_back(part); }

  [[nodiscard]] strset link_flags() const override {
    auto res = seq::link_flags();

    auto flags = impl::current_target()->build_path() / (name() + ".flags");
    if (std::filesystem::exists(flags)) {
      const auto p = std::filesystem::current_path() / flags;
      res.insert("@" + p.string());
    }

    return res;
  }
};
} // namespace ecow
