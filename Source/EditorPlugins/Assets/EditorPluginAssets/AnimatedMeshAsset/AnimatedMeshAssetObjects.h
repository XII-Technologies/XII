/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ModelImporter/MeshImport.h>
#include <SharedPluginAssets/Common/MeshMaterialSlots.h>

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

  bool m_bRecalculateNormals  = false;
  bool m_bRecalculateTangents = true;
  bool m_bNormalizeWeights    = true;
  bool m_bImportMaterials     = true;

  xiiEnum<xiiModelVertexColorConversion> m_VertexColorConversion;

  xiiHybridArray<xiiMaterialResourceSlot, 8> m_Slots;

  bool     m_bSimplifyMesh             = false;
  xiiUInt8 m_uiGeneratedLODs           = 0;
  bool     m_bAggressiveSimplification = false;
  xiiUInt8 m_uiMeshSimplification      = 50;
  xiiUInt8 m_uiMaxSimplificationError  = 5;
};
