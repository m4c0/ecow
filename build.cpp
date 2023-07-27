#define ECOW_META_BUILD
#include "ecow.hpp"

using namespace ecow;

#ifdef __APPLE__
#define CLANG_FINDER "brew --prefix llvm@16"
#else
#define CLANG_FINDER "which llvm@16"
#endif

static auto clang_dir() {
  return std::filesystem::path{impl::popen(CLANG_FINDER)};
}

class builder : public exe {
protected:
  virtual void build_self() const override {
    if (impl::current_target()->supports(host))
      exe::build_self();
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
  auto cdir = clang_dir();

  auto bld = unit::create<builder>("build");
  auto u = bld->add_unit("build");

  if (!std::filesystem::exists("ecow.hpp")) {
    u->add_include_dir("../ecow");
    u->add_include_dir((cdir / "include").string());
    u->add_library_dir((cdir / "lib").string());
    u->add_system_library("clang");
    u->add_system_library("clang-cpp");
    u->add_system_library("LLVM");
  }

  auto all = unit::create<seq>("all");
  all->add_ref(bld);
  all->add_unit<sys>(bld->executable().string());
  return run_main(all, argc, argv);
}
