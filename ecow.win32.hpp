#pragma once
#include "ecow.core.hpp"

namespace ecow::impl {
class native_target : public target {
  std::string cxx() override { return "clang++ -fno-ms-compatibility"; }
};
} // namespace ecow::impl
