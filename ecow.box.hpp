#pragma once
#include "ecow.deps.hpp"
#include "ecow.mod.hpp"
#include "ecow.unit.hpp"

namespace ecow {
class box : public unit {
  std::vector<std::shared_ptr<mod>> m_mods{};

  void build_after_deps(std::shared_ptr<mod> m) const {
    const auto &dps = deps::dependency_map[m->main_pcm_file().string()];
    for (auto &m : m_mods) {
      if (dps.contains(m->main_cpp_file().string())) {
        build_after_deps(m);
      }
    }
    m->build();
  }

  virtual void build_self() const override {
    for (auto &m : m_mods) {
      build_after_deps(m);
    }
  }
  virtual void create_self_cdb(std::ostream &o) const override {
    for (const auto &u : m_mods) {
      u->create_cdb(o);
    }
  }
  [[nodiscard]] virtual pathset self_objects() const override {
    pathset res{};
    for (const auto &u : m_mods) {
      const auto objs = u->objects();
      std::copy(objs.begin(), objs.end(), std::inserter(res, res.end()));
    }
    return res;
  }

  virtual void visit(features f, strmap &out) const override {
    unit::visit(f, out);
    std::for_each(m_mods.begin(), m_mods.end(),
                  [f, &out](auto &u) { u->visit(f, out); });
  }

  virtual void recurse_wsdeps(wsdeps::map_t &res) const override {
    unit::recurse_wsdeps(res);
    for (const auto &u : m_mods) {
      u->recurse_wsdeps(res);
    }
  }

  [[nodiscard]] virtual strset link_flags() const override {
    strset res{unit::link_flags()};
    for (const auto &u : m_mods) {
      const auto fws = u->link_flags();
      std::copy(fws.begin(), fws.end(), std::inserter(res, res.end()));
    }
    return res;
  }

  [[nodiscard]] virtual pathset resources() const override {
    pathset res{unit::resources()};
    for (const auto &u : m_mods) {
      const auto fws = u->resources();
      std::copy(fws.begin(), fws.end(), std::inserter(res, res.end()));
    }
    return res;
  }

public:
  using unit::unit;

  auto add_mod(const char *name) {
    auto res = create<mod>(name);
    m_mods.push_back(res);
    return res;
  }
};
} // namespace ecow
