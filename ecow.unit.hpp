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
struct command_failed : public std::runtime_error {
  using runtime_error::runtime_error;
};

class unit {
  std::string m_name;
  std::unordered_set<std::string> m_link_flags{};
  std::vector<std::shared_ptr<feat>> m_features{};
  std::unordered_set<std::string> m_include_dirs{};
  std::unordered_set<features> m_requirements{};
  std::unordered_set<std::string> m_resources{};
  wsdeps::map_t m_wsdeps;

  [[nodiscard]] auto clang() const {
    const auto ext =
        std::filesystem::path{m_name}.has_extension() ? "" : ".cpp";
    return impl::clang{m_name + ext, obj_name(m_name)}
        .add_arg("-c")
        .add_include_dirs(m_include_dirs)
        .with_deps();
  }

protected:
  using pathset = std::set<std::filesystem::path>;
  using strmap = std::map<std::string, std::string>;
  using strvec = std::vector<std::string>;
  using strset = std::unordered_set<std::string>;

  [[nodiscard]] static std::string pcm_name(const std::string &who) {
    return impl::current_target()->build_folder() + who + ".pcm";
  }
  [[nodiscard]] static std::string obj_name(const std::string &who) {
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

  virtual void build_self() const { clang().run(); }
  virtual void calculate_self_deps() {}

  virtual pathset self_objects() const {
    pathset res{};
    res.insert(obj_name(name()));
    return res;
  }

  [[nodiscard]] bool has_wsdep(const std::string &n) {
    return m_wsdeps.contains(n);
  }
  void merge_wsdep(const std::string &k, const pathset &objs,
                   pathset &res) const {
    for (const auto &obj : objs) {
      if (obj.is_absolute()) {
        res.insert(obj);
        continue;
      }

      const auto p = std::filesystem::current_path().parent_path();
      res.insert(p / k / obj);
    }
  }

  [[nodiscard]] bool current_target_supports_me() const noexcept {
    return std::all_of(
        m_requirements.begin(), m_requirements.end(),
        [](auto r) { return impl::current_target()->supports(r); });
  }

public:
  explicit unit(std::string name) : m_name{name} {}

  void add_framework(const std::string &name) {
    add_link_flag("-framework " + name);
  }

  void add_include_dir(std::string dir) { m_include_dirs.insert(dir); }
  void add_library_dir(std::string dir) { add_link_flag("-L" + dir); }
  void add_resource(std::string res) { m_resources.insert(res); }
  void add_requirement(features r) { m_requirements.insert(r); }

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
    if (!current_target_supports_me())
      return;

    std::for_each(m_wsdeps.begin(), m_wsdeps.end(), [f, &out](auto &kv) {
      wsdeps::curpath_raii c{kv.first};
      kv.second->visit(f, out);
    });
    std::for_each(m_features.begin(), m_features.end(),
                  [f, &out](auto &mf) { mf->visit(f, out); });
  }

  virtual void recurse_wsdeps(wsdeps::map_t &res) const {
    for (auto &[k, v] : m_wsdeps) {
      res[k] = v;
      v->recurse_wsdeps(res);
    }
  }

  void build() const {
    if (!current_target_supports_me())
      return;

    std::for_each(m_wsdeps.begin(), m_wsdeps.end(), [this](auto &v) {
      wsdeps::curpath_raii c{v.first};
      v.second->build();
    });

    wsdeps::map_t wsd{};
    recurse_wsdeps(wsd);

    wsdeps::target t{wsd};
    build_self();
  }

  void calculate_deps() {
    if (!current_target_supports_me())
      return;

    calculate_self_deps();
    for (const auto &[k, u] : m_wsdeps) {
      wsdeps::curpath_raii c{k};
      u->calculate_deps();
    }
  }

  [[nodiscard]] virtual strset link_flags() const {
    if (!current_target_supports_me())
      return {};

    strset res = m_link_flags;
    for (const auto &[k, u] : m_wsdeps) {
      wsdeps::curpath_raii c{k};
      const auto flags = u->link_flags();
      std::copy(flags.begin(), flags.end(), std::inserter(res, res.end()));
    }
    return res;
  }

  [[nodiscard]] virtual pathset resources() const {
    if (!current_target_supports_me())
      return {};

    pathset res{};
    for (const auto &r : m_resources) {
      res.insert(r);
    }
    for (const auto &[k, u] : m_wsdeps) {
      wsdeps::curpath_raii c{k};
      const auto r = u->resources();
      merge_wsdep(k, r, res);
    }
    return res;
  }

  [[nodiscard]] pathset objects() const {
    if (!current_target_supports_me())
      return {};

    pathset res = self_objects();
    for (const auto &[k, u] : m_wsdeps) {
      wsdeps::curpath_raii c{k};
      const auto objs = u->objects();
      merge_wsdep(k, objs, res);
    }
    return res;
  }

  template <typename Tp>
  static std::shared_ptr<Tp> create(const std::string &name) {
    return std::make_shared<Tp>(name);
  }
};
} // namespace ecow
