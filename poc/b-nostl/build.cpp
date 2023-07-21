#include "../../ecow.hpp"
#include "../c-nostl/build.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto all = unit::create<seq>("all");

  auto m = all->add_unit<mod>("mb");
  m->add_wsdep("c-nostl", dbl_exp());

  auto myapp = all->add_unit<app>("bee");
  myapp->add_ref(m);
  myapp->add_unit<>("bee-exe");

  return run_main(all, argc, argv);
}
