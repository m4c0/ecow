#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class exe : public seq {
  [[nodiscard]] auto final_exe_name() const {
    return impl::current_target()->build_folder() + exe_name();
  }

protected:
  [[nodiscard]] virtual std::string exe_name() const { return name(); }

public:
  using seq::seq;

  virtual void build() override {
    using namespace std::string_literals;

    seq::build();

    const auto exe_nm = final_exe_name();
    const auto exe_time = impl::last_write_time(exe_nm);

    bool any_is_newer = false;
    std::string cmd = impl::ld() + " -o " + exe_nm;
    for (const auto &o : objects()) {
      const auto obj = obj_name(o);
      const auto otime = impl::last_write_time(obj);
      if (otime > exe_time)
        any_is_newer = true;

      cmd.append(" "s + obj);
    }

    if (!any_is_newer)
      return;

    for (const auto &f : link_flags()) {
      cmd.append(" ");
      cmd.append(f);
    }

    std::cerr << "linking " << exe_nm << std::endl;
    if (std::system(cmd.c_str()) != 0)
      throw impl::clang_failed{cmd};
  }
};
} // namespace ecow
