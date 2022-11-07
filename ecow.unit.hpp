#pragma once

#include "ecow.core.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;
  std::unordered_set<std::string> m_link_flags{};

protected:
  using strvec = std::vector<std::string>;
  using strset = std::unordered_set<std::string>;

  [[nodiscard]] static auto pcm_name(const std::string &who) {
    return impl::current_target()->build_folder() + who + ".pcm";
  }
  [[nodiscard]] static auto obj_name(const std::string &who) {
    return impl::current_target()->build_folder() + who + ".o";
  }

  [[nodiscard]] constexpr const auto &name() const noexcept { return m_name; }
  [[nodiscard]] bool target_supports(impl::target::features f) const noexcept {
    return impl::current_target()->supports(f);
  }

  void add_link_flag(const std::string &name) { m_link_flags.insert(name); }

public:
  using feats = impl::target::features;

  explicit unit(std::string name) : m_name{name} {}

  void add_system_library(const std::string &name) {
    add_link_flag("-l" + name);
  }

  virtual void build(const std::string &flags = "") {
    const auto ext =
        std::filesystem::path{m_name}.has_extension() ? "" : ".cpp";
    impl::run_clang(flags + " -c", m_name + ext, obj_name(m_name));
  }

  [[nodiscard]] virtual strset link_flags() const { return m_link_flags; }

  [[nodiscard]] virtual strvec objects() const {
    strvec res{};
    res.push_back(name());
    return res;
  }
};
} // namespace ecow
