#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define off_t _off_t
#endif

#include "ecow.llvm.hpp"

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/CodeGen/ObjectFilePCHContainerOperations.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningService.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningTool.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/TargetParser/Host.h"

#include <filesystem>
#include <fstream>

using namespace clang::tooling::dependencies;
using namespace clang::driver;
using namespace clang;

static auto &dep_scan_srv() {
  static constexpr const auto scan_mode =
      ScanningMode::DependencyDirectivesScan;
  static constexpr const auto format = ScanningOutputFormat::P1689;
  static constexpr const auto opt_args = false;
  static constexpr const auto eager_load_mod = false;

  static auto service =
      DependencyScanningService{scan_mode, format, opt_args, eager_load_mod};
  return service;
}
ecow::llvm::deps ecow::llvm::find_deps(const input &in,
                                       const std::string &depfile,
                                       bool must_recompile) {
  ecow::llvm::deps res{{}, true};

  if (depfile != "" && !must_recompile) {
    std::ifstream deps{depfile};

    auto self = std::filesystem::path{in.to}.stem().string();

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
      res.deps.insert(mod);
    }

    return res;
  }

  auto from =
      (std::filesystem::current_path() / in.from).make_preferred().string();
  auto to = (std::filesystem::current_path() / in.to).make_preferred().string();

  Twine dir{"."};
  Twine file{from};
  Twine output{to};

  tooling::CompileCommand input{".", from, in.cmd_line, output};
  std::string cwd{"."};

  std::string mf_out{};
  std::string mf_out_path{};
  auto tool = DependencyScanningTool{dep_scan_srv()};
  auto rule =
      tool.getP1689ModuleDependencyFile(input, cwd, mf_out, mf_out_path);
  if (!rule) {
    ::llvm::handleAllErrors(rule.takeError(), [&](::llvm::StringError &err) {
      ::llvm::errs() << err.getMessage();
    });
    return {{}, false};
  }

  for (auto &req : rule->Requires) {
    res.deps.insert(req.ModuleName);
  }
  return res;
}

using strvec = std::vector<std::string>;

class ecow_idlist_pragma_handler : public PragmaHandler {
  CompilerInstance *m_ci;
  std::unique_ptr<raw_pwrite_stream> m_out{};

  void output_item(const Token &t) {
    if (!m_out) {
      SmallString<128> path(m_ci->getFrontendOpts().OutputFile);
      ::llvm::sys::path::replace_extension(path, extension());

      m_out = m_ci->createOutputFile(path, false, false, false);
    }

    if (m_out)
      (*m_out) << translate_item(t.getIdentifierInfo()->getName()) << "\n";
  }

protected:
  virtual std::string translate_item(const Twine &t) = 0;
  virtual std::string extension() = 0;

public:
  ecow_idlist_pragma_handler(const char *name, CompilerInstance *ci)
      : PragmaHandler(name), m_ci{ci} {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    Token Tok;
    do {
      PP.LexUnexpandedToken(Tok);
      if (Tok.getKind() == tok::eod) {
        return;
      }
      if (!Tok.isAnyIdentifier()) {
        PP.Diag(Tok, diag::err_pp_identifier_arg_not_identifier)
            << Tok.getKind();
        return;
      }
      output_item(Tok);
    } while (true);
  }
};

class fw_pragma_handler : public ecow_idlist_pragma_handler {
public:
  fw_pragma_handler(CompilerInstance *ci)
      : ecow_idlist_pragma_handler("add_framework", ci) {}

  std::string extension() override { return "flags"; }
  std::string translate_item(const Twine &t) override {
    return ("-framework " + t).str();
  }
};

class syslib_pragma_handler : public ecow_idlist_pragma_handler {
public:
  syslib_pragma_handler(CompilerInstance *ci)
      : ecow_idlist_pragma_handler("add_system_library", ci) {}

  std::string extension() override { return "flags"; }
  std::string translate_item(const Twine &t) override {
    return ("-l" + t).str();
  }
};

