#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using xiiMeshResourceHandle     = xiiTypedResourceHandle<class xiiMeshResource>;
using xiiMaterialResourceHandle = xiiTypedResourceHandle<class xiiMaterialResource>;

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeMeshFactory final : public xiiParticleTypeFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeMeshFactory, xiiParticleTypeFactory);

public:
  virtual const xiiRTTI* GetTypeType() const override;
  virtual void           CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  xiiString m_sMesh;
  xiiString m_sMaterial;
  xiiString m_sTintColorParameter;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeMesh final : public xiiParticleType
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeMesh, xiiParticleType);

public:
  xiiParticleTypeMesh();
  ~xiiParticleTypeMesh();

  virtual void CreateRequiredStreams() override;

  xiiMeshResourceHandle             m_hMesh;
  mutable xiiMaterialResourceHandle m_hMaterial;
  xiiTempHashedString               m_sTintColorParameter;

  virtual void ExtractTypeRenderData(xiiMsgExtractRenderData& msg, const xiiTransform& instanceTransform) const override;

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
  virtual void Process(xiiUInt64 uiNumElements) override {}

  bool QueryMeshAndMaterialInfo() const;

  xiiProcessingStream* m_pStreamPosition       = nullptr;
  xiiProcessingStream* m_pStreamSize           = nullptr;
  xiiProcessingStream* m_pStreamColor          = nullptr;
  xiiProcessingStream* m_pStreamRotationSpeed  = nullptr;
  xiiProcessingStream* m_pStreamRotationOffset = nullptr;
  xiiProcessingStream* m_pStreamAxis           = nullptr;

  mutable bool                    m_bRenderDataCached = false;
  mutable xiiBoundingBoxSphere    m_Bounds;
  mutable xiiRenderData::Category m_RenderCategory;
};
