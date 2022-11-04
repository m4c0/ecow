#pragma once

#include "ecow.exe.hpp"

namespace ecow {
class app : public exe {
  [[nodiscard]] static inline auto exe_name(const std::string &name) {
#ifdef __APPLE__
    auto path = name + ".app/Contents/MacOS";
    std::filesystem::create_directories(path);
    return path + "/" + name;
#elif _WIN32
    return name + ".exe";
#else
    return name;
#endif
  }

public:
  explicit app(const std::string &name) : exe{exe_name(name)} {}

  [[nodiscard]] bool build() override { return exe::build(); }
  void clean() override { exe::clean(); }
};
} // namespace ecow
