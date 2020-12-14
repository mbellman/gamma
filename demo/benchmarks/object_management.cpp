#define GAMMA_ENABLE_BENCHMARK_TESTING

#include <vector>

#include "Gamma.h"
#include "benchmarks/object_management.h"

using namespace Gamma;

constexpr static uint32 TOTAL_MESHES = 100;
constexpr static uint32 TOTAL_OBJECTS = 10000;

struct TestObject {
  Vec3f position;
  Vec3f rotation;
  Vec3f scale;
  bool isDirty = true;
  uint32 meshId;
  uint32 objectId;
};

static void updateObjects(const std::vector<TestObject*>& objects) {
  for (auto* object : objects) {
    auto& o = *object;

    o.position = Vec3f(1.0f, 0.5f, 10.0f);
    o.rotation = Vec3f(1.0f, 1.0f, 2.0f);
    o.scale = Vec3f(2.0f);
    o.isDirty = true;
  }
}

void benchmark_objects_optimized() {
  // setup
  struct ObjectRecord {
    uint32 meshId;
    uint32 objectId;
  };

  struct PackedMesh {
    std::vector<Matrix4f> matrices;
    std::vector<TestObject> objects;
    uint32 id;
  };

  std::vector<PackedMesh*> meshes;
  std::vector<ObjectRecord> updateRecords;

  for (uint32 i = 0; i < TOTAL_MESHES; i++) {
    auto* mesh = new PackedMesh();

    mesh->matrices.resize(TOTAL_OBJECTS);
    mesh->objects.resize(TOTAL_OBJECTS);
    mesh->id = i;

    for (uint32 j = 0; j < TOTAL_OBJECTS; j++) {
      mesh->objects[j].meshId = i;
      mesh->objects[j].objectId = j;

      if (j % 100 == 0) {
        updateRecords.push_back({ i, j });
      }
    }

    meshes.push_back(mesh);
  }

  auto simulateGameUpdate = [&]() {
    for (auto& record : updateRecords) {
      auto& mesh = *meshes[record.meshId];
      auto& object = mesh.objects[record.objectId];

      object.position = Vec3f(1.0f, 0.5f, 10.0f);
      object.rotation = Vec3f(1.0f, 1.0f, 2.0f);
      object.scale = Vec3f(2.0f);

      mesh.matrices[object.objectId] = (
        Matrix4f::translate(object.position) *
        Matrix4f::rotate(object.rotation) *
        Matrix4f::scale(object.scale)
      ).transpose();
    }
  };

  // test
  Gm_RunLoopedBenchmarkTest([&]() {
    simulateGameUpdate();

    log(updateRecords.size(), "objects updated");
  });
}

void benchmark_objects_unoptimized() {
  // setup
  struct PointerMesh {
    std::vector<Matrix4f> matrices;
    std::vector<TestObject*> objects;
    uint32 id;
  };

  std::vector<PointerMesh*> meshes;
  std::vector<TestObject*> objectsToUpdate;

  for (uint32 i = 0; i < TOTAL_MESHES; i++) {
    meshes.push_back(new PointerMesh());

    auto& mesh = *meshes.back();

    mesh.matrices.resize(TOTAL_OBJECTS);
    mesh.id = i;

    for (uint32 j = 0; j < TOTAL_OBJECTS; j++) {
      auto* object = new TestObject();

      object->meshId = i;
      object->objectId = j;

      mesh.objects.push_back(object);

      if (j % 100 == 0) {
        objectsToUpdate.push_back(object);
      }
    }
  }

  // test
  Gm_RunLoopedBenchmarkTest([&]() {
    updateObjects(objectsToUpdate);

    for (uint32 i = 0; i < TOTAL_MESHES; i++) {
      auto& mesh = *meshes[i];
      auto& matrices = mesh.matrices;
      auto& objects = mesh.objects;
      
      for (uint32 j = 0; j < TOTAL_OBJECTS; j++) {
        auto& object = *objects[j];
        
        if (object.isDirty) {
          matrices[j] = (
            Matrix4f::translate(object.position) *
            Matrix4f::rotate(object.rotation) *
            Matrix4f::scale(object.scale)
          ).transpose();

          object.isDirty = false;
        }
      }
    }

    log(objectsToUpdate.size(), "objects updated");
  });
}