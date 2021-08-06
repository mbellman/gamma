#include <vector>

#include "Gamma.h"
#include "benchmarks/object_management.h"

using namespace Gamma;

constexpr static uint32 TOTAL_MESHES = 100;
constexpr static uint32 TOTAL_OBJECTS = 10000;
constexpr static uint32 TEST_ITERATIONS = 10000;

static uint64 benchmark_pointer_objects() {
  log("benchmark_pointer_objects");

  std::vector<Object*> ptr_objects;

  for (uint32 i = 0; i < TOTAL_OBJECTS; i++) {
    ptr_objects.push_back(new Object());
  }

  // Simulate object recycling (memory fragmentation)
  for (uint32 x = 5; x < 10; x += 2) {
    for (uint32 i = 0; i < TOTAL_OBJECTS; i += x) {
      delete ptr_objects[i];

      ptr_objects[i] = new Object();
    }
  }

  return Gm_RepeatBenchmarkTest([&]() {
    for (uint32 i = 0; i < TOTAL_OBJECTS; i++) {
      auto& object = *ptr_objects[i];

      object.position = Vec3f(1.0f, 0.5f, 0.25f);
      object.scale = 20.0f;
      object.rotation = Vec3f(0.9f, 2.3f, 1.4f);
    }
  }, TEST_ITERATIONS);
}

static uint64 benchmark_pool_objects() {
  log("benchmark_pool_objects");

  ObjectPool pool;

  std::vector<ObjectPool*> pools;

  for (uint32 i = 0; i < TOTAL_MESHES; i++) {
    pools.push_back(new ObjectPool());
    pools[i]->reserve(100);

    for (uint32 j = 0; j < 100; j++) {
      pools[i]->createObject();
    }
  }

  return Gm_RepeatBenchmarkTest([&]() {
    for (uint32 i = 0; i < TOTAL_MESHES; i++) {
      auto& pool = *pools[i];

      for (auto& object : pool) {
        object.position = Vec3f(1.0f, 0.5f, 0.25f);
        object.scale = 20.0f;
        object.rotation = Vec3f(0.9f, 2.3f, 1.4f);
      }
    }
  }, TEST_ITERATIONS);
}

void benchmark_object_management() {
  Gm_CompareBenchmarks(
    benchmark_pointer_objects(),
    benchmark_pool_objects()
  );
}