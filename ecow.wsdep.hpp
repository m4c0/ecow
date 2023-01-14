#pragma once

namespace ecow {
class unit;
}
namespace ecow::wsdeps {
using map_t = std::map<std::string, std::shared_ptr<unit>>;
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

class target : public impl::deco_target {
  const map_t &m_map;

public:
  explicit target(const map_t &m) : m_map{m} {}

  [[nodiscard]] virtual std::set<std::string>
  prebuilt_module_paths() const override {
    auto res = deco_target::prebuilt_module_paths();
    for (const auto &[k, v] : m_map)
      res.insert("../" + k + "/" + build_folder());
    return res;
  }
};
} // namespace ecow::wsdeps
