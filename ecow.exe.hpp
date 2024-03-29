#pragma once

#include "ecow.seq.hpp"

namespace ecow::impl {
std::filesystem::path clang_dir();
}

namespace ecow {
class exe : public seq {
protected:
  [[nodiscard]] virtual std::string final_exe_name() const {
    return impl::current_target()->build_folder() + exe_name();
  }

  [[nodiscard]] virtual std::string exe_name() const {
    return impl::current_target()->exe_name(name());
  }

  virtual void build_self() const override {
    using namespace std::string_literals;

    seq::build_self();

    const auto exe_nm = final_exe_name();
    const auto exe_time = impl::last_write_time(exe_nm);
    const auto ldflags = impl::current_target()->ldflags();
    const auto cxx = impl::clang_dir() / "bin" / "clang++";

    bool any_is_newer = false;
    std::string cmd = cxx.string() + " -o " + exe_nm;
    for (const auto &f : ldflags) {
      cmd.append(" ");
      cmd.append(f);
    }
    for (const auto &obj : objects()) {
      const auto otime = impl::last_write_time(obj);
      if (otime > exe_time)
        any_is_newer = true;

      cmd.append(" "s + obj.string());
    }

    if (!any_is_newer)
      return;

    auto ldf = std::getenv("ECOW_LDFLAGS");
    if (ldf != nullptr) {
      cmd.append(" ");
      cmd.append(ldf);
    }
    // TODO: use -gdwarf on windows
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
