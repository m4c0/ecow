#pragma once

#include "ecow.clang.hpp"
#include "ecow.core.hpp"
#include "ecow.feat.hpp"
#include "ecow.wsdep.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace ecow {
class unit {
  std::string m_name;
  std::unordered_set<std::string> m_link_flags{};
  std::vector<std::shared_ptr<feat>> m_features{};
  std::unordered_set<std::string> m_include_dirs{};
  wsdeps::map_t m_wsdeps;

protected:
  using pathset = std::set<std::filesystem::path>;
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
  [[nodiscard]] constexpr const auto &include_dirs() const noexcept {
    return m_include_dirs;
  }
  [[nodiscard]] bool target_supports(features f) const noexcept {
    return impl::current_target()->supports(f);
  }

  void add_link_flag(const std::string &name) { m_link_flags.insert(name); }

  virtual void build_self() const {
    const auto ext =
        std::filesystem::path{m_name}.has_extension() ? "" : ".cpp";
    impl::clang{m_name + ext, obj_name(m_name)}
        .add_arg("-c")
        .add_include_dirs(m_include_dirs)
        .with_deps()
        .run();
  }
  virtual pathset self_objects() const {
    pathset res{};
    res.insert(obj_name(name()));
    return res;
  }

public:
  explicit unit(std::string name) : m_name{name} {}

  void add_include_dir(std::string dir) { m_include_dirs.insert(dir); }

  void add_system_library(const std::string &name) {
    add_link_flag("-l" + name);
  }

  void add_wsdep(std::string name, std::shared_ptr<unit> ref) {
    m_wsdeps[name] = ref;
  }

  template <typename FTp, typename... Args> auto add_feat(Args &&...args) {
    auto f = std::make_shared<FTp>(std::forward<Args>(args)...);
    m_features.push_back(f);
    return f;
  }
  virtual void visit(features f, strmap &out) const {
    std::for_each(m_wsdeps.begin(), m_wsdeps.end(),
                  [f, &out](auto &kv) { kv.second->visit(f, out); });
    std::for_each(m_features.begin(), m_features.end(),
                  [f, &out](auto &mf) { mf->visit(f, out); });
  }

  void build() const {
    std::for_each(m_wsdeps.begin(), m_wsdeps.end(), [this](auto &v) {
      wsdeps::curpath_raii c{v.first};
      v.second->build();
    });

    wsdeps::target t{m_wsdeps};

    build_self();
  }

  [[nodiscard]] virtual strset link_flags() const {
    strset res = m_link_flags;
    for (const auto &[k, u] : m_wsdeps) {
      const auto flags = u->link_flags();
      std::copy(flags.begin(), flags.end(), std::inserter(res, res.end()));
    }
    return res;
  }

  [[nodiscard]] pathset objects() const {
    pathset res = self_objects();
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

  template <typename Tp>
  static std::shared_ptr<Tp> create(const std::string &name) {
    return std::make_shared<Tp>(name);
  }
};
} // namespace ecow
