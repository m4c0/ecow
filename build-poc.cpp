#include "ecow.hpp"

using namespace ecow;

int main() {
  seq all{"all"};

  auto *a = all.add_unit<exe>("a");

  auto *m = a->add_unit<mod>("m");
  m->add_part("interface_part");
  m->add_part("impl_part");
  m->add_impl("impl");

  a->add_unit<>("user");

  all.add_unit<sys>("a");
  return all.build() ? 0 : 1;
}
