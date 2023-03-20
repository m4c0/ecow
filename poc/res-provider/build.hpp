#include "../../ecow.hpp"

auto res() {
  using namespace ecow;

  // This is an attempt of simulating a resource that needs to be build (like
  // shaders)
  auto r = unit::create<unit>("i-am-res-src");
  r->add_resource("i-am-res-src.o");
  return r;
}
