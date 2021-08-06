#include "Gamma.h"
#include "benchmarks/matrix_multiplication.h"

using namespace Gamma;

constexpr static uint32 TEST_ITERATIONS = 500;

static uint64 benchmark_2_multiplications() {
  log("benchmark_2_multiplications");

  struct Transformable {
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
    Matrix4f matrix;
  };

  constexpr static uint32 size = 10000;

  Transformable* transformables = new Transformable[size];

  for (uint32 i = 0; i < size; i++) {
    transformables[i].position = Vec3f(10.0f, 5.0f, 12.3f);
    transformables[i].rotation = Vec3f(-3.6f, 7.9f, 0.035f);
    transformables[i].scale = Vec3f(15.f, 12.3f, 0.8f);
  }

  return Gm_RepeatBenchmarkTest([&]() {
    for (uint32 i = 0; i < size; i++) {
      auto& transformable = transformables[i];

      transformable.matrix = (
        Matrix4f::translation(transformable.position) *
        Matrix4f::scale(transformable.scale) *
        Matrix4f::rotation(transformable.rotation)
      );
    }
  }, TEST_ITERATIONS);
}

static uint64 benchmark_Matrix4f_transformation() {
  log("benchmark_Matrix4f_transformation");

  struct Transformable {
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
    Matrix4f matrix;
  };

  constexpr static uint32 size = 10000;

  Transformable* transformables = new Transformable[size];

  for (uint32 i = 0; i < size; i++) {
    transformables[i].position = Vec3f(10.0f, 5.0f, 12.3f);
    transformables[i].rotation = Vec3f(-3.6f, 7.9f, 0.035f);
    transformables[i].scale = Vec3f(15.f, 12.3f, 0.8f);
  }

  return Gm_RepeatBenchmarkTest([&]() {
    for (uint32 i = 0; i < size; i++) {
      auto& transformable = transformables[i];

      transformable.matrix = Matrix4f::transformation(
        transformable.position,
        transformable.rotation,
        transformable.scale
      );
    }
  }, TEST_ITERATIONS);
}

void benchmark_matrix_multiplication() {
  Gm_CompareBenchmarks(
    benchmark_2_multiplications(),
    benchmark_Matrix4f_transformation()
  );
}