#pragma once

#include "ecow.feat.hpp"

namespace ecow {
class js : public feat {
  std::map<std::string, std::string> m_values{};

protected:
  [[nodiscard]] features type() const noexcept override { return webassembly; }

  void visit(strmap &out) const noexcept override {
    std::copy(m_values.begin(), m_values.end(), std::inserter(out, out.end()));
  }

public:
  void set(const std::string &k, const std::string &v) {
    m_values.emplace(k, v);
  }
};
} // namespace ecow
