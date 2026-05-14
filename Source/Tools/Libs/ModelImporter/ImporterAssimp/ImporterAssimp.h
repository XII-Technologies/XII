/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ModelImporter/Importer/Importer.h>

#include <assimp/Importer.hpp>

class xiiEditableSkeletonJoint;
struct aiNode;
struct aiMesh;

namespace xiiModelImporter
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

    xiiResult ImportBoneColliders(xiiEditableSkeletonJoint* pJoint);

    void SimplifyAiMesh(aiMesh* pMesh);

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

    xiiSet<aiMesh*> m_OptimizedMeshes;
  };

  extern xiiColor ConvertAssimpType(const aiColor3D& value, bool bInvert = false);
  extern xiiColor ConvertAssimpType(const aiColor4D& value, bool bInvert = false);
  extern xiiMat4  ConvertAssimpType(const aiMatrix4x4& value, bool bDummy = false);
  extern xiiVec3  ConvertAssimpType(const aiVector3D& value, bool bDummy = false);
  extern xiiQuat  ConvertAssimpType(const aiQuaternion& value, bool bDummy = false);
  extern float    ConvertAssimpType(float value, bool bDummy = false);
  extern int      ConvertAssimpType(int value, bool bDummy = false);

} // namespace xiiModelImporter
