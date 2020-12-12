#include <cstdio>
#include <cstdlib>

#include "system/entities.h"

namespace Gamma {
  /**
   * Gm_CreatePrimitiveMesh
   * ----------------------
   */
  Mesh* Gm_CreatePrimitiveMesh(Primitive primitive) {
    return new Mesh();
  }

  /**
   * Gm_LoadMesh
   * -----------
   */
  Mesh* Gm_LoadMesh(const char* path) {
    // @TODO malloc()?
    return new Mesh();
  }

  /**
   * Gm_FreeMesh
   * -----------
   */
  void Gm_FreeMesh(Mesh* mesh) {
    // @TODO free()?
    delete mesh;
  }

  /**
   * Gm_RecomputeObjectMatrix
   * ------------------------
   */
  void Gm_RecomputeObjectMatrix(Object* object) {
    // @TODO
  }
}