#pragma once

#include <EditorPluginAssets/Util/AssetUtils.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>

class xiiAnimatedMeshAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimatedMeshAssetProperties, xiiReflectedClass);

public:
  xiiAnimatedMeshAssetProperties();
  ~xiiAnimatedMeshAssetProperties();

  xiiString m_sMeshFile;
  xiiString m_sDefaultSkeleton;

  bool m_bRecalculateNormals   = false;
  bool m_bRecalculateTrangents = true;
  bool m_bNormalizeWeights     = false;
  bool m_bImportMaterials      = true;

  xiiEnum<xiiMeshNormalPrecision>     m_NormalPrecision;
  xiiEnum<xiiMeshTexCoordPrecision>   m_TexCoordPrecision;
  xiiEnum<xiiMeshBoneWeigthPrecision> m_BoneWeightPrecision;

  xiiHybridArray<xiiMaterialResourceSlot, 8> m_Slots;
};
