#include "ecow.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  seq all{"all"};

  auto a = all.add_unit<exe>("a");

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

  all.add_ref(a); // Tests if we can build incrementally

  return all.main(argc, argv);
}
