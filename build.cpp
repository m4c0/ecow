#define ECOW_META_BUILD
#include "ecow.hpp"

using namespace ecow;

class builder : public exe {
protected:
  void build_self() const override {
    if (!impl::current_target()->supports(host))
      return;

    const auto cdir = impl::clang_dir();
    const auto cinc = cdir / "include";
    const auto clib = cdir / "lib";

    if (std::filesystem::exists("ecow.hpp")) {
      impl::clang{"build.cpp", exe_name()}.run(true);
      impl::clang("ecow.cpp", "ecow.o")
          .add_arg("-c")
          .add_arg("-I" + cinc.string())
          .run(true);
      return;
    }

    auto c = impl::clang{"build.cpp", exe_name()}
                 .add_arg("../ecow/ecow.o")
                 .add_arg("-I../ecow")
                 .add_arg("-L" + clib.string())
#if _WIN32
                 .add_arg("-fms-runtime-lib=dll")
                 .add_arg("-nostdlib")
                 .add_arg("-nostdlib++")
                 .add_arg("-lVersion");

    for (const auto &e : std::filesystem::directory_iterator(clib)) {
      const auto &p = e.path();
      if (p.extension() == ".lib")
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

public:
  using exe::exe;

  [[nodiscard]] auto executable() const {
    // This runs before "exe_name" can access a target
    return std::filesystem::current_path() /
           impl::host_target{}.exe_name(name());
  }
};

int main(int argc, char **argv) {
  auto bld = unit::create<builder>("build");
  return run_main(bld, argc, argv);
}
