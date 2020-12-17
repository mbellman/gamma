#define GAMMA_ENABLE_BENCHMARK_TESTING

#include <vector>

#include "Gamma.h"
#include "benchmarks/object_management.h"

using namespace Gamma;

constexpr static uint32 TOTAL_MESHES = 100;
constexpr static uint32 TOTAL_OBJECTS = 10000;

void benchmark_objects_optimized() {
  // setup
  log("benchmark_objects_optimized\n");

  struct TestObject {
    uint32 meshId;
    uint32 objectId;
    uint32 matrixId;
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
  };

  struct TestMesh {
    std::vector<Matrix4f> matrices;
    std::vector<TestObject> objects;
  };

  struct TransformCommand {
    uint32 meshId;
    uint32 objectId;
    uint32 matrixId;
  };

  std::vector<TestMesh*> meshes;
  TransformCommand* commands = new TransformCommand[10000];
  uint32 totalCommands = 0;

  for (uint32 i = 0; i < 100; i++) {
    auto* mesh = new TestMesh();

    mesh->objects.resize(100);
    mesh->matrices.resize(100);

    for (uint32 j = 0; j < 100; j++) {
      mesh->objects[j].meshId = i;
      mesh->objects[j].objectId = j;
      mesh->objects[j].matrixId = j;
    }

    meshes.push_back(mesh);
  }

  auto transform = [&](const TestObject& object) mutable {
    commands[totalCommands++] = {
      object.meshId,
      object.objectId,
      object.matrixId
    };
  };

  // test
  Gm_RepeatBenchmarkTest([&]() {
    totalCommands = 0;

    for (uint32 i = 0; i < meshes.size(); i++) {
      auto& mesh = *meshes[i];

      for (uint32 j = 0; j < mesh.objects.size(); j++) {
        auto& object = mesh.objects[j];

        object.position = Vec3f(1.0f, 0.5f, 10.0f);
        object.rotation = Vec3f(1.0f, 1.0f, 2.0f);
        object.scale = Vec3f(2.0f);

        transform(object);
      }
    }

    for (uint32 x = 0; x < totalCommands; x++) {
      auto& command = commands[x];
      auto& mesh = *meshes[command.meshId];
      auto& object = mesh.objects[command.objectId];
      
      mesh.matrices[command.matrixId] = Matrix4f::transformation(
        object.position,
        object.scale,
        object.rotation
      ).transpose();
    }
  }, 50);
}

void benchmark_objects_unoptimized() {
  // setup
  log("benchmark_objects_unoptimized\n");

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