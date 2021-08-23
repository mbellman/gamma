#include <vector>

#include "Gamma.h"
#include "benchmarks/object_management.h"

using namespace Gamma;

constexpr static uint32 TOTAL_MESHES = 100;
constexpr static uint32 TOTAL_OBJECTS = 100000;

static uint64 benchmark_pointer_object_properties(uint32 iterations) {
  Console::log("benchmark_pointer_object_properties");

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
  Console::log("benchmark_pointer_object_matrices");

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
  Console::log("benchmark_pool_object_properties");

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

      // for (uint32 j = 0; j < pool.total(); j++) {
      //   auto& object = *pool.getById(j);

      //   object.position = Vec3f(1.0f, 0.5f, 0.25f);
      //   object.scale = 20.0f;
      //   object.rotation = Vec3f(0.9f, 2.3f, 1.4f);
      // }

      for (auto& object : pool) {
        object.position = Vec3f(1.0f, 0.5f, 0.25f);
        object.scale = 20.0f;
        object.rotation = Vec3f(0.9f, 2.3f, 1.4f);
      }
    }
  }, iterations);
}

static uint64 benchmark_pool_object_matrices(uint32 iterations) {
  Console::log("benchmark_pool_object_matrices");

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
  Console::log("benchmark_soa_object_properties");

  struct SOA_Objects {
    float *x = nullptr;
    float *y = nullptr;
    float *z = nullptr;
    float *sx = nullptr;
    float *sy = nullptr;
    float *sz = nullptr;
    float *rx = nullptr;
    float *ry = nullptr;
    float *rz = nullptr;
  };

  SOA_Objects objects;

  objects.x = new float[TOTAL_OBJECTS];
  objects.y = new float[TOTAL_OBJECTS];
  objects.z = new float[TOTAL_OBJECTS];

  objects.sx = new float[TOTAL_OBJECTS];
  objects.sy = new float[TOTAL_OBJECTS];
  objects.sz = new float[TOTAL_OBJECTS];

  objects.rx = new float[TOTAL_OBJECTS];
  objects.ry = new float[TOTAL_OBJECTS];
  objects.rz = new float[TOTAL_OBJECTS];

  defer({
    delete[] objects.x;
    delete[] objects.y;
    delete[] objects.z;

    delete[] objects.sx;
    delete[] objects.sy;
    delete[] objects.sz;

    delete[] objects.rx;
    delete[] objects.ry;
    delete[] objects.rz;
  });

  #define setAll(property, value) loop(TOTAL_OBJECTS) {\
    objects.property[idx] = value;\
  }\

  return Gm_RepeatBenchmarkTest([&]() {
    setAll(x, 1.0f);
    setAll(y, 0.5f);
    setAll(z, 0.25f);

    setAll(sx, 20.0f);
    setAll(sy, 20.0f);
    setAll(sz, 20.0f);

    setAll(rx, 0.9f);
    setAll(ry, 2.3f);
    setAll(rz, 1.4f);
  }, iterations);
}

static uint64 benchmark_soa_object_matrices(uint32 iterations) {
  Console::log("benchmark_soa_object_matrices");

  struct SOA_Objects {
    float *x = nullptr;
    float *y = nullptr;
    float *z = nullptr;
    float *sx = nullptr;
    float *sy = nullptr;
    float *sz = nullptr;
    float *rx = nullptr;
    float *ry = nullptr;
    float *rz = nullptr;
    Matrix4f* matrices = nullptr;
  };

  SOA_Objects objects;

  objects.x = new float[TOTAL_OBJECTS];
  objects.y = new float[TOTAL_OBJECTS];
  objects.z = new float[TOTAL_OBJECTS];

  objects.sx = new float[TOTAL_OBJECTS];
  objects.sy = new float[TOTAL_OBJECTS];
  objects.sz = new float[TOTAL_OBJECTS];

  objects.rx = new float[TOTAL_OBJECTS];
  objects.ry = new float[TOTAL_OBJECTS];
  objects.rz = new float[TOTAL_OBJECTS];

  objects.matrices = new Matrix4f[TOTAL_OBJECTS];

  defer({
    delete[] objects.x;
    delete[] objects.y;
    delete[] objects.z;

    delete[] objects.sx;
    delete[] objects.sy;
    delete[] objects.sz;

    delete[] objects.rx;
    delete[] objects.ry;
    delete[] objects.rz;

    delete[] objects.matrices;
  });

  #define setAll(property, value) loop(TOTAL_OBJECTS) {\
    objects.property[idx] = value;\
  }\

  return Gm_RepeatBenchmarkTest([&]() {
    setAll(x, 1.0f);
    setAll(y, 0.5f);
    setAll(z, 0.25f);

    setAll(sx, 20.0f);
    setAll(sy, 20.0f);
    setAll(sz, 20.0f);

    setAll(rx, 0.9f);
    setAll(ry, 2.3f);
    setAll(rz, 1.4f);

    loop(TOTAL_OBJECTS) {
      objects.matrices[idx] = Matrix4f::transformation(
        Vec3f(1.0f, 0.5f, 0.25f),
        20.0f,
        Vec3f(0.9f, 2.3f, 1.4f)
      ).transpose();
    }
  }, iterations);
}

void benchmark_object_management() {
  auto b_pointers = benchmark_pointer_object_properties(1);
  auto b_pool = benchmark_pool_object_properties(1);
  auto b_soa = benchmark_soa_object_properties(1);

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