#include <cstdio>
#include <cstdlib>

#include "math/vector.h"
#include "system/assert.h"
#include "system/entities.h"
#include "system/ObjLoader.h"

#define UNUSED_OBJECT_INDEX USHRT_MAX

namespace Gamma {
  /**
   * Defines positions for each corner of the unit cube
   */
  const static Vec3f cubeCornerPositions[8] = {
    { -1.0f, -1.0f, -1.0f },  // rear side, bottom left
    { 1.0f, -1.0f, -1.0f },   // rear side, bottom right
    { 1.0f, 1.0f, -1.0f },    // rear side, top right
    { -1.0f, 1.0f, -1.0f },   // rear side, top left
    { -1.0f, -1.0f, 1.0f },   // far side, bottom left
    { 1.0f, -1.0f, 1.0f },    // far side, bottom right
    { 1.0f, 1.0f, 1.0f },     // far side, top right
    { -1.0f, 1.0f, 1.0f }     // far side, top left
  };

  /**
   * Defines UV coordinates for each cube side
   */
  const static Vec2f cubeUvs[4] = {
    { 1.0f, 1.0f },           // bottom right
    { 0.0f, 1.0f },           // bottom left
    { 0.0f, 0.0f },           // top left
    { 1.0f, 0.0f }            // top right
  };

  /**
   * Maps the corners of each cube side to corner position indexes
   */
  const static int cubeFaces[6][4] = {
    { 1, 0, 3, 2 },           // back
    { 7, 6, 2, 3 },           // top
    { 4, 5, 6, 7 },           // front
    { 0, 1, 5, 4 },           // bottom
    { 0, 4, 7, 3 },           // left
    { 5, 1, 2, 6 }            // right
  };

  /**
   * ObjectPool
   * ----------
   */
  Object* ObjectPool::begin() const {
    return objects;
  }

  Object& ObjectPool::createObject() {
    uint16 id = runningId++;

    assert(max() > total(), "Object Pool out of space: " + std::to_string(max()) + " objects allowed in this pool");
    assert(indices[id] == UNUSED_OBJECT_INDEX, "Attempted to create an Object in an occupied slot");

    // Retrieve and initialize object
    uint16 index = totalActiveObjects;
    Object& object = objects[index];

    object._record.id = id;
    object._record.generation++;

    // Reset object matrix
    matrices[index] = Matrix4f::identity();

    // Enable object lookup by ID -> index
    indices[id] = index;

    totalActiveObjects++;

    return object;
  }

  Object* ObjectPool::end() const {
    return &objects[totalActiveObjects];
  }

  void ObjectPool::free() {
    for (uint16 i = 0; i < USHRT_MAX; i++) {
      indices[i] = UNUSED_OBJECT_INDEX;
    }

    if (objects != nullptr) {
      delete[] objects;
    }

    if (matrices != nullptr) {
      delete[] matrices;
    }

    objects = nullptr;
    matrices = nullptr;
  }

  Object* ObjectPool::getById(uint16 objectId) const {
    uint16 index = indices[objectId];

    return index == UNUSED_OBJECT_INDEX ? nullptr : &objects[index];
  }

  Object* ObjectPool::getByRecord(const ObjectRecord& record) const {
    auto* object = getById(record.id);

    if (object == nullptr || object->_record.generation != record.generation) {
      return nullptr;
    }

    return object;
  }

  Matrix4f* ObjectPool::getMatrices() const {
    return matrices;
  }

  uint16 ObjectPool::max() const {
    return maxObjects;
  }

  void ObjectPool::removeById(uint16 objectId) {
    uint16 index = indices[objectId];

    if (index == UNUSED_OBJECT_INDEX) {
      return;
    }

    totalActiveObjects--;

    uint16 lastIndex = totalActiveObjects;

    // Swap last object/matrix into removed index
    objects[index] = objects[lastIndex];
    matrices[index] = matrices[lastIndex];

    // Update ID -> index lookup table
    indices[objects[index]._record.id] = index;
    indices[objectId] = UNUSED_OBJECT_INDEX;
  }

  void ObjectPool::reserve(uint16 size) {
    free();

    maxObjects = size;
    totalActiveObjects = 0;
    objects = new Object[size];
    matrices = new Matrix4f[size];
  }

  uint16 ObjectPool::total() const {
    return totalActiveObjects;
  }

  void ObjectPool::transformById(uint16 objectId, const Matrix4f& matrix) {
    matrices[indices[objectId]] = matrix;
  }

