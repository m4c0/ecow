#include "../../ecow.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto all = unit::create<seq>("all");

  auto t = all->add_unit<tool>("tool.exe");
  t->add_unit<>("tool");

  all->add_unit<sys>(t->executable().string());

  return run_main(all, argc, argv);
}
