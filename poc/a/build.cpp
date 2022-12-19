#include "../../ecow.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto all = unit::create<seq>("all");

  auto a = all->add_unit<app>("a");
  a->add_resource("build.cpp");

  auto m = a->add_unit<mod>("m");
  m->add_part("interface_part");
  m->add_part("impl_part");
  m->add_impl("impl");

  a->add_unit<>("user");

  auto mjs = m->add_feat<js>();
  mjs->set("main", "");
  mjs->set("test", "() => console.log('hello')");

  // tests objc and otherplatform-specifics
  auto pf = a->add_unit<per_feat<seq>>("per_feat");
  auto dummy = pf->for_feature(features::objective_c).add_unit<objc>("dummy");
  dummy->add_framework("Foundation"); // Not really needed with clang

  // TODO: define a way to build/run "host" tools (such as asset builders)
  // all->add_unit<sys>(a->exe_path().make_preferred().string());

  auto myapp = all->add_unit<exe>("my_cli");
  myapp->add_ref(m);
  myapp->add_unit<>("user");

  return run_main(all, argc, argv);
}
