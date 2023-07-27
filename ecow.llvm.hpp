#pragma once
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
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
