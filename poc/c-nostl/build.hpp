#pragma once
#include "../../ecow.hpp"
#include "../a-nostl/build.hpp"

auto dbl_exp() {
  using namespace ecow;

  auto m = unit::create<mod>("c");
  m->add_wsdep("a-nostl", exported());
  return m;
}
