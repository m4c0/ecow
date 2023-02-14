#pragma once

#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace ecow {
enum features {
  android_ndk,
  cocoa,
  host,
  posix,
  objective_c,
  uikit,
  windows_api,
  webassembly,
};

class feat {
  std::map<std::string, std::string> m_values{};

protected:
  [[nodiscard]] virtual features type() const noexcept = 0;

  void set(const std::string &k, const std::string &v) {
    m_values.emplace(k, v);
  }

  [[nodiscard]] constexpr const auto &values() const noexcept {
    return m_values;
  }

public:
  virtual ~feat() = default;

  void visit(features f, decltype(m_values) &out) const {
    if (f != type())
      return;

    std::copy(m_values.begin(), m_values.end(), std::inserter(out, out.end()));
  }
};
} // namespace ecow
