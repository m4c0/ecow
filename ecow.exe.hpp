#pragma once

#include "ecow.seq.hpp"

namespace ecow {
class exe : public seq {
public:
  using seq::seq;

  [[nodiscard]] bool build(args_t args) override {
    if (!seq::build(args))
      return false;

    const auto exe_time = impl::last_write_time(name());

    bool any_is_newer = false;
    std::string cmd = impl::ld() + " -o " + name();
    for (const auto &o : objects()) {
      const auto obj = o + ".o";
      const auto otime = impl::last_write_time(obj);
      if (otime > exe_time)
        any_is_newer = true;

      using namespace std::string_literals;
      cmd.append(" "s + obj);
    }

    if (!any_is_newer)
      return true;

    std::cerr << "linking " << name() << std::endl;
    return std::system(cmd.c_str()) == 0;
  }
  void clean(args_t args) override {
    seq::clean(args);
    impl::remove(name() + ".exe");
  }
};
} // namespace ecow
