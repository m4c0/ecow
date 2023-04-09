#pragma once

#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace ecow {
enum features {
  android_ndk,
  application,
  cocoa,
  host,
  native,
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

protected:
  [[nodiscard]] const std::string &name() const noexcept { return m_name; }
  [[nodiscard]] virtual std::string value() const noexcept = 0;
  [[nodiscard]] features type() const noexcept override { return F; }
  void visit(strmap &out) const noexcept override { out[m_name] = value(); }

  t_feat_pair(std::string name) : m_name{name} {}
};

template <features F> class file_feat : public t_feat_pair<F> {
  std::string m_value;

protected:
  file_feat(std::string name, std::string value)
      : t_feat_pair<F>{name}, m_value{value} {}

  [[nodiscard]] std::string value() const noexcept override {
    std::ifstream ifs{this->name() + ".js"};
    if (ifs) {
      std::ostringstream i{};
      i << ifs.rdbuf();
      return i.str();
    } else {
      return m_value;
    }
  }
};

struct inline_js : file_feat<wasm_env> {
  inline_js(std::string name, std::string value) : file_feat{name, value} {}
};
struct setup_js : file_feat<wasm_setup> {
  explicit setup_js(std::string fn) : file_feat{fn, fn} {}
};
} // namespace ecow
