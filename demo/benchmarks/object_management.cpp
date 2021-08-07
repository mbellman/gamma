#include <vector>

#include "Gamma.h"
#include "benchmarks/object_management.h"

using namespace Gamma;

constexpr static uint32 TOTAL_MESHES = 100;
constexpr static uint32 TOTAL_OBJECTS = 1000000;

static uint64 benchmark_pointer_object_properties(uint32 iterations) {
  log("benchmark_pointer_object_properties");

  std::vector<Object*> ptr_objects;

  loop(TOTAL_OBJECTS) {
    ptr_objects.push_back(new Object());
  }

  defer({
    loop(TOTAL_OBJECTS) {
      delete ptr_objects[idx];
    }

    ptr_objects.clear();
  });

  // Simulate object recycling (memory fragmentation)
  for (uint32 x = 5; x < 10; x += 2) {
    for (uint32 i = 0; i < TOTAL_OBJECTS; i += x) {
      delete ptr_objects[i];

      ptr_objects[i] = new Object();
    }
  }

  return Gm_RepeatBenchmarkTest([&]() {
    loop(TOTAL_OBJECTS) {
      auto& object = *ptr_objects[idx];

      object.position = Vec3f(1.0f, 0.5f, 0.25f);
      object.scale = 20.0f;
      object.rotation = Vec3f(0.9f, 2.3f, 1.4f);
    }
  }, iterations);
}

static uint64 benchmark_pointer_object_matrices(uint32 iterations) {
  log("benchmark_pointer_object_matrices");

  std::vector<Object*> ptr_objects;
  std::vector<Matrix4f> ptr_matrices;

  loop(TOTAL_OBJECTS) {
    ptr_objects.push_back(new Object());
  }

  ptr_matrices.resize(TOTAL_OBJECTS);

  defer({
    loop(TOTAL_OBJECTS) {
      delete ptr_objects[idx];
    }

    ptr_objects.clear();
    ptr_matrices.clear();
  });

  // Simulate object recycling (memory fragmentation)
  for (uint32 x = 5; x < 10; x += 2) {
    for (uint32 i = 0; i < TOTAL_OBJECTS; i += x) {
      delete ptr_objects[i];

      ptr_objects[i] = new Object();
    }
  }

  return Gm_RepeatBenchmarkTest([&]() {
    loop(TOTAL_OBJECTS) {
      auto& object = *ptr_objects[idx];

      object.position = Vec3f(1.0f, 0.5f, 0.25f);
      object.scale = 20.0f;
      object.rotation = Vec3f(0.9f, 2.3f, 1.4f);

      ptr_matrices[idx] = Matrix4f::transformation(
        object.position,
        object.scale,
        object.rotation
      ).transpose();
    }
  }, iterations);
}

static uint64 benchmark_pool_object_properties(uint32 iterations) {
  log("benchmark_pool_object_properties");

  std::vector<ObjectPool*> pools;

  for (uint32 i = 0; i < TOTAL_MESHES; i++) {
    pools.push_back(new ObjectPool());
    pools[i]->reserve(TOTAL_OBJECTS / TOTAL_MESHES);

    for (uint32 j = 0; j < TOTAL_OBJECTS / TOTAL_MESHES; j++) {
      pools[i]->createObject();
    }
  }

  defer({
    for (uint32 i = 0; i < TOTAL_MESHES; i++) {
      pools[i]->free();
    }
  });

  return Gm_RepeatBenchmarkTest([&]() {
    for (uint32 i = 0; i < TOTAL_MESHES; i++) {
      auto& pool = *pools[i];

      for (auto& object : pool) {
        object.position = Vec3f(1.0f, 0.5f, 0.25f);
        object.scale = 20.0f;
        object.rotation = Vec3f(0.9f, 2.3f, 1.4f);
      }
    }
  }, iterations);
}