class impls_pragma_handler : public ecow_idlist_pragma_handler {
public:
  impls_pragma_handler(CompilerInstance *ci)
      : ecow_idlist_pragma_handler("add_impl", ci) {}

  std::string extension() override { return "impls"; }
  std::string translate_item(const Twine &t) override { return t.str(); }
};

struct ecow_action : WrapperFrontendAction {
  ecow_action()
      : WrapperFrontendAction{
            std::make_unique<GenerateModuleInterfaceAction>()} {}

  bool BeginSourceFileAction(CompilerInstance &ci) override {
    auto &pp = ci.getPreprocessor();
    pp.AddPragmaHandler("ecow", new fw_pragma_handler(&ci));
    pp.AddPragmaHandler("ecow", new impls_pragma_handler(&ci));
    pp.AddPragmaHandler("ecow", new syslib_pragma_handler(&ci));

    return WrapperFrontendAction::BeginSourceFileAction(ci);
  }
};

static const ecow::llvm::input *llvm16_hack;
int cc1(SmallVectorImpl<const char *> &args) {
  auto in = llvm16_hack;

  auto cinst = std::make_unique<CompilerInstance>();

  auto pch_ops = cinst->getPCHContainerOperations();
  pch_ops->registerWriter(std::make_unique<ObjectFilePCHContainerWriter>());
  pch_ops->registerReader(std::make_unique<ObjectFilePCHContainerReader>());

  auto diag_opts =
      IntrusiveRefCntPtr<DiagnosticOptions>{new DiagnosticOptions()};
  auto diag_ids = new DiagnosticIDs();
  auto diag_cli = new TextDiagnosticBuffer;
  DiagnosticsEngine diags{diag_ids, &*diag_opts, diag_cli};

  auto argv = ::llvm::ArrayRef(args).slice(1);
  CompilerInvocation::CreateFromArgs(cinst->getInvocation(), argv, diags,
                                     args[0]);
  cinst->createDiagnostics();

  diag_cli->FlushDiagnostics(cinst->getDiagnostics());

  auto ext = std::filesystem::path{in->to}.extension();
  std::unique_ptr<FrontendAction> a{};
  if (ext == ".pcm") {
    a.reset(new ecow_action{});
  } else if (ext == ".o") {
    a.reset(new EmitObjAction{});
  } else {
    return 1;
  }

  auto res = cinst->ExecuteAction(*a);
  return !res;
}

bool ecow::llvm::compile(const input &in) {
  ::llvm::InitializeAllTargets();
  ::llvm::InitializeAllTargetMCs();
  ::llvm::InitializeAllAsmPrinters();
  ::llvm::InitializeAllAsmParsers();

  IntrusiveRefCntPtr<DiagnosticOptions> diag_opts{new DiagnosticOptions()};
  IntrusiveRefCntPtr<DiagnosticIDs> diag_ids{new DiagnosticIDs()};
  auto diag_cli = new TextDiagnosticPrinter(::llvm::errs(), &*diag_opts);

  DiagnosticsEngine diags{diag_ids, diag_opts, diag_cli};

  llvm16_hack = &in;

  Driver driver{in.clang_exe, ::llvm::sys::getDefaultTargetTriple(), diags};
  driver.setInstalledDir(
      std::filesystem::path{in.clang_exe}.parent_path().parent_path().string());
  driver.CC1Main = cc1;

  ::llvm::CrashRecoveryContext::Enable();

  std::vector<const char *> args{};
  for (auto &str : in.cmd_line) {
    args.push_back(str.data());
  }
  std::unique_ptr<Compilation> c{driver.BuildCompilation(args)};
  if (!c || c->containsError())
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return false;

  ::llvm::SmallVector<std::pair<int, const ::clang::driver::Command *>, 4>
      failures;
  int res = driver.ExecuteCompilation(*c, failures);
  for (const auto &p : failures) {
    if (p.first)
      return false;
  }

  diags.getClient()->finish();
  return res == 0;
}
