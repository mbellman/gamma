#define GAMMA_ENABLE_BENCHMARK_TESTING

#include <vector>

#include "Gamma.h"
#include "benchmarks/object_management.h"

using namespace Gamma;

constexpr static uint32 TOTAL_MESHES = 100;
constexpr static uint32 TOTAL_OBJECTS = 10000;

void benchmark_objects_optimized() {
  // setup
  log("benchmark_objects_optimized");

  struct PackedObject {
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
    Matrix4f matrix;
  };

  std::vector<PackedObject> objects;

  objects.resize(TOTAL_OBJECTS);

  // test
  Gm_RepeatBenchmarkTest([&]() {
    for (auto& object : objects) {
      object.position = Vec3f(1.0f, 0.5f, 10.0f);
      object.rotation = Vec3f(1.0f, 1.0f, 2.0f);
      object.scale = Vec3f(2.0f);

      object.matrix = (
        Matrix4f::translation(object.position) *
        Matrix4f::scale(object.scale) *
        Matrix4f::rotation(object.rotation)
      ).transpose();
    }
  }, 50);
}

void benchmark_objects_unoptimized() {
  // setup
  log("benchmark_objects_unoptimized");

  struct PointerObject;

  struct PointerMesh {
    std::vector<Matrix4f> matrices;
    std::vector<PointerObject*> objects;
  };

  struct PointerObject {
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
    uint32 id;
    PointerMesh* parent = nullptr;
  };

  std::vector<PointerMesh*> meshes;
  std::vector<PointerObject*> objectsToUpdate;

  for (uint32 i = 0; i < TOTAL_MESHES; i++) {
    auto* mesh = new PointerMesh();

    mesh->matrices.resize(TOTAL_OBJECTS);
    meshes.push_back(mesh);

    for (uint32 j = 0; j < TOTAL_OBJECTS; j++) {
      auto* object = new PointerObject();

      object->id = j;
      object->parent = mesh;

      mesh->objects.push_back(object);

      if (j % 100 == 0) {
        objectsToUpdate.push_back(object);
      }
    }
  }

  // test
  Gm_RepeatBenchmarkTest([&]() {
    for (auto* object : objectsToUpdate) {
      object->position = Vec3f(1.0f, 0.5f, 10.0f);
      object->rotation = Vec3f(1.0f, 1.0f, 2.0f);
      object->scale = Vec3f(2.0f);

      auto* mesh = object->parent;

      mesh->matrices[object->id] = (
        Matrix4f::translation(object->position) *
        Matrix4f::scale(object->scale) *
        Matrix4f::rotation(object->rotation)
      ).transpose();
    }
  }, 50);
}