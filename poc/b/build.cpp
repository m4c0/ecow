#include "../../ecow.hpp"
#include "../a/build.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto all = unit::create<seq>("all");

  auto myapp = all->add_unit<exe>("my_cli");
  myapp->add_wsdep("a", exported());
  myapp->add_unit<>("user");

  return run_main(all, argc, argv);
}
