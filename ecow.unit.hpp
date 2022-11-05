#pragma once

#include "ecow.core.hpp"

#include <string>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;

protected:
  using strvec = std::vector<std::string>;

  [[nodiscard]] static auto pcm_name(const std::string &who) {
    return impl::current_target().build_folder() + who + ".pcm";
  }
  [[nodiscard]] static auto obj_name(const std::string &who) {
    return impl::current_target().build_folder() + who + ".o";
  }

public:
  explicit unit(std::string name) : m_name{name} {}

  [[nodiscard]] constexpr const auto &name() const noexcept { return m_name; }

  [[nodiscard]] virtual bool build() {
    const auto ext = impl::ext_of(m_name);
    if (ext != "") {
      return impl::run_clang("-c", m_name + ext, obj_name(m_name));
    } else {
      std::cerr << "Unit not found with '.cpp' or '.mm' extension: " << m_name
                << std::endl;
      return false;
    }
  }
  virtual void clean() { impl::remove(obj_name(m_name)); }

  [[nodiscard]] virtual strvec objects() const {
    strvec res{};
    res.push_back(name());
    return res;
  }
};
} // namespace ecow
