#pragma once

#include <ModelImporter2/Importer/Importer.h>

#include <assimp/Importer.hpp>

class xiiEditableSkeletonJoint;
struct aiNode;
struct aiMesh;

namespace xiiModelImporter2
{
  class ImporterAssimp : public Importer
  {
  public:
    ImporterAssimp();
    ~ImporterAssimp();

  protected:
    virtual xiiResult DoImport() override;

  private:
    xiiResult TraverseAiScene();

    xiiResult PrepareOutputMesh();
    xiiResult RecomputeTangents();

    xiiResult TraverseAiNode(aiNode* pNode, const xiiMat4& parentTransform, xiiEditableSkeletonJoint* pCurJoint);
    xiiResult ProcessAiMesh(aiMesh* pMesh, const xiiMat4& transform);

    xiiResult ImportMaterials();
    xiiResult ImportAnimations();

    Assimp::Importer m_Importer;
    const aiScene*   m_pScene               = nullptr;
    xiiUInt32        m_uiTotalMeshVertices  = 0;
    xiiUInt32        m_uiTotalMeshTriangles = 0;

    struct MeshInstance
    {
      xiiMat4 m_GlobalTransform;
      aiMesh* m_pMesh;
    };

    xiiMap<xiiUInt32, xiiHybridArray<MeshInstance, 4>> m_MeshInstances;
  };

  extern xiiColor ConvertAssimpType(const aiColor3D& value, bool invert = false);
  extern xiiColor ConvertAssimpType(const aiColor4D& value, bool invert = false);
  extern xiiMat4  ConvertAssimpType(const aiMatrix4x4& value, bool dummy = false);
  extern xiiVec3  ConvertAssimpType(const aiVector3D& value, bool dummy = false);
  extern xiiQuat  ConvertAssimpType(const aiQuaternion& value, bool dummy = false);
  extern float    ConvertAssimpType(float value, bool dummy = false);
  extern int      ConvertAssimpType(int value, bool dummy = false);

} // namespace xiiModelImporter2
