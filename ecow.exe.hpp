#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class exe : public seq {
protected:
  [[nodiscard]] virtual std::string exe_name() const { return name(); }

public:
  using seq::seq;

  [[nodiscard]] virtual bool build(const std::string &flags = "") override {
    using namespace std::string_literals;

    if (!seq::build(flags))
      return false;

    const auto exe_time = impl::last_write_time(exe_name());

    bool any_is_newer = false;
    std::string cmd = impl::ld() + " -o " + exe_name();
    for (const auto &f : frameworks()) {
      cmd.append(" -framework "s + f);
    }
    for (const auto &o : objects()) {
      const auto obj = obj_name(o);
      const auto otime = impl::last_write_time(obj);
      if (otime > exe_time)
        any_is_newer = true;

      cmd.append(" "s + obj);
    }

    if (!any_is_newer)
      return true;

    std::cerr << "linking " << exe_name() << std::endl;
    return std::system(cmd.c_str()) == 0;
  }
  void clean() override {
    seq::clean();
    impl::remove(exe_name() + ".exe");
  }
};
} // namespace ecow