  /**
   * Gm_ComputeNormals
   * -----------------
   */
  void Gm_ComputeNormals(Mesh* mesh) {
    auto& vertices = mesh->vertices;
    auto& faceIndexes = mesh->faceIndexes;

    for (uint32 i = 0; i < faceIndexes.size(); i += 3) {
      Vertex& v1 = vertices[faceIndexes[i]];
      Vertex& v2 = vertices[faceIndexes[i + 1]];
      Vertex& v3 = vertices[faceIndexes[i + 2]];

      Vec3f normal = Vec3f::cross(v2.position - v1.position, v3.position - v1.position).unit();

      v1.normal += normal;
      v2.normal += normal;
      v3.normal += normal;
    }
  }

  /**
   * Gm_ComputeTangents
   * ------------------
   */
  void Gm_ComputeTangents(Mesh* mesh) {
    auto& vertices = mesh->vertices;
    auto& faceIndexes = mesh->faceIndexes;

    for (uint32 i = 0; i < faceIndexes.size(); i += 3) {
      Vertex& v1 = vertices[faceIndexes[i]];
      Vertex& v2 = vertices[faceIndexes[i + 1]];
      Vertex& v3 = vertices[faceIndexes[i + 2]];

      Vec3f e1 = v2.position - v1.position;
      Vec3f e2 = v3.position - v1.position;

      float deltaU1 = v2.uv.x - v1.uv.x;
      float deltaV1 = v2.uv.y - v1.uv.y;
      float deltaU2 = v3.uv.x - v1.uv.x;
      float deltaV2 = v3.uv.y - v1.uv.y;

      float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

      Vec3f tangent = {
        f * (deltaV2 * e1.x - deltaV1 * e2.x),
        f * (deltaV2 * e1.y - deltaV1 * e2.y),
        f * (deltaV2 * e1.z - deltaV1 * e2.z)
      };

      v1.tangent += tangent;
      v2.tangent += tangent;
      v3.tangent += tangent;
    }
  }

  /**
   * Gm_CreateCube
   * -------------
   *
   * Constructs a cube Mesh using predefined vertex data.
   * None of a Cube's vertices are shared between its sides,
   * ensuring that normals remain constant along them.
   */
  Mesh* Gm_CreateCube() {
    auto* mesh = new Mesh();
    auto& vertices = mesh->vertices;
    auto& faceIndexes = mesh->faceIndexes;

    vertices.resize(24);
    faceIndexes.resize(36);

    // for each cube side
    for (uint8 i = 0; i < 6; i++) {
      auto& face = cubeFaces[i];
      uint32 f_offset = i * 6;
      uint32 v_offset = i * 4;

      // define vertex indexes for the two triangle faces on each cube side
      faceIndexes[f_offset] = v_offset;
      faceIndexes[f_offset + 1] = v_offset + 1;
      faceIndexes[f_offset + 2] = v_offset + 2;

      faceIndexes[f_offset + 3] = v_offset;
      faceIndexes[f_offset + 4] = v_offset + 2;
      faceIndexes[f_offset + 5] = v_offset + 3;

      // for each corner on this side
      for (uint8 j = 0; j < 4; j++) {
        auto& vertex = vertices[v_offset++];

        // define the corner vertex position/uvs
        vertex.position = cubeCornerPositions[face[j]];
        vertex.uv = cubeUvs[j];
      }
    }

    Gm_ComputeNormals(mesh);
    Gm_ComputeTangents(mesh);

    return mesh;
  }

  /**
   * Gm_FreeMesh
   * -----------
   */
  void Gm_FreeMesh(Mesh* mesh) {
    mesh->vertices.clear();
    mesh->faceIndexes.clear();
    mesh->objects.free();
  }

  /**
   * Gm_LoadMesh
   * -----------
   */
  Mesh* Gm_LoadMesh(const char* path) {
    ObjLoader obj(path);

    auto* mesh = new Mesh();
    auto& vertices = mesh->vertices;
    auto& faceIndexes = mesh->faceIndexes;

    vertices.resize(obj.vertices.size());
    faceIndexes.resize(obj.faces.size() * 3);

    for (uint32 i = 0; i < vertices.size(); i++) {
      vertices[i].position = obj.vertices[i];
      // @todo resolve uv coordinates
    }

    for (uint32 i = 0; i < obj.faces.size(); i++) {
      faceIndexes[i * 3] = obj.faces[i].v1.vertexIndex;
      faceIndexes[i * 3 + 1] = obj.faces[i].v2.vertexIndex;
      faceIndexes[i * 3 + 2] = obj.faces[i].v3.vertexIndex;
    }

    Gm_ComputeNormals(mesh);
    Gm_ComputeTangents(mesh);

    return mesh;
  }
}