#pragma once

#include <chrono>
#include <functional>

#include "system/type_aliases.h"

namespace Gamma {
  auto Gm_CreateTimer() {
    auto start = std::chrono::system_clock::now();

    return [&]() {
      auto end = std::chrono::system_clock::now();

      std::chrono::system_clock::duration duration = end - start;

      return duration;
    };
  };

  void Gm_RepeatBenchmarkTest(const std::function<void()>& test, uint32 times = 1);
  void Gm_RunBenchmarkTest(const std::function<void()>& test);
  void Gm_RunLoopedBenchmarkTest(const std::function<void()>& test, uint32 pause = 1000);
  void Gm_Sleep(uint32 milliseconds);
}