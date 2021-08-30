#include <cstdio>
#include <cstdlib>
#include <map>
#include <utility>

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

  uint16 ObjectPool::partitionByDistance(uint16 start, float distance, const Vec3f& cameraPosition) {
    uint16 current = start;
    uint16 end = total() - 1;

    while (end > current) {
      float currentObjectDistance = (objects[current].position - cameraPosition).magnitude();

      if (currentObjectDistance <= distance) {
        current++;
      } else {
        float endObjectDistance;

        do {
          endObjectDistance = (objects[end].position - cameraPosition).magnitude();
        } while (endObjectDistance > distance && --end > current);

        if (current != end) {
          swapObjects(current, end);
        }

        // @optimize we can increment 'current' here if endObjectDistance < distance
        // to avoid recalculating the same distance on the next while loop cycle
        // @todo test this out and make sure it works
      }
    }

    return current;
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

  void ObjectPool::swapObjects(uint16 indexA, uint16 indexB) {
    Object objectA = objects[indexA];
    Matrix4f matrixA = matrices[indexA];

    objects[indexA] = objects[indexB];
    matrices[indexA] = matrices[indexB];

    objects[indexB] = objectA;
    matrices[indexB] = matrixA;

    indices[objects[indexA]._record.id] = indexA;
    indices[objects[indexB]._record.id] = indexB;
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
    auto& faceElements = mesh->faceElements;

    for (uint32 i = 0; i < faceElements.size(); i += 3) {
      Vertex& v1 = vertices[faceElements[i]];
      Vertex& v2 = vertices[faceElements[i + 1]];
      Vertex& v3 = vertices[faceElements[i + 2]];

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
    auto& faceElements = mesh->faceElements;

    for (uint32 i = 0; i < faceElements.size(); i += 3) {
      Vertex& v1 = vertices[faceElements[i]];
      Vertex& v2 = vertices[faceElements[i + 1]];
      Vertex& v3 = vertices[faceElements[i + 2]];

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
    auto& faceElements = mesh->faceElements;

    vertices.resize(24);
    faceElements.resize(36);

    // For each cube side
    for (uint8 i = 0; i < 6; i++) {
      auto& face = cubeFaces[i];
      uint32 f_offset = i * 6;
      uint32 v_offset = i * 4;

      // Define vertex indexes for the two triangle faces on each cube side
      faceElements[f_offset] = v_offset;
      faceElements[f_offset + 1] = v_offset + 1;
      faceElements[f_offset + 2] = v_offset + 2;

      faceElements[f_offset + 3] = v_offset;
      faceElements[f_offset + 4] = v_offset + 2;
      faceElements[f_offset + 5] = v_offset + 3;

      // For each corner on this side
      for (uint8 j = 0; j < 4; j++) {
        auto& vertex = vertices[v_offset++];

        // Define the corner vertex position/uvs
        vertex.position = cubeCornerPositions[face[j]];
        vertex.uv = cubeUvs[j];
      }
    }

    Gm_ComputeNormals(mesh);
    Gm_ComputeTangents(mesh);

    return mesh;
  }

  /**
   * Gm_CreatePlane
   * --------------
   */
  Mesh* Gm_CreatePlane(uint32 size) {
    auto* mesh = new Mesh();
    auto& vertices = mesh->vertices;
    auto& faceElements = mesh->faceElements;

    for (uint32 x = 0; x < size; x++) {
      for (uint32 z = 0; z < size; z++) {
        Vertex vertex;

        float u = (float)x / (float)(size - 1);
        float v = (float)z / (float)(size - 1);

        vertex.position = Vec3f(u - 0.5f, 0.0f, -v + 0.5f);
        vertex.uv = Vec2f(u, 1.0f - v);

        vertices.push_back(vertex);
      }
    }

    for (uint32 z = 0; z < size - 1; z++) {
      for (uint32 x = 0; x < size - 1; x++) {
        uint32 offset = z * size + x;

        faceElements.push_back(offset);
        faceElements.push_back(offset + 1 + size);
        faceElements.push_back(offset + 1);

        faceElements.push_back(offset);
        faceElements.push_back(offset + size);
        faceElements.push_back(offset + 1 + size);
      }
    }

    Gm_ComputeNormals(mesh);
    Gm_ComputeTangents(mesh);

    return mesh;
  }

  /**
   * Gm_BufferObjData
   * ----------------
   *
   * @todo description
   *
   * @todo we may not want to add the base vertex offset here;
   * once this is used to pack multiple (distinct, not merely LOD)
   * meshes into a common vertex/element buffer, it may be preferable
   * to associate a baseVertex/firstIndex with each mesh. firstIndex
   * alone is technically feasible though. reconsider when revisiting
   * this for glMultiDrawElementsIndirect().
   */
  void Gm_BufferObjData(const ObjLoader& obj, std::vector<Vertex>& vertices, std::vector<uint32>& faceElements) {
    uint32 baseVertex = vertices.size();

    if (obj.textureCoordinates.size() == 0) {
      // No texture coordinates defined in the model file,
      // so we can load the vertices/face elements directly
      for (uint32 i = 0; i < obj.vertices.size(); i++) {
        Vertex vertex;
        vertex.position = obj.vertices[i];

        vertices.push_back(vertex);
      }

      for (uint32 i = 0; i < obj.faces.size(); i++) {
        faceElements.push_back(baseVertex + obj.faces[i].v1.vertexIndex);
        faceElements.push_back(baseVertex + obj.faces[i].v2.vertexIndex);
        faceElements.push_back(baseVertex + obj.faces[i].v3.vertexIndex);
      }
    } else {
      // Texture coordinates defined, so we need to create
      // vertices by unique position/uv pairs, and add face
      // elements based on created vertices
      typedef std::pair<uint32, uint32> VertexPair;

      std::map<VertexPair, uint32> pairToVertexIndexMap;

      for (const auto& face : obj.faces) {
        VertexPair pairs[3] = {
          { face.v1.vertexIndex, face.v1.textureCoordinateIndex },
          { face.v2.vertexIndex, face.v2.textureCoordinateIndex },
          { face.v3.vertexIndex, face.v3.textureCoordinateIndex }
        };

        // Add face elements, creating vertices if necessary
        for (uint32 p = 0; p < 3; p++) {
          auto& pair = pairs[p];
          auto indexRecord = pairToVertexIndexMap.find(pair);

          if (indexRecord != pairToVertexIndexMap.end()) {
            // Vertex already exists, so we can just add the face element
            faceElements.push_back(indexRecord->second);
          } else {
            // Vertex doesn't exist, so we need to create it
            Vertex vertex;
            uint32 index = vertices.size();

            vertex.position = obj.vertices[pair.first];
            // @todo see if uv.y needs to be inverted
            vertex.uv = obj.textureCoordinates[pair.second];
            
            vertices.push_back(vertex);
            faceElements.push_back(index);

            pairToVertexIndexMap.emplace(pair, index);
          }
        }
      }
    }
  }

  /**
   * Gm_LoadMesh
   * -----------
   *
   * @todo description
   */
  Mesh* Gm_LoadMesh(const char* path) {
    ObjLoader obj(path);

    auto* mesh = new Mesh();

    Gm_BufferObjData(obj, mesh->vertices, mesh->faceElements);
    Gm_ComputeNormals(mesh);
    Gm_ComputeTangents(mesh);

    return mesh;
  }

  /**
   * Gm_LoadMesh
   * -----------
   *
   * @todo description
   */
  Mesh* Gm_LoadMesh(std::initializer_list<const char*> paths) {
    auto* mesh = new Mesh();

    mesh->lods.resize(paths.size());

    for (uint32 i = 0; i < paths.size(); i++) {
      const char* path = *(paths.begin() + i);

      ObjLoader obj(path);

      mesh->lods[i].elementOffset = mesh->faceElements.size();

      Gm_BufferObjData(obj, mesh->vertices, mesh->faceElements);

      mesh->lods[i].elementCount = mesh->faceElements.size() - mesh->lods[i].elementOffset;

      // @todo (?) For now, the vertex offset is always 0,
      // since the offset for each set of LoD vertices is
      // already added to the element indexes. It may be
      // necessary to revisit this when more geometry is
      // packed into global vertex/element buffers. Normals
      // and tangents are computed for each triplet of face
      // elements, where those elements refer to vertex
      // indexes without offsets considered. Eventually we
      // might want to generate vertices + face elements
      // locally, use the face elements to compute normals/
      // tangents for those local vertices, and finally
      // buffer the results into a global list, defining
      // a vertex offset for the new set.
      mesh->lods[i].vertexOffset = 0;
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
    mesh->faceElements.clear();
    mesh->objects.free();
  }
}