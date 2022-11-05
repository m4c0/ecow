#pragma once

#include "ecow.core.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;
  std::unordered_set<std::string> m_frameworks{};

protected:
  using strvec = std::vector<std::string>;
  using strset = std::unordered_set<std::string>;

  [[nodiscard]] static auto pcm_name(const std::string &who) {
    return impl::current_target().build_folder() + who + ".pcm";
  }
  [[nodiscard]] static auto obj_name(const std::string &who) {
    return impl::current_target().build_folder() + who + ".o";
  }

  [[nodiscard]] constexpr const auto &name() const noexcept { return m_name; }

public:
  explicit unit(std::string name) : m_name{name} {}

  [[nodiscard]] virtual bool build(const std::string &flags = "") {
    const auto ext =
        std::filesystem::path{m_name}.has_extension() ? "" : ".cpp";
    return impl::run_clang(flags + " -c", m_name + ext, obj_name(m_name));
  }
  virtual void clean() { impl::remove(obj_name(m_name)); }

  void add_framework(const std::string &name) {
#ifdef __APPLE__
    m_frameworks.insert(name);
#endif
  }

  [[nodiscard]] virtual strset frameworks() const { return m_frameworks; }

  [[nodiscard]] virtual strvec objects() const {
    strvec res{};
    res.push_back(name());
    return res;
  }
};
} // namespace ecow
