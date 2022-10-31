# ecow

C++ library to build C++ projects.

This requires clang, version 15 or later. It is highly opiniated based on how I develop my personal C++ projects.

## Disclaimer

This is not production-ready. Use it at your own risk.

## Trying it out

1. Clone it
2. Build the proof-of-concept builder
```sh
clang++ -std=c++20 build-poc.cpp -o build
```
3. Run the proof-of-concept builder
```
./build
```

## Using it

1. Clone it somewhere
2. Write your builder code. Check the code for which classes you should use. A quintessential "hello world" would be like this:
```cpp
#include "ecow.hpp"
using namespace ecow;
int main() {
  return sys("echo hello world").build() ? 0 : 1;
}
```
3. Build your builder
```
clang++ -std=c++20 build.cpp -o build
```
4. Run your builder
```
./build
```
