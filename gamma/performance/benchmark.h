#pragma once

#include <functional>

#include "system/type_aliases.h"

namespace Gamma {
  void Gm_RunBenchmarkTest(const std::function<void()>& test);
  void Gm_RunLoopedBenchmarkTest(const std::function<void()>& test, uint32 pause = 1000);
  void Gm_Sleep(uint32 milliseconds);
}