#include "../res-provider/build.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  auto a = unit::create<app>("app");
  a->add_wsdep("res-provider", res());
  a->add_unit("main");
  return run_main(a, argc, argv);
}
