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

  [[nodiscard]] virtual bool build(const std::string &flags = "") override {
    return std::all_of(m_units.begin(), m_units.end(),
                       [flags](const auto &u) { return u->build(flags); });
  }

  [[nodiscard]] virtual strset link_flags() const override {
    strset res{unit::link_flags()};
    for (const auto &u : m_units) {
      const auto fws = u->link_flags();
      std::copy(fws.begin(), fws.end(), std::inserter(res, res.end()));
    }
    return res;
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
