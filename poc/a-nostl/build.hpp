#include "../../ecow.hpp"

auto exported() {
  using namespace ecow;

  auto m = unit::create<mod>("m");
  m->add_part("interface_part");
  m->add_part("impl_part");
  m->add_impl("impl");

  m->add_feat<export_symbol>("main");
  m->add_feat<inline_js>("test", "() => console.log('hello')");

  return m;
}
