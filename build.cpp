#define ECOW_META_BUILD
#include "ecow.hpp"

namespace ecow::llvm {
bool compile(const input &in) {
  std::ostringstream str{};
  str << "clang++";
  for (auto it = in.cmd_line.begin() + 1; it != in.cmd_line.end(); ++it) {
    str << ' ' << '"' << *it << '"';
  }
  return 0 == system(str.str().c_str());
}
} // namespace ecow::llvm

using namespace ecow;

class builder : public exe {
  std::string m_ecow_o;

protected:
  void build_self() const override {
    if (!impl::current_target()->supports(host))
      return;

    const auto cdir = impl::clang_dir();
    const auto cinc = cdir / "include";
    const auto clib = cdir / "lib";

    if (std::filesystem::exists("ecow.hpp")) {
      // impl::clang{"build.cpp", exe_name()}.run(true);
      impl::clang("ecow.cpp", "ecow.o")
          .add_arg("-c")
          .add_arg("-I" + cinc.string())
#if _WIN32
          .add_arg("-fms-runtime-lib=dll")
#endif
          .run(true);
      return;
    }

    auto c = impl::clang{"build.cpp", exe_name()}
                 .add_arg(m_ecow_o)
                 .add_arg("-I../ecow")
                 .add_arg("-L" + clib.string())
#if _WIN32
                 .add_arg("-fms-runtime-lib=dll")
                 .add_arg("-nostdlib")
                 .add_arg("-nostdlib++")
                 .add_arg("-lVersion");

    for (const auto &e : std::filesystem::directory_iterator(clib)) {
      const auto &p = e.path();
      if (p.extension() != ".lib")
        continue;
      if (p.stem() == "LLVM-C")
        continue;
      if (p.stem() == "libclang")
        continue;

      c.add_arg("-l" + p.stem().string());
    }
#else
                 .add_arg("-lclang")
                 .add_arg("-lclang-cpp")
                 .add_arg("-lLLVM");
#endif
    c.run(true);
  }
  std::string final_exe_name() const override { return exe_name(); }

  static auto ecow_o(const std::string &argv0) {
    return std::filesystem::path{argv0}.parent_path() / "ecow.o";
  }

public:
  explicit builder(const std::string &argv0)
      : exe("build"), m_ecow_o{ecow_o(argv0).string()} {}

  [[nodiscard]] auto executable() const {
    // This runs before "exe_name" can access a target
    return std::filesystem::current_path() /
           impl::host_target{}.exe_name(name());
  }
};

int main(int argc, char **argv) {
  auto bld = unit::create<builder>(argv[0]);
  return run_main(bld, argc, argv);
}
