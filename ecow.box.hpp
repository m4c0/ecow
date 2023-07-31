#pragma once
#include "ecow.deps.hpp"
#include "ecow.mod.hpp"
#include "ecow.unit.hpp"

namespace ecow {
class box : public unit {
  std::vector<std::shared_ptr<mod>> m_mods{};

  [[nodiscard]] static bool has_already(decltype(m_mods) &res,
                                        const std::string &d) {
    for (const auto &m : res) {
      if (m->module_name() == d)
        return true;
    }
    return false;
  }
  void rec_deps(decltype(m_mods) &res, std::shared_ptr<mod> m) const {
    res.push_back(m);
    m->generate_deps();

    auto mn = m->module_name();

    for (auto &d : deps::dependency_map[mn]) {
      if (!std::filesystem::exists(d + ".cppm"))
        continue;
      if (has_already(res, d))
        continue;
      rec_deps(res, unit::create<mod>(d));
    }
  }
  [[nodiscard]] auto auto_mods() const {
    decltype(m_mods) res{};
    for (auto m : m_mods) {
      rec_deps(res, m);
    }
    return res;
  }

  void build_deps_of(const std::string &mm) const {
    for (auto &d : deps::dependency_map[mm]) {
      if (d.starts_with(mm + ":")) {
        build_deps_of(d);
      } else {
        for (auto &m : auto_mods()) {
          if (m->module_name() == d) {
            build_after_deps(m);
          }
        }
      }
    }
  }
  void build_after_deps(std::shared_ptr<mod> mm) const {
    build_deps_of(mm->module_name());
    mm->build();
  }

  virtual void build_self() const override {
    generate_self_deps();
    for (auto &m : auto_mods()) {
      build_after_deps(m);
    }
  }
  virtual void generate_self_deps() const override {
    for (const auto &u : m_mods) {
      u->generate_deps();
    }
    for (const auto &u : auto_mods()) {
      u->generate_deps();
    }
  }
  [[nodiscard]] virtual pathset self_objects() const override {
    pathset res{};
    for (const auto &u : auto_mods()) {
      const auto objs = u->objects();
      std::copy(objs.begin(), objs.end(), std::inserter(res, res.end()));
    }
    return res;
  }

  virtual void visit(features f, strmap &out) const override {
    unit::visit(f, out);
    for (const auto &u : auto_mods()) {
      u->visit(f, out);
    }
  }

  virtual void recurse_wsdeps(wsdeps::map_t &res) const override {
    unit::recurse_wsdeps(res);
    for (const auto &u : auto_mods()) {
      u->recurse_wsdeps(res);
    }
  }

  [[nodiscard]] virtual strset link_flags() const override {
    strset res{unit::link_flags()};
    for (const auto &u : auto_mods()) {
      const auto fws = u->link_flags();
      std::copy(fws.begin(), fws.end(), std::inserter(res, res.end()));
    }
    return res;
  }

  [[nodiscard]] virtual pathset resources() const override {
    pathset res{unit::resources()};
    for (const auto &u : auto_mods()) {
      const auto fws = u->resources();
      std::copy(fws.begin(), fws.end(), std::inserter(res, res.end()));
    }
    return res;
  }

public:
  box(const std::string &name) : unit(name) {
    m_mods.push_back(unit::create<mod>(name));
  }

  auto add_mod(const char *name) {
    auto res = create<mod>(name);
    m_mods.push_back(res);
    return res;
  }
};
} // namespace ecow
