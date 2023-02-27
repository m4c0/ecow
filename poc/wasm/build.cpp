#include "../../ecow.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto a = unit::create<app>("test");

  auto j = a->add_unit<>("test")->add_feat<js>();
  j->set("exported", "");
  j->set("inline", "1");
  j->set("file", "");

  return run_main(a, argc, argv);
}
