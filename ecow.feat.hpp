#pragma once

#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace ecow {
enum features {
  android_ndk,
  application,
  cocoa,
  host,
  posix,
  objective_c,
  uikit,
  windows_api,
  webassembly,
};

class feat {
protected:
  using strmap = std::map<std::string, std::string>;

  [[nodiscard]] virtual features type() const noexcept = 0;
  virtual void visit(strmap &out) const noexcept = 0;

public:
  virtual ~feat() = default;

  void visit(features f, strmap &out) const {
    if (f == type())
      visit(out);
  }
};
} // namespace ecow
