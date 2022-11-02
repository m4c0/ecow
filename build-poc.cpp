#include "ecow.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  seq all{"all"};

  auto a = all.add_unit<exe>("a.exe");

  auto m = a->add_unit<mod>("m");
  m->add_part("interface_part");
  m->add_part("impl_part");
  m->add_impl("impl");

  a->add_unit<>("user");
#ifdef __APPLE__
  a->add_unit<>("dummy"); // Just tests "mm" extension
#endif

#if _WIN32
  all.add_unit<sys>("a.exe");
#else
  all.add_unit<sys>("./a.exe");
#endif

  auto myapp = all.add_unit<app>("my_app");
  myapp->add_ref(m);
  myapp->add_unit<>("user");

  return all.main(argc, argv);
}
