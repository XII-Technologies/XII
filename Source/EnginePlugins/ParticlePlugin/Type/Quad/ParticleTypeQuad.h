#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using xiiTexture2DResourceHandle = xiiTypedResourceHandle<class xiiTexture2DResource>;

struct XII_PARTICLEPLUGIN_DLL xiiQuadParticleOrientation
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Billboard,

    Rotating_OrthoEmitterDir,
    Rotating_EmitterDir,

    Fixed_EmitterDir,
    Fixed_WorldUp,
    Fixed_RandomDir,

    FixedAxis_EmitterDir,
    FixedAxis_ParticleDir,

    Default = Billboard
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PARTICLEPLUGIN_DLL, xiiQuadParticleOrientation);

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeQuadFactory final : public xiiParticleTypeFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeQuadFactory, xiiParticleTypeFactory);

public:
  virtual const xiiRTTI* GetTypeType() const override;
  virtual void           CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const override;

  xiiEnum<xiiQuadParticleOrientation>  m_Orientation;
  xiiAngle                             m_MaxDeviation;
  xiiEnum<xiiParticleTypeRenderMode>   m_RenderMode;
  xiiString                            m_sTexture;
  xiiEnum<xiiParticleTextureAtlasType> m_TextureAtlasType;
  xiiUInt8                             m_uiNumSpritesX = 1;
  xiiUInt8                             m_uiNumSpritesY = 1;
  xiiString                            m_sTintColorParameter;
  xiiString                            m_sDistortionTexture;
  float                                m_fDistortionStrength = 0;
  float                                m_fStretch            = 1;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeQuad final : public xiiParticleType
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeQuad, xiiParticleType);

public:
  xiiParticleTypeQuad();
  ~xiiParticleTypeQuad();

  virtual void CreateRequiredStreams() override;

  xiiEnum<xiiQuadParticleOrientation>  m_Orientation;
  xiiAngle                             m_MaxDeviation;
  xiiEnum<xiiParticleTypeRenderMode>   m_RenderMode;
  xiiTexture2DResourceHandle           m_hTexture;
  xiiEnum<xiiParticleTextureAtlasType> m_TextureAtlasType;
  xiiUInt8                             m_uiNumSpritesX = 1;
  xiiUInt8                             m_uiNumSpritesY = 1;
  xiiTempHashedString                  m_sTintColorParameter;
  xiiTexture2DResourceHandle           m_hDistortionTexture;
  float                                m_fDistortionStrength = 0;
  float                                m_fStretch            = 1;

  virtual void ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const override;

  struct sod
  {
    XII_DECLARE_POD_TYPE();

    float     dist;
    xiiUInt32 index;
  };


protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
  virtual void Process(xiiUInt64 uiNumElements) override {}
  void         AllocateParticleData(const xiiUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData) const;
  void         AddParticleRenderData(xiiMsgExtractRenderData& msg, const xiiTransform& instanceTransform) const;
  void         CreateExtractedData(const xiiHybridArray<sod, 64>* pSorted) const;

  xiiProcessingStream* m_pStreamLifeTime       = nullptr;
  xiiProcessingStream* m_pStreamPosition       = nullptr;
  xiiProcessingStream* m_pStreamSize           = nullptr;
  xiiProcessingStream* m_pStreamColor          = nullptr;
  xiiProcessingStream* m_pStreamRotationSpeed  = nullptr;
  xiiProcessingStream* m_pStreamRotationOffset = nullptr;
  xiiProcessingStream* m_pStreamAxis           = nullptr;
  xiiProcessingStream* m_pStreamVariation      = nullptr;
  xiiProcessingStream* m_pStreamLastPosition   = nullptr;

  mutable xiiArrayPtr<xiiBaseParticleShaderData>          m_BaseParticleData;
  mutable xiiArrayPtr<xiiBillboardQuadParticleShaderData> m_BillboardParticleData;
  mutable xiiArrayPtr<xiiTangentQuadParticleShaderData>   m_TangentParticleData;
};
