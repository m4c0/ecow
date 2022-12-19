#pragma once

#include "ecow.unit.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>

namespace ecow {
class seq : public unit {
  using wsdeps_t = std::map<std::string, std::shared_ptr<unit>>;

  std::vector<std::shared_ptr<unit>> m_units;
  wsdeps_t m_wsdeps;

  void build_wsdep(const std::string &k, const std::shared_ptr<unit> &u) {
    using namespace std::filesystem;

    class curpath_raii {
      path o{current_path()};

    public:
      curpath_raii(const std::string &n) {
        auto p = o.parent_path() / n;
        if (!exists(p))
          throw std::runtime_error("Project not in workspace: " + n);

        create_directories(p / impl::current_target()->build_folder());
        current_path(p);
      }
      ~curpath_raii() { current_path(o); }
    };

    curpath_raii c{k};
    u->build();
  }

protected:
  virtual void build_self() {
    std::for_each(m_units.begin(), m_units.end(), std::mem_fn(&unit::build));
  }

public:
  using unit::unit;

  template <typename Tp = unit> auto add_unit(const std::string &name) {
    auto res = create<Tp>(name);
    m_units.push_back(res);
    return res;
  }
  void add_ref(std::shared_ptr<unit> ref) { m_units.push_back(ref); }

  void add_wsdep(std::string name, std::shared_ptr<unit> ref) {
    m_wsdeps[name] = ref;
  }

  virtual void visit(features f, strmap &out) const override {
    unit::visit(f, out);
    std::for_each(m_units.begin(), m_units.end(),
                  [f, &out](auto &u) { u->visit(f, out); });
    std::for_each(m_wsdeps.begin(), m_wsdeps.end(),
                  [f, &out](auto &kv) { kv.second->visit(f, out); });
  }

  virtual void build() override {
    std::for_each(m_wsdeps.begin(), m_wsdeps.end(),
                  [this](const auto &d) { build_wsdep(d.first, d.second); });

    class wsdep_target : public impl::deco_target {
      const wsdeps_t &m;

    public:
      explicit wsdep_target(const wsdeps_t &m) : m{m} {}

      [[nodiscard]] virtual std::set<std::string>
      prebuilt_module_paths() const override {
        auto res = deco_target::prebuilt_module_paths();
        for (const auto &[k, v] : m)
          res.insert("../" + k + "/" + build_folder());
        return res;
      }
    };
    wsdep_target t{m_wsdeps};

    build_self();
  }

  [[nodiscard]] virtual strset link_flags() const override {
    strset res{unit::link_flags()};
    for (const auto &u : m_units) {
      const auto fws = u->link_flags();
      std::copy(fws.begin(), fws.end(), std::inserter(res, res.end()));
    }
    for (const auto &[k, u] : m_wsdeps) {
      const auto fws = u->link_flags();
      std::copy(fws.begin(), fws.end(), std::inserter(res, res.end()));
    }
    return res;
  }

  [[nodiscard]] virtual pathset objects() const override {
    pathset res{};
    for (const auto &u : m_units) {
      const auto objs = u->objects();
      std::copy(objs.begin(), objs.end(), std::inserter(res, res.end()));
    }
    for (const auto &[k, u] : m_wsdeps) {
      const auto objs = u->objects();
      for (const auto &obj : objs) {
        if (obj.is_absolute()) {
          res.insert(obj);
          continue;
        }

        const auto p = std::filesystem::current_path().parent_path();
        res.insert(p / k / obj);
      }
    }
    return res;
  }
};
} // namespace ecow
