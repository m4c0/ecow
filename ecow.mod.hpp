#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class mod : public seq {
  strvec m_impls;
  strvec m_parts;

  static void compile_part(const std::string &who) {
    impl::run_clang_with_deps("--precompile", who + ".cppm", pcm_name(who));
    impl::run_clang("-c", pcm_name(who), obj_name(who));
  }
  void compile_impl(const std::string &who) {
    using namespace std::string_literals;
    impl::run_clang_with_deps("-fmodule-file="s + pcm_name(name()) + " -c",
                              who + ".cpp", obj_name(who));
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
