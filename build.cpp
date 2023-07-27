#include <stdlib.h>

#ifdef __APPLE__
#define ECOW_OUTPUT "build"
#else
#define ECOW_OUTPUT "build.exe"
#endif

int main() {
  return system("clang++ -std=c++20 -I../ecow build.cpp -o " ECOW_OUTPUT);
}
