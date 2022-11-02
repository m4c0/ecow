#pragma once

#include "ecow.unit.hpp"

#include <algorithm>
#include <memory>

namespace ecow {
class seq : public unit {
  std::vector<std::shared_ptr<unit>> m_units;

public:
  using unit::unit;

  template <typename Tp = unit> auto add_unit(const std::string &name) {
    auto res = std::make_shared<Tp>(name);
    m_units.push_back(res);
    return res;
  }
  void add_ref(std::shared_ptr<unit> ref) { m_units.push_back(ref); }

  [[nodiscard]] virtual bool build(args_t args) override {
    return std::all_of(m_units.begin(), m_units.end(),
                       [args](const auto &u) { return u->build(args); });
  }
  virtual void clean(args_t args) override {
    std::for_each(m_units.begin(), m_units.end(),
                  [args](const auto &u) { return u->clean(args); });
  }

  [[nodiscard]] virtual strvec objects() const override {
    strvec res{};
    for (const auto &u : m_units) {
      const auto objs = u->objects();
      std::copy(objs.begin(), objs.end(), std::back_inserter(res));
    }
    return res;
  }
};
} // namespace ecow
