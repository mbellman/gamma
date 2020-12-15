#define GAMMA_ENABLE_BENCHMARK_TESTING

#include "Gamma.h"
#include "benchmarks/matrix_multiplication.h"

using namespace Gamma;

void benchmark_matrix_multiplication() {
  log("benchmark_matrix_multiplication\n");

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

  log("with 2 multiplications\n");

  Gm_RepeatBenchmarkTest([&]() {
    for (uint32 i = 0; i < size; i++) {
      auto& transformable = transformables[i];

      transformable.matrix = (
        Matrix4f::translation(transformable.position) *
        Matrix4f::scale(transformable.scale) *
        Matrix4f::rotation(transformable.rotation)
      );
    }
  }, 50);

  log("with Matrix4f::transformation()\n");

  Gm_RepeatBenchmarkTest([&]() {
    for (uint32 i = 0; i < size; i++) {
      auto& transformable = transformables[i];

      transformable.matrix = Matrix4f::transformation(
        transformable.position,
        transformable.rotation,
        transformable.scale
      );
    }
  }, 50);
}