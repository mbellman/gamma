#include <cstdio>
#include <cstdlib>

#include "math/vector.h"
#include "system/entities.h"

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

    if (mesh->objects != nullptr) {
      delete[] mesh->objects;
    }

    if (mesh->matrices != nullptr) {
      delete[] mesh->matrices;
    }

    mesh->objects = nullptr;
    mesh->matrices = nullptr;
    mesh->totalActiveMatrices = 0;
    mesh->totalActiveObjects = 0;
    mesh->maxInstances = 0;
  }

  /**
   * Gm_LoadMesh
   * -----------
   */
  Mesh* Gm_LoadMesh(const char* path) {
    // @TODO load .obj model files
    return new Mesh();
  }
}