#ifdef _WIN32
#define off_t _off_t
#endif

#include "ecow.llvm.hpp"

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningService.h"
#include "clang/Tooling/DependencyScanning/DependencyScanningTool.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/VirtualFileSystem.h"

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
protected:
  virtual void translate_item(const Twine &t) = 0;

public:
  ecow_idlist_pragma_handler(const char *name) : PragmaHandler(name) {}

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
      translate_item(Tok.getIdentifierInfo()->getName());
    } while (true);
  }
};

class syslib_pragma_handler : public ecow_idlist_pragma_handler {
  CompilerInstance *m_ci;
  std::unique_ptr<raw_pwrite_stream> m_out{};

public:
  syslib_pragma_handler(CompilerInstance *ci)
      : ecow_idlist_pragma_handler("add_system_library"), m_ci{ci} {}

  void translate_item(const Twine &t) override {
    if (!m_out) {
      SmallString<128> path(m_ci->getFrontendOpts().OutputFile);
      ::llvm::sys::path::replace_extension(path, "flags");

      m_out = m_ci->createOutputFile(path, false, false, false);
    }

    if (m_out) {
      auto flag = ("-l" + t).str();
      (*m_out) << flag << "\n";
    }
  }
};

class impls_pragma_handler : public ecow_idlist_pragma_handler {
  strvec *m_impls;

public:
  impls_pragma_handler(strvec *i)
      : ecow_idlist_pragma_handler("add_impl"), m_impls{i} {}

  void translate_item(const Twine &t) override {
    m_impls->push_back(t.str());
    llvm::errs() << t.str() << "\n";
  }
};

struct ecow_action : WrapperFrontendAction {
  strvec m_impls{};

  ecow_action()
      : WrapperFrontendAction{
            std::make_unique<GenerateModuleInterfaceAction>()} {}

  bool BeginSourceFileAction(CompilerInstance &ci) override {
    auto &pp = ci.getPreprocessor();
    pp.AddPragmaHandler("ecow", new syslib_pragma_handler(&ci));
    pp.AddPragmaHandler("ecow", new impls_pragma_handler(&m_impls));

    return WrapperFrontendAction::BeginSourceFileAction(ci);
  }
};

bool ecow::llvm::compile(const input &in) {
  std::string title = "ecow clang driver";

  auto diag_opts =
      IntrusiveRefCntPtr<DiagnosticOptions>{new DiagnosticOptions()};
  auto diag_ids = IntrusiveRefCntPtr<DiagnosticIDs>{new DiagnosticIDs()};
  auto diag_cli = new TextDiagnosticPrinter(::llvm::errs(), &*diag_opts);

  DiagnosticsEngine diags{diag_ids, diag_opts, diag_cli};

  Driver driver{in.clang_exe, in.triple, diags, title};

  std::string to = (std::filesystem::current_path() / in.to).string();

  std::vector<const char *> args{};
  for (auto &str : in.cmd_line) {
    args.push_back(str.data());
  }
  auto c = driver.BuildCompilation(args);
  if (!c || c->containsError())
    // We did a mistake in clang args. Bail out and let the diagnostics
    // client do its job informing the user
    return false;

  auto cc1_args = c->getJobs().getJobs()[0]->getArguments();

  auto files = IntrusiveRefCntPtr<FileManager>(new FileManager({}));
  auto cdiag =
      CompilerInstance::createDiagnostics(&*diag_opts, diag_cli, false);

  SourceManager src_mgr(*cdiag, *files);
  cdiag->setSourceManager(&src_mgr);

  auto cinv = std::make_shared<CompilerInvocation>();
  CompilerInvocation::CreateFromArgs(*cinv, cc1_args, *cdiag);

  auto pch_opts = std::make_shared<PCHContainerOperations>();
  CompilerInstance cinst{pch_opts};
  cinst.setInvocation(cinv);
  cinst.setFileManager(&*files);
  cinst.createDiagnostics(diag_cli, false);
  cinst.createSourceManager(*files);

  auto ext = std::filesystem::path{to}.extension();
  std::unique_ptr<FrontendAction> a{};
  if (ext == ".pcm") {
    a.reset(new ecow_action{});
  } else if (ext == ".o") {
    a.reset(new EmitObjAction{});
  } else {
    return false;
  }

  files->clearStatCache();

  return cinst.ExecuteAction(*a);
}

static struct init_llvm {
  init_llvm() {
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();
  }
  ~init_llvm() { llvm::llvm_shutdown(); }
} XX;
