#define ECOW_META_BUILD
#include "ecow.hpp"

using namespace ecow;

class builder : public exe {
protected:
  virtual void build_self() const override {
    if (impl::current_target()->supports(host))
      exe::build_self();
  }

public:
  using exe::exe;

  [[nodiscard]] auto executable() const {
    return impl::host_target{}.exe_name(name());
  }
};

int main(int argc, char **argv) {
  auto bld = unit::create<builder>("build");
  auto u = bld->add_unit("build");

  if (!std::filesystem::exists("ecow.hpp")) {
    u->add_include_dir("../ecow");
  }

  auto all = unit::create<seq>("all");
  all->add_ref(bld);
  all->add_unit<sys>(bld->executable());
  return run_main(all, argc, argv);
}
