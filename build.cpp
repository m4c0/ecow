#define ECOW_META_BUILD
#include "ecow.hpp"

using namespace ecow;

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
  auto cdir = impl::clang_dir();

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
  // TODO: pass argv if we actually want to meta-invoke
  // all->add_unit<sys>(bld->executable().string());
  return run_main(all, argc, argv);
}
