#pragma once
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningService.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningTool.h"
#include "llvm/Support/VirtualFileSystem.h"

#ifdef __APPLE__
#define ECOW_CLANG_FINDER "brew --prefix llvm@16"
#else
#define ECOW_CLANG_FINDER "which llvm@16"
#endif

namespace ecow::impl {
static auto find_clang_exe(const char *name) {
  return std::filesystem::path{impl::popen(ECOW_CLANG_FINDER)} / "bin" / name;
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
void ecow::impl::clang::generate_deps() {
  using namespace clang::tooling::dependencies;
  using namespace clang;

  if (deps::dependency_map.contains(m_from))
    return;

  std::string clang_exe = find_clang_exe(m_cpp ? "clang++" : "clang");

  auto to = (std::filesystem::current_path() / m_to).make_preferred().string();

  Twine dir{"."};
  Twine file{m_from};
  Twine output{to};

  std::vector<std::string> cmd_line{};
  cmd_line.push_back(clang_exe);
  for (const auto &r : m_args) {
    cmd_line.push_back(r);
  }
  cmd_line.push_back(m_from);
  cmd_line.push_back("-o");
  cmd_line.push_back(to);

  tooling::CompileCommand input{".", m_from, cmd_line, output};
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

  // if (rule->Provides) rule->Provides->ModuleName
  for (auto &req : rule->Requires) {
    // req.ModuleName
    deps::dependency_map[rule->PrimaryOutput].insert(req.SourcePath);
  }
}

void ecow::impl::clang::really_run() {
  using namespace clang::driver;
  using namespace clang;

  std::string clang_exe = find_clang_exe(m_cpp ? "clang++" : "clang");
  std::string triple = impl::current_target()->triple();
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
