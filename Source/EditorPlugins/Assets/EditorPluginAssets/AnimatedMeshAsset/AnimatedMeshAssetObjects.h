/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginAssets/Util/AssetUtils.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>

struct xiiPropertyMetaStateEvent;

class xiiAnimatedMeshAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimatedMeshAssetProperties, xiiReflectedClass);

public:
  xiiAnimatedMeshAssetProperties();
  ~xiiAnimatedMeshAssetProperties();

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  xiiString m_sMeshFile;
  xiiString m_sDefaultSkeleton;

  bool m_bRecalculateNormals   = false;
  bool m_bRecalculateTrangents = true;
  bool m_bNormalizeWeights     = false;
  bool m_bImportMaterials      = true;

  xiiEnum<xiiMeshNormalPrecision>       m_NormalPrecision;
  xiiEnum<xiiMeshTexCoordPrecision>     m_TexCoordPrecision;
  xiiEnum<xiiMeshBoneWeigthPrecision>   m_BoneWeightPrecision;
  xiiEnum<xiiMeshVertexColorConversion> m_VertexColorConversion;

  xiiHybridArray<xiiMaterialResourceSlot, 8> m_Slots;

  bool     m_bSimplifyMesh             = false;
  bool     m_bAggressiveSimplification = false;
  xiiUInt8 m_uiMeshSimplification      = 50;
  xiiUInt8 m_uiMaxSimplificationError  = 5;
};
