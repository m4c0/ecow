#include "ecow.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  auto bld = unit::create<tool>("build");
  bld->add_unit("build")->add_include_dir("../ecow");

  auto all = unit::create<seq>("all");
  all->add_ref(bld);
  all->add_unit<sys>(bld->executable().string());
  return run_main(all, argc, argv);
}
