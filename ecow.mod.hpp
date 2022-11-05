#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class mod : public seq {
  strvec m_impls;
  strvec m_parts;

  [[nodiscard]] static bool compile_part(const std::string &who) {
    return impl::run_clang("--precompile", who + ".cppm", pcm_name(who)) &&
           impl::run_clang("-c", pcm_name(who), obj_name(who));
  }
  [[nodiscard]] bool compile_impl(const std::string &who) {
    using namespace std::string_literals;
    return impl::run_clang("-fmodule-file="s + pcm_name(name()) + " -c",
                           who + ".cpp", obj_name(who));
  }

  static void remove_part(const std::string &who) {
    impl::remove(pcm_name(who));
    impl::remove(obj_name(who));
  }
  static void remove_impl(const std::string &who) {
    impl::remove(obj_name(who));
  }

public:
  using seq::add_unit;
  using seq::seq;
  void add_impl(std::string impl) { m_impls.push_back(impl); }
  void add_part(std::string part) { m_parts.push_back(name() + "-" + part); }

  [[nodiscard]] bool build(const std::string &flags = "") override {
    return std::all_of(m_parts.begin(), m_parts.end(),
                       [this](auto w) { return compile_part(w); }) &&
           compile_part(name()) &&
           std::all_of(m_impls.begin(), m_impls.end(),
                       [this](auto w) { return compile_impl(w); }) &&
           seq::build(flags);
  }
  void clean() override {
    std::for_each(m_parts.begin(), m_parts.end(),
                  [](const auto &u) { return remove_part(u); });
    remove_part(name());
    std::for_each(m_impls.begin(), m_impls.end(),
                  [](const auto &u) { return remove_impl(u); });
    seq::clean();
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
