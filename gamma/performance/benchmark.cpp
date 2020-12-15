#include <chrono>
#include <iostream>
#include <thread>

#include "performance/benchmark.h"

static auto createTimer = []() {
  auto start = std::chrono::system_clock::now();

  return [&]() {
    auto end = std::chrono::system_clock::now();

    std::chrono::duration duration = end - start;

    return duration;
  };
};

namespace Gamma {
  void Gm_RepeatBenchmarkTest(const std::function<void()>& test, uint32 times) {
    auto getTime = createTimer();

    for (uint32 i = 0; i < times; i++) {
      Gm_RunBenchmarkTest(test);
    }

    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(getTime()).count();

    std::cout << "\nFinished " << times << " iterations in " << milliseconds << "ms\n\n";
  }

  void Gm_RunBenchmarkTest(const std::function<void()>& test) {
    auto getTime = createTimer();

    test();

    auto time = getTime();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(time).count();

    std::cout << "Benchmark finished in: " << milliseconds << "ms (" << microseconds << "us)\n";
  }

  void Gm_RunLoopedBenchmarkTest(const std::function<void()>& test, uint32 pause) {
    while (true) {
      Gm_RunBenchmarkTest(test);
      Gm_Sleep(pause);
    }
  }

  void Gm_Sleep(uint32 milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  }
}