#pragma once

#include "ecow.core.hpp"
#include "ecow.feat.hpp"

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;
  std::unordered_set<std::string> m_link_flags{};
  std::vector<std::shared_ptr<feat>> m_features{};

protected:
  using strmap = std::map<std::string, std::string>;
  using strvec = std::vector<std::string>;
  using strset = std::unordered_set<std::string>;

  [[nodiscard]] static auto pcm_name(const std::string &who) {
    return impl::current_target()->build_folder() + who + ".pcm";
  }
  [[nodiscard]] static auto obj_name(const std::string &who) {
    return impl::current_target()->build_folder() + who + ".o";
  }

  [[nodiscard]] constexpr const auto &name() const noexcept { return m_name; }
  [[nodiscard]] bool target_supports(features f) const noexcept {
    return impl::current_target()->supports(f);
  }

  void add_link_flag(const std::string &name) { m_link_flags.insert(name); }

public:
  explicit unit(std::string name) : m_name{name} {}

  void add_system_library(const std::string &name) {
    add_link_flag("-l" + name);
  }
  template <typename FTp> auto add_feat() {
    auto f = std::make_shared<FTp>();
    m_features.push_back(f);
    return f;
  }
  virtual void visit(features f, strmap &out) const {
    for_each(m_features.begin(), m_features.end(),
             [f, &out](auto &mf) { mf->visit(f, out); });
  }

  virtual void build() {
    const auto ext =
        std::filesystem::path{m_name}.has_extension() ? "" : ".cpp";
    impl::clang{m_name + ext, obj_name(m_name)}.add_arg("-c").with_deps().run();
  }

  [[nodiscard]] virtual strset link_flags() const { return m_link_flags; }

  [[nodiscard]] virtual strvec objects() const {
    strvec res{};
    res.push_back(name());
    return res;
  }
};
} // namespace ecow
