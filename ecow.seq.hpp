#pragma once

#include "ecow.unit.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>

namespace ecow {
class seq : public unit {
  std::vector<std::shared_ptr<unit>> m_units;

protected:
  virtual void build_self() const override {
    std::for_each(m_units.begin(), m_units.end(), std::mem_fn(&unit::build));
  }
  [[nodiscard]] virtual pathset self_objects() const override {
    pathset res{};
    for (const auto &u : m_units) {
      const auto objs = u->objects();
      std::copy(objs.begin(), objs.end(), std::inserter(res, res.end()));
    }
    return res;
  }

public:
  using unit::unit;

  template <typename Tp = unit> auto add_unit(const std::string &name) {
    auto res = create<Tp>(name);
    m_units.push_back(res);
    return res;
  }
  void add_ref(std::shared_ptr<unit> ref) { m_units.push_back(ref); }

  virtual void visit(features f, strmap &out) const override {
    unit::visit(f, out);
    std::for_each(m_units.begin(), m_units.end(),
                  [f, &out](auto &u) { u->visit(f, out); });
  }

  [[nodiscard]] virtual strset link_flags() const override {
    strset res{unit::link_flags()};
    for (const auto &u : m_units) {
      const auto fws = u->link_flags();
      std::copy(fws.begin(), fws.end(), std::inserter(res, res.end()));
    }
    return res;
  }
};
} // namespace ecow
