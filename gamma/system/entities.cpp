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

    return mesh;
  }

  /**
   * Gm_LoadMesh
   * -----------
   */
  Mesh* Gm_LoadMesh(const char* path) {
    // @TODO load .obj model files
    return new Mesh();
  }

  /**
   * Gm_FreeMesh
   * -----------
   */
  void Gm_FreeMesh(Mesh* mesh) {
    delete mesh;
  }

  /**
   * Gm_RecomputeObjectMatrix
   * ------------------------
   */
  void Gm_RecomputeObjectMatrix(Object* object) {
    // @TODO recompute reference Mesh matrix entry for the object
  }
}