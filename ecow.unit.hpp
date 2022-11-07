#pragma once

#include "ecow.core.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;

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

public:
  using feats = impl::target::features;

  explicit unit(std::string name) : m_name{name} {}

  [[nodiscard]] virtual bool build(const std::string &flags = "") {
    const auto ext =
        std::filesystem::path{m_name}.has_extension() ? "" : ".cpp";
    return impl::run_clang(flags + " -c", m_name + ext, obj_name(m_name));
  }

  [[nodiscard]] virtual strset frameworks() const { return {}; }

  [[nodiscard]] virtual strvec objects() const {
    strvec res{};
    res.push_back(name());
    return res;
  }
};
} // namespace ecow
