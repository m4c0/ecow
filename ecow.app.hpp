#pragma once

#include "ecow.exe.hpp"

namespace ecow {
class app : public exe {
protected:
  [[nodiscard]] std::string exe_name() const override {
    return impl::current_target().app_exe_name(name());
  }

public:
  explicit app(const std::string &name) : exe{name} {}

  [[nodiscard]] bool build() override { return exe::build(); }
  void clean() override { exe::clean(); }
};
} // namespace ecow
