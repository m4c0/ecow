#include "../../ecow.hpp"

using namespace ecow;

int main(int argc, char **argv) {
  auto a = unit::create<app>("test");
  a->add_unit<>("test");

  a->add_feat<export_symbol>("main");
  a->add_feat<inline_js>("inline", "1");
  a->add_feat<inline_js>("file", "");
  a->add_feat<setup_js>("console.log");
  a->add_feat<setup_js>("setup_file");

  return run_main(a, argc, argv);
}
