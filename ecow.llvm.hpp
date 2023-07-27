#pragma once
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/Support/VirtualFileSystem.h"

void ecow::impl::clang::really_run() {
  using namespace clang::driver;
  using namespace clang;

  llvm::StringRef clang_exe = m_compiler;
  std::string triple = impl::current_target()->triple();
  std::string title = "ecow clang driver";

  auto diag_opts =
      IntrusiveRefCntPtr<DiagnosticOptions>{new DiagnosticOptions()};
  auto diag_ids = IntrusiveRefCntPtr<DiagnosticIDs>{new DiagnosticIDs()};
  auto diag_cli = new TextDiagnosticPrinter(llvm::errs(), &*diag_opts);

  DiagnosticsEngine diags{diag_ids, diag_opts, diag_cli};

  Driver driver{clang_exe, triple, diags, title};

  // Not sure why ref'ng directly from std::set fails
  llvm::SmallVector<llvm::StringRef> args0{m_args.begin(), m_args.end()};
  args0.push_back(m_from);
  args0.push_back("-o");
  args0.push_back((std::filesystem::current_path() / m_to).string());

  std::vector<const char *> args{};
  for (auto str : args0) {
    args.push_back(str.begin());
  }
  for (auto str : args) {
    std::cerr << str << "\n";
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
