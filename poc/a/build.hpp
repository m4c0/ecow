#include "../../ecow.hpp"

auto exported() {
  using namespace ecow;

  auto m = unit::create<mod>("m");
  m->add_part("interface_part");
  m->add_part("impl_part");
  m->add_impl("impl");

  auto mjs = m->add_feat<js>();
  mjs->set("main", "");
  mjs->set("test", "() => console.log('hello')");

  return m;
}