static uint64 benchmark_pool_object_matrices(uint32 iterations) {
  log("benchmark_pool_object_matrices");

  std::vector<ObjectPool*> pools;

  for (uint32 i = 0; i < TOTAL_MESHES; i++) {
    pools.push_back(new ObjectPool());
    pools[i]->reserve(TOTAL_OBJECTS / TOTAL_MESHES);

    for (uint32 j = 0; j < TOTAL_OBJECTS / TOTAL_MESHES; j++) {
      pools[i]->createObject();
    }
  }

  defer({
    for (uint32 i = 0; i < TOTAL_MESHES; i++) {
      pools[i]->free();
    }
  });

  return Gm_RepeatBenchmarkTest([&]() {
    for (uint32 i = 0; i < TOTAL_MESHES; i++) {
      auto& pool = *pools[i];

      for (uint32 j = 0; j < pool.total(); j++) {
        auto& object = *pool.getById(j);

        object.position = Vec3f(1.0f, 0.5f, 0.25f);
        object.scale = 20.0f;
        object.rotation = Vec3f(0.9f, 2.3f, 1.4f);

        pool.transformById(object._record.id, Matrix4f::transformation(
          object.position,
          object.scale,
          object.rotation
        ).transpose());
      }
    }
  }, iterations);
}

static uint64 benchmark_soa_object_properties(uint32 iterations) {
  log("benchmark_soa_object_properties");

  struct SOA_Objects {
    Vec3f *positions = nullptr;
    Vec3f *scales = nullptr;
    Vec3f *rotations = nullptr;
  };

  SOA_Objects objects;

  objects.positions = new Vec3f[TOTAL_OBJECTS];
  objects.scales = new Vec3f[TOTAL_OBJECTS];
  objects.rotations = new Vec3f[TOTAL_OBJECTS];

  defer({
    delete[] objects.positions;
    delete[] objects.scales;
    delete[] objects.rotations;
  });

  return Gm_RepeatBenchmarkTest([&]() {
    loop(TOTAL_OBJECTS) {
      objects.positions[idx] = Vec3f(1.0f, 0.5f, 0.25f);
    }

    loop(TOTAL_OBJECTS) {
      objects.scales[idx] = 20.0f;
    }

    loop(TOTAL_OBJECTS) {
      objects.rotations[idx] = Vec3f(0.9f, 2.3f, 1.4f);
    }
  }, iterations);
}

static uint64 benchmark_soa_object_matrices(uint32 iterations) {
  log("benchmark_soa_object_matrices");

  struct SOA_Objects {
    Vec3f *positions = nullptr;
    Vec3f *scales = nullptr;
    Vec3f *rotations = nullptr;
    Matrix4f *matrices = nullptr;
  };

  SOA_Objects objects;

  objects.positions = new Vec3f[TOTAL_OBJECTS];
  objects.scales = new Vec3f[TOTAL_OBJECTS];
  objects.rotations = new Vec3f[TOTAL_OBJECTS];
  objects.matrices = new Matrix4f[TOTAL_OBJECTS];

  defer({
    delete[] objects.positions;
    delete[] objects.scales;
    delete[] objects.rotations;
    delete[] objects.matrices;
  });

  return Gm_RepeatBenchmarkTest([&]() {
    loop(TOTAL_OBJECTS) {
      objects.positions[idx] = Vec3f(1.0f, 0.5f, 0.25f);
    }

    loop(TOTAL_OBJECTS) {
      objects.scales[idx] = 20.0f;
    }

    loop(TOTAL_OBJECTS) {
      objects.rotations[idx] = Vec3f(0.9f, 2.3f, 1.4f);
    }

    loop(TOTAL_OBJECTS) {
      objects.matrices[idx] = Matrix4f::transformation(
        objects.positions[idx],
        objects.scales[idx],
        objects.rotations[idx]
      ).transpose();
    }
  }, iterations);
}

void benchmark_object_management() {
  auto b_pointers = benchmark_pointer_object_properties(100);
  auto b_pool = benchmark_pool_object_properties(100);
  auto b_soa = benchmark_soa_object_properties(100);

  // auto b_pointers = benchmark_pointer_object_matrices(1);
  // auto b_pool = benchmark_pool_object_matrices(1);
  // auto b_soa = benchmark_soa_object_matrices(1);

  Gm_CompareBenchmarks(
    b_pointers,
    b_pool
  );

  Gm_CompareBenchmarks(
    b_pool,
    b_soa
  );
}