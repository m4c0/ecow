module;
extern "C" int printf(const char *, ...);

module m:impl_part;
import :interface_part;

const auto w = "world.";
void world() { printf("%s\n", w); }
