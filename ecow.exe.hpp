#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class exe : public seq {
  [[nodiscard]] auto final_exe_name() const {
    return impl::current_target()->build_folder() + exe_name();
  }

protected:
  [[nodiscard]] virtual std::string exe_name() const { return name(); }

  virtual void build_self() const override {
    using namespace std::string_literals;

    seq::build_self();

    const auto exe_nm = final_exe_name();
    const auto exe_time = impl::last_write_time(exe_nm);
    const auto ldflags = impl::current_target()->ldflags();

    bool any_is_newer = false;
    std::string cmd = impl::cxx() + ldflags + " -o " + exe_nm;
    for (const auto &obj : objects()) {
      const auto otime = impl::last_write_time(obj);
      if (otime > exe_time)
        any_is_newer = true;

      cmd.append(" "s + obj.string());
    }

    if (!any_is_newer)
      return;

    if (std::getenv("ECOW_DEBUG")) {
      cmd.append(" -g");
    } else {
      cmd.append(" -O3");
    }

    for (const auto &f : link_flags()) {
      cmd.append(" ");
      cmd.append(f);
    }

    std::cerr << "linking " << exe_nm << std::endl;
    if (std::system(cmd.c_str()) != 0)
      throw impl::clang_failed{cmd};
  }

public:
  using seq::seq;
};
} // namespace ecow
