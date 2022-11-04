#pragma once

#include "ecow.core.hpp"

#include <string>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;

protected:
  using strvec = std::vector<std::string>;

public:
  explicit unit(std::string name) : m_name{name} {}

  [[nodiscard]] constexpr const auto &name() const noexcept { return m_name; }

  [[nodiscard]] virtual bool build() {
    const auto ext = impl::ext_of(m_name);
    if (ext != "") {
      return impl::run_clang("-c", m_name + ext, m_name + ".o");
    } else {
      std::cerr << "Unit not found with '.cpp' or '.mm' extension: " << m_name
                << std::endl;
      return false;
    }
  }
  virtual void clean() { impl::remove(m_name + ".o"); }

  [[nodiscard]] virtual strvec objects() const {
    strvec res{};
    res.push_back(name());
    return res;
  }
};
} // namespace ecow
