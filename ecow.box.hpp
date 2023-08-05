#pragma once
#include "ecow.deps.hpp"
#include "ecow.mod.hpp"
#include "ecow.unit.hpp"

namespace ecow {
class box : public seq {
  std::set<std::string> m_cache{};

  void add_mod(const std::string &n) {
    auto m = create<mod>(n);
    m->calculate_deps();
    m_cache.insert(n);

    for (auto &d : deps::of(n)) {
      calculate_deps_of(d);
    }

    // add after its deps
    add_ref(m);
  }

  void add_dep(const std::string &n) {
    if (has_wsdep(n))
      return;
    add_wsdep(n, unit::create<box>(n));
    m_cache.insert(n);
  }

  void calculate_deps_of(const std::string &n) {
    if (m_cache.contains(n))
      return;

    if (std::filesystem::exists(n + ".cppm")) {
      add_mod(n);
      return;
    }
    auto cur_path = std::filesystem::current_path();
    auto wsd_path = cur_path.parent_path() / n;
    if (std::filesystem::exists(wsd_path / "build.cpp")) {
      add_dep(n);
    }
  }

  virtual void calculate_self_deps() override {
    if (m_cache.size() > 0)
      return;

    calculate_deps_of(name());
  }

public:
  using seq::seq;
};
} // namespace ecow
