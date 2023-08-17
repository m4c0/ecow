#ifdef _WIN32
#define off_t _off_t
#endif

#include "ecow.clang.hpp"

#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningService.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningTool.h"
#include "llvm/Support/VirtualFileSystem.h"

namespace ecow::impl {
static auto find_clang_exe(const char *name) {
  return (clang_dir() / "bin" / name).string();
}
} // namespace ecow::impl

auto &dep_scan_srv() {
  using namespace clang::tooling::dependencies;
  using namespace clang;

  static constexpr const auto scan_mode =
      ScanningMode::DependencyDirectivesScan;
  static constexpr const auto format = ScanningOutputFormat::P1689;
  static constexpr const auto opt_args = false;
  static constexpr const auto eager_load_mod = false;

  static auto service =
      DependencyScanningService{scan_mode, format, opt_args, eager_load_mod};
  return service;
}
std::set<std::string> ecow::impl::clang::generate_deps() {
  using namespace clang::tooling::dependencies;
  using namespace clang;

  if (m_with_deps && !must_recompile()) {
    std::ifstream deps{depfile()};

    auto self = std::filesystem::path{m_to}.stem().string();

    std::set<std::string> res{};
    std::string line;
    deps >> line; // output:
    while (deps) {
      deps >> line;
      if (line == "\\")
        continue;

      auto path = std::filesystem::path{line};
      if (path.extension() != ".pcm")
        continue;

      auto mod = path.stem().string();
      if (mod == self)
        continue;

      auto dash = std::find(mod.begin(), mod.end(), '-');
      if (dash != mod.end()) {
        *dash = ':';
      }
      res.insert(mod);
    }

    return res;
  }

  auto from =
      (std::filesystem::current_path() / m_from).make_preferred().string();
  auto to = (std::filesystem::current_path() / m_to).make_preferred().string();

  std::string clang_exe = find_clang_exe(m_cpp ? "clang++" : "clang");

  Twine dir{"."};
  Twine file{from};
  Twine output{to};

  std::vector<std::string> cmd_line{};
  cmd_line.push_back(clang_exe);
  for (const auto &r : m_args) {
    cmd_line.push_back(r);
  }
  cmd_line.push_back(from);
  cmd_line.push_back("-o");
  cmd_line.push_back(to);

  tooling::CompileCommand input{".", from, cmd_line, output};
  std::string cwd{"."};

  std::string mf_out{};
  std::string mf_out_path{};
  auto tool = DependencyScanningTool{dep_scan_srv()};
  auto rule =
      tool.getP1689ModuleDependencyFile(input, cwd, mf_out, mf_out_path);
  if (!rule) {
    llvm::handleAllErrors(rule.takeError(), [&](llvm::StringError &err) {
      std::cerr << err.getMessage();
    });
    throw clang_failed{full_cmd()};
  }

  std::set<std::string> res{};
  for (auto &req : rule->Requires) {
    res.insert(req.ModuleName);
  }
  return res;
}

void ecow::impl::clang::really_run(const std::string &triple) {
  using namespace clang::driver;
  using namespace clang;

  std::string clang_exe = find_clang_exe(m_cpp ? "clang++" : "clang");
  std::string title = "ecow clang driver";

  auto diag_opts =
      IntrusiveRefCntPtr<DiagnosticOptions>{new DiagnosticOptions()};
  auto diag_ids = IntrusiveRefCntPtr<DiagnosticIDs>{new DiagnosticIDs()};
  auto diag_cli = new TextDiagnosticPrinter(llvm::errs(), &*diag_opts);

  DiagnosticsEngine diags{diag_ids, diag_opts, diag_cli};

  Driver driver{clang_exe, triple, diags, title};

  std::string to = (std::filesystem::current_path() / m_to).string();

  // Not sure why ref'ng directly from std::set fails
  std::vector<llvm::StringRef> args0{};
  args0.push_back(clang_exe);
  for (const auto &r : m_args) {
    args0.push_back(r);
  }
  args0.push_back(m_from);
  args0.push_back("-o");
  args0.push_back(to);

  std::vector<const char *> args{};
  for (auto str : args0) {
    args.push_back(str.begin());
  }
  auto c = driver.BuildCompilation(args);

  if (c->containsError())
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    throw clang_failed{full_cmd()};

  llvm::SmallVector<std::pair<int, const Command *>, 4> fail_cmds{};
  if (driver.ExecuteCompilation(*c, fail_cmds) != 0)
    throw clang_failed{full_cmd()};

  for (const auto &p : fail_cmds) {
    if (p.first != 0)
      throw clang_failed{full_cmd()};
  }
}
