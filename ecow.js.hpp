#pragma once

#include "ecow.feat.hpp"

namespace ecow {
class js : public feat {
protected:
  [[nodiscard]] features type() const noexcept override { return webassembly; }

public:
  using feat::set;
};
} // namespace ecow
