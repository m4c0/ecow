#include "../../ecow.hpp"
#include "../a-nostl/build.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto a = exported();

  auto all = unit::create<seq>("all");

  auto m = all->add_unit<mod>("mb");
  m->add_wsdep("a-nostl", a);

  auto myapp = all->add_unit<app>("bee");
  myapp->add_wsdep("a-nostl", a);
  myapp->add_unit<>("bee-exe");

  return run_main(all, argc, argv);
}
