#pragma once

#include "ecow.unit.hpp"

#include <unordered_map>

namespace ecow {
template <typename UTp> class per_feat : public unit {
  std::unordered_map<features, UTp> m_map;

public:
  using unit::unit;

  [[nodiscard]] UTp &for_feature(features f) {
    return (*m_map.try_emplace(f, name()).first).second;
  }
  void visit(features vf, strmap &out) const override {
    unit::visit(vf, out);
    for (auto &[f, u] : m_map) {
      if (target_supports(f))
        u.visit(vf, out);
    }
  }

  void build() override {
    for (auto &[f, u] : m_map) {
      if (target_supports(f))
        u.build();
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
  [[nodiscard]] pathset objects() const override {
    pathset res{};
    for (auto &[f, u] : m_map) {
      if (!target_supports(f))
        continue;

      auto fw = u.objects();
      std::copy(fw.begin(), fw.end(), std::inserter(res, res.end()));
    }
    return res;
  }
};
} // namespace ecow
