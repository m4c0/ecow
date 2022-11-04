#pragma once

#include "ecow.exe.hpp"

namespace ecow {
class app : public exe {
public:
  explicit app(const std::string &name)
      : exe{impl::current_target().app_exe_name(name)} {}

  [[nodiscard]] bool build() override { return exe::build(); }
  void clean() override { exe::clean(); }
};
} // namespace ecow
