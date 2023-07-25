#include "../../ecow.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  auto all = unit::create<seq>("all");
  all->add_unit<mod>("c");
  all->add_unit<mod>("b");

  auto a = all->add_unit<mod>("a");
  a->add_part("z");
  a->add_part("y");
  a->add_part("x");

  return run_main(all, argc, argv);
}
