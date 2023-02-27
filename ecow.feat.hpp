#pragma once

#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace ecow {
enum features {
  android_ndk,
  application,
  cocoa,
  export_syms,
  host,
  objective_c,
  posix,
  uikit,
  wasm_env,
  wasm_setup,
  webassembly,
  windows_api,
};

class feat {
protected:
  using strmap = std::map<std::string, std::string>;

  [[nodiscard]] virtual features type() const noexcept = 0;
  virtual void visit(strmap &out) const noexcept = 0;

public:
  virtual ~feat() = default;

  void visit(features f, strmap &out) const {
    if (f == type())
      visit(out);
  }
};

template <features F> class t_feat_pair : public feat {
  std::string m_name;
  std::string m_value;

protected:
  [[nodiscard]] features type() const noexcept override { return F; }
  void visit(strmap &out) const noexcept override { out[m_name] = m_value; }

  t_feat_pair(std::string name, std::string value)
      : m_name{name}, m_value{value} {}
};

struct export_symbol : t_feat_pair<export_syms> {
  export_symbol(std::string name) : t_feat_pair{name, ""} {}
};
struct inline_js : t_feat_pair<wasm_env> {
  inline_js(std::string name, std::string value) : t_feat_pair{name, value} {}
};
struct setup_js : t_feat_pair<wasm_setup> {
  setup_js(std::string fn) : t_feat_pair{fn, ""} {}
};
} // namespace ecow
