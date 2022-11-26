#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct xiiKrautAssetMaterial
{
  xiiString m_sLabel;
  xiiString m_sMaterial;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiKrautAssetMaterial);

class xiiKrautTreeAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiKrautTreeAssetProperties, xiiReflectedClass);

public:
  xiiKrautTreeAssetProperties();
  ~xiiKrautTreeAssetProperties();

  xiiString m_sKrautFile;
  float     m_fUniformScaling       = 1.0f;
  float     m_fLodDistanceScale     = 1.0f;
  float     m_fStaticColliderRadius = 0.4f;
  float     m_fTreeStiffness        = 10.0f;
  xiiString m_sSurface;

  xiiHybridArray<xiiKrautAssetMaterial, 8> m_Materials;

  xiiUInt16 m_uiRandomSeedForDisplay = 0;

  xiiHybridArray<xiiUInt16, 16> m_GoodRandomSeeds;
};
