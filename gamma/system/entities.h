#pragma once

#include <vector>

#include "math/geometry.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "system/type_aliases.h"

namespace Gamma {
  enum LightType {
    POINT,
    DIRECTIONAL,
    SPOT
  };

  /**
   * BaseEntity
   * ----------
   *
   * Defines base attributes extensible to any dynamic
   * elements in a scene, such as Objects or Lights.
   */
  struct BaseEntity {
    /**
     * Controls how long an entity will remain in the scene
     * before being subject to removal, in milliseconds.
     * Entities with lifetimes of -1 are ignored and allowed
     * to remain in the scene indefinitely.
     */
    int lifetime = -1;
  };

  /**
   * Object
   * ------
   *
   * Objects are derived from Meshes, defining individual
   * instances of a Mesh distributed throughout a scene,
   * each with its own transformations.
   */
  struct Object : BaseEntity {
    uint32 _meshId = 0;
    uint32 _meshGeneration = 0;
    uint32 _objectId = 0;
    uint32 _matrixId = 0;
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
  };

  /**
   * Light
   * -----
   *
   * Defines a light source, which affects scene illumination
   * and color/reflective properties of illuminated surfaces.
   */
  struct Light : BaseEntity {
    Vec3f position;
    Vec3f color = Vec3f(1.0f);
    float radius = 100.0f;
    float power = 1.0f;
    LightType type = LightType::POINT;
    bool canCastShadows = false;
  };

  /**
   * Mesh
   * ----
   *
   * A Mesh serves as a static reference model from which
   * individual Objects can be created, where Objects
   * only contain transformation properties and other
   * instance-specific attributes.
   */
  struct Mesh {
    /**
     * A unique ID for the mesh.
     */
    uint32 id;
    /**
     * Determines the number of times the mesh object has
     * been recycled based on removal and reallocation of
     * meshes within a scene. Mesh lookups rely on both a
     * mesh ID and generation; if a mesh meeting these
     * criteria can't be found, the lookup will fail.
     */
    uint32 generation = 0;
    /**
     * Static mesh vertices in model space.
     */
    std::vector<Vertex> vertices;
    /**
     * Vertex indices for each triangle face of the mesh,
     * defined in groups of three.
     */
    std::vector<uint32> faceIndexes;
    /**
     * Defines the maximum number of allotted instances
     * for the mesh, determined whenever the mesh is
     * added to a scene.
     */
    uint32 maxInstances = 0;
    /**
     * An array of objects created from the mesh.
     */
    Object* objects = nullptr;
    uint32 totalActiveObjects = 0;
    /**
     * An array of transformation matrices for each object
     * created from the mesh, stored contiguously to allow
     * the data to be easily uploaded to the GPU.
     */
    Matrix4f* matrices = nullptr;
    uint32 totalActiveMatrices = 0;
  };

  /**
   * Gm_CreateCube
   * -------------
   */
  Mesh* Gm_CreateCube();

  /**
   * Gm_FreeMesh
   * -----------
   */
  void Gm_FreeMesh(Mesh* mesh);

  /**
   * Gm_LoadMesh
   * -----------
   */
  Mesh* Gm_LoadMesh(const char* path);
}