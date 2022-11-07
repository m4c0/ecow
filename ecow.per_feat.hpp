#pragma once

#include "ecow.unit.hpp"

#include <unordered_map>

namespace ecow {
template <typename UTp> class per_feat : public unit {
  std::unordered_map<feats, UTp> m_map;

public:
  using unit::unit;

  [[nodiscard]] UTp &for_feature(feats f) {
    return (*m_map.try_emplace(f, name()).first).second;
  }

  [[nodiscard]] bool build(const std::string &flags = "") override {
    for (auto &[f, u] : m_map) {
      if (target_supports(f) && !u.build(flags))
        return false;
    }
    return true;
  }
  [[nodiscard]] strset frameworks() const override {
    strset res{};
    for (auto &[f, u] : m_map) {
      if (!target_supports(f))
        continue;

      auto fw = u.frameworks();
      std::copy(fw.begin(), fw.end(), std::inserter(res, res.end()));
    }
    return res;
  }
  [[nodiscard]] strvec objects() const override {
    strvec res{};
    for (auto &[f, u] : m_map) {
      if (!target_supports(f))
        continue;

      auto fw = u.objects();
      std::copy(fw.begin(), fw.end(), std::back_inserter(res));
    }
    return res;
  }
};
} // namespace ecow
