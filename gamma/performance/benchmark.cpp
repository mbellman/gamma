#include <chrono>
#include <iostream>
#include <thread>

#include "performance/benchmark.h"

namespace Gamma {
  void Gm_RunBenchmarkTest(const std::function<void()>& test) {
    auto startTime = std::chrono::system_clock::now();

    test();

    auto endTime = std::chrono::system_clock::now();

    std::chrono::duration duration = endTime - startTime;

    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

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