#pragma once

#include "ecow.unit.hpp"

#include <unordered_map>

namespace ecow {
template <typename UTp> class per_feat : public unit {
  std::unordered_map<features, UTp> m_map;

protected:
  void build_self() const override {
    for (auto &[f, u] : m_map) {
      if (target_supports(f))
        u.build();
    }
  }
  void create_self_cdb(std::ostream &o) const override {
    for (auto &[f, u] : m_map) {
      if (target_supports(f))
        u.create_cdb(o);
    }
  }
  [[nodiscard]] pathset self_objects() const override {
    pathset res{};
    for (auto &[f, u] : m_map) {
      if (!target_supports(f))
        continue;

      auto fw = u.objects();
      std::copy(fw.begin(), fw.end(), std::inserter(res, res.end()));
    }
    return res;
  }

public:
  using unit::unit;

  [[nodiscard]] UTp &for_feature(features f) {
    return (*m_map.try_emplace(f, name()).first).second;
  }
  void visit(features vf, strmap &out) const override {
    unit::visit(vf, out);
    for (auto &[f, u] : m_map) {
      if (target_supports(f) && target_supports(vf))
        u.visit(vf, out);
    }
  }

  [[nodiscard]] strset link_flags() const override {
    strset res{};
    for (auto &[f, u] : m_map) {
      if (!target_supports(f))
        continue;

      auto fw = u.link_flags();
      std::copy(fw.begin(), fw.end(), std::inserter(res, res.end()));
    }
    return res;
  }

  [[nodiscard]] pathset resources() const override {
    pathset res{};
    for (auto &[f, u] : m_map) {
      if (!target_supports(f))
        continue;

      auto fw = u.resources();
      std::copy(fw.begin(), fw.end(), std::inserter(res, res.end()));
    }
    return res;
  }
};
} // namespace ecow
