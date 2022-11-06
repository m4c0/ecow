#include "ecow.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  seq all{"all"};

  auto a = all.add_unit<app>("a");

  auto m = a->add_unit<mod>("m");
  m->add_part("interface_part");
  m->add_part("impl_part");
  m->add_impl("impl");

  a->add_unit<>("user");
#ifdef __APPLE__
  a->add_unit<>("dummy.mm"); // Just tests "mm" extension
#endif

  // TODO: define a way to build/run "host" tools (such as asset builders)
  // all.add_unit<sys>(a->exe_path().make_preferred().string());

  auto myapp = all.add_unit<exe>("my_cli");
  myapp->add_ref(m);
  myapp->add_unit<>("user");

  return run_main(all, argc, argv);
}
