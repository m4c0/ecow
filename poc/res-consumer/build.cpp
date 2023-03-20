#include "../res-provider/build.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  auto a = unit::create<app>("app");
  a->add_wsdep("res-provider", res());
  a->add_unit("main");
  a->add_resource("main.o"); // Tests local compiled resources (shaders, etc)
  return run_main(a, argc, argv);
}
