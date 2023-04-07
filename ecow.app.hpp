#pragma once

#include "ecow.exe.hpp"

#include <fstream>
#include <numeric>

namespace ecow {
class app : public exe {
protected:
  [[nodiscard]] std::string exe_name() const override {
    return impl::current_target()->app_exe_name(name());
  }

  void build_self() const override {
    exe::build_self();
    impl::current_target()->bundle(name(), *this);

    const auto res_fld = impl::current_target()->resource_path(name());
    for (const auto &res : resources()) {
      impl::run_copy(res, res_fld / res.filename());
    }
  };

public:
  explicit app(const std::string &name) : exe{name} {}
};
} // namespace ecow
