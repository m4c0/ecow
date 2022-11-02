#pragma once

#include "ecow.core.hpp"

#include <span>
#include <string>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;

protected:
  using args_t = std::span<char *>;
  using strvec = std::vector<std::string>;

public:
  explicit unit(std::string name) : m_name{name} {}

  [[nodiscard]] constexpr const auto &name() const noexcept { return m_name; }

  [[nodiscard]] virtual bool build(args_t args) {
    const auto ext = impl::ext_of(m_name);
    if (ext != "") {
      return impl::run_clang("-c", m_name + ext, m_name + ".o");
    } else {
      std::cerr << "Unit not found with '.cpp' or '.mm' extension: " << m_name
                << std::endl;
      return false;
    }
  }
  virtual void clean(args_t args) { impl::remove(m_name + ".o"); }

  [[nodiscard]] virtual strvec objects() const {
    strvec res{};
    res.push_back(name());
    return res;
  }

  [[nodiscard]] virtual int main(int argc, char **argv) {
    auto args = args_t{argv, static_cast<size_t>(argc)};
    switch (args.size()) {
    case 0:
      std::terminate();
    case 1:
      return build(args) ? 0 : 1;
    case 2:
      using namespace std::string_view_literals;
      if (args[1] == "clean"sv) {
        clean(args);
        return 0;
      }
    default:
      std::cerr << "I don't know how to do that" << std::endl;
      return 1;
    }
  }
};
} // namespace ecow
