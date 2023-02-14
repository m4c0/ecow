#pragma once
#include "ecow.exe.hpp"

namespace ecow {
class tool : public exe {
protected:
  virtual void build_self() const override {
    if (impl::current_target()->supports(host))
      exe::build_self();
  }

public:
  using exe::exe;

  [[nodiscard]] auto executable() const {
    return impl::host_target().build_path() / exe_name();
  }
};
} // namespace ecow
