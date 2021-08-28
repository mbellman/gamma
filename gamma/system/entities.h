#pragma once

#include <vector>
#include <string>

#include "math/geometry.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "system/ObjLoader.h"
#include "system/type_aliases.h"

#define UNUSED_LIGHT_INDEX -1

namespace Gamma {
  enum LightType {
    POINT,
    DIRECTIONAL,
    SPOT,
    POINT_SHADOWCASTER,
    DIRECTIONAL_SHADOWCASTER,
    SPOT_SHADOWCASTER
  };

  /**
   * @todo description
   */
  enum MeshType {
    /**
     * @todo description
     */
    EMISSIVE = 0x00,
    /**
     * @todo description
     */
    REFRACTIVE = 0xF0,
    /**
     * @todo description
     */
    REFLECTIVE = 0xFA,
    /**
     * @todo description
     */
    NON_EMISSIVE = 0xFF
  };

  /**
   * ObjectRecord
   * ------------
   *
   * A unique identifier for an object which allows it
   * to be looked up on the appropriate Mesh and at its
   * corresponding index, with ID checks for referential
   * integrity.
   */
  struct ObjectRecord {
    uint16 meshIndex = 0;
    uint16 meshId = 0;
    uint16 id = 0;
    uint16 generation = 0;
  };

  /**
   * Object
   * ------
   *
   * Objects are derived from Meshes, defining individual
   * instances of a Mesh distributed throughout a scene,
   * each with its own transformations.
   */
  struct Object {
    ObjectRecord _record;
    Vec3f position;
    Vec3f scale;
    Vec3f rotation;
  };

  /**
   * ObjectPool
   * ----------
   *
   * @todo description
   */
  class ObjectPool {
  public:
    Object* begin() const;
    Object& createObject();
    Object* end() const;
    void free();
    Object* getById(uint16 objectId) const;
    Object* getByRecord(const ObjectRecord& record) const;
    Matrix4f* getMatrices() const;
    uint16 max() const;
    void removeById(uint16 objectId);
    void reserve(uint16 size);
    uint16 total() const;
    void transformById(uint16 objectId, const Matrix4f& matrix);

  private:
    Object* objects = nullptr;
    Matrix4f* matrices = nullptr;
    uint16 indices[USHRT_MAX];
    uint16 maxObjects = 0;
    uint16 totalActiveObjects = 0;
    uint16 runningId = 0;
  };

  /**
   * Light
   * -----
   *
   * Defines a light source, which affects scene illumination
   * and color/reflective properties of illuminated surfaces.
   */
  struct Light {
    Vec3f position;
    float radius = 100.0f;
    Vec3f color = Vec3f(1.0f);
    float power = 1.0f;
    Vec3f direction;
    uint32 type = LightType::POINT;
    int id = UNUSED_LIGHT_INDEX;
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
     * The index of the mesh in a scene's Mesh array,
     * used for efficient mesh lookups.
     */
    uint16 index = 0;
    /**
     * A unique ID for the mesh. If a mesh retrieved
     * at a specific index in a scene's Mesh array
     * does not match an expected ID (e.g., if the
     * mesh structure has been recycled), the reference
     * should be considered stale.
     */
    uint16 id = 0;
    /**
     * Static mesh vertices in model space.
     */
    std::vector<Vertex> vertices;
    /**
     * Vertex indices for each triangle face of the mesh,
     * defined in groups of three.
     */
    std::vector<uint32> faceElements;
    /**
     * @todo description
     */
    std::vector<uint32> firstIndexOffsets;
    /**
     * A collection of objects representing unique instances
     * of the mesh.
     */
    ObjectPool objects;
    /**
     * An optional albedo texture for the mesh.
     */
    std::string texture = "";
    /**
     * An optional normal map texture for the mesh.
     */
    std::string normalMap = "";
    /**
     * An optional specularity map texture for the mesh.
     */
    std::string specularityMap = "";
    /**
     * Defines the mesh type.
     *
     * @see MeshType
     */
    uint8 type = MeshType::NON_EMISSIVE;
    /**
     * @todo description
     */
    uint8 maxCascade = 3;
    /**
     * Controls whether the mesh's instances are rendered
     * to shadow maps, enabling them to cast shadows.
     */
    bool canCastShadows = true;
  };

  /**
   * Gm_ComputeNormals
   * -----------------
   */
  void Gm_ComputeNormals(Mesh* mesh);

  /**
   * Gm_ComputeTangents
   * ------------------
   */
  void Gm_ComputeTangents(Mesh* mesh);

  /**
   * Gm_CreateCube
   * -------------
   */
  Mesh* Gm_CreateCube();

  /**
   * Gm_CreatePlane
   * --------------
   */
  Mesh* Gm_CreatePlane(uint32 size);

  /**
   * Gm_BufferObjData
   * ----------------
   */
  void Gm_BufferObjData(const ObjLoader& obj, std::vector<Vertex>& vertices, std::vector<uint32>& faceElements);

  /**
   * Gm_LoadMesh
   * -----------
   */
  Mesh* Gm_LoadMesh(const char* path);
  Mesh* Gm_LoadMesh(std::initializer_list<const char*> paths);

  /**
   * Gm_FreeMesh
   * -----------
   */
  void Gm_FreeMesh(Mesh* mesh);
}