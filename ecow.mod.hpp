#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class mod : public seq {
  strvec m_impls;
  strvec m_parts;

  static void compile_part(const std::string &who) {
    impl::clang{who + ".cppm", pcm_name(who)}
        .add_arg("--precompile")
        .with_deps()
        .run();
    impl::clang{pcm_name(who), obj_name(who)}.add_arg("-c").run();
  }
  void compile_impl(const std::string &who) const {
    impl::clang{who + ".cpp", obj_name(who)}
        .add_arg("-c")
        .add_arg("-fmodule-file=" + pcm_name(name()))
        .with_deps()
        .run();
  }

protected:
  void build_self() const override {
    std::for_each(m_parts.begin(), m_parts.end(),
                  [this](auto w) { return compile_part(w); });
    compile_part(name());
    std::for_each(m_impls.begin(), m_impls.end(),
                  [this](auto w) { return compile_impl(w); });
    seq::build_self();
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
};
} // namespace ecow
