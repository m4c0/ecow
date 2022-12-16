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
  void compile_impl(const std::string &who) {
    impl::clang{who + ".cpp", obj_name(who)}
        .add_arg("-c")
        .add_arg("-fmodule-file=" + pcm_name(name()))
        .with_deps()
        .run();
  }

public:
  using seq::add_unit;
  using seq::seq;

  void add_impl(std::string impl) { m_impls.push_back(impl); }
  void add_part(std::string part) { m_parts.push_back(name() + "-" + part); }

  void build() override {
    std::for_each(m_parts.begin(), m_parts.end(),
                  [this](auto w) { return compile_part(w); });
    compile_part(name());
    std::for_each(m_impls.begin(), m_impls.end(),
                  [this](auto w) { return compile_impl(w); });
    seq::build();
  }

  [[nodiscard]] strvec objects() const override {
    const auto super = seq::objects();

    strvec res{};
    std::copy(m_parts.begin(), m_parts.end(), std::back_inserter(res));
    std::copy(m_impls.begin(), m_impls.end(), std::back_inserter(res));
    res.push_back(name());
    std::copy(super.begin(), super.end(), std::back_inserter(res));
    return res;
  }
};
} // namespace ecow
