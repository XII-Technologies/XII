#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using xiiTexture2DResourceHandle = xiiTypedResourceHandle<class xiiTexture2DResource>;
struct xiiTrailParticleData;

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeTrailFactory final : public xiiParticleTypeFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeTrailFactory, xiiParticleTypeFactory);

public:
  virtual const xiiRTTI* GetTypeType() const override;
  virtual void           CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  xiiEnum<xiiParticleTypeRenderMode>   m_RenderMode;
  xiiUInt16                            m_uiMaxPoints;
  xiiTime                              m_UpdateDiff;
  xiiString                            m_sTexture;
  xiiEnum<xiiParticleTextureAtlasType> m_TextureAtlasType;
  xiiUInt8                             m_uiNumSpritesX = 1;
  xiiUInt8                             m_uiNumSpritesY = 1;
  xiiString                            m_sTintColorParameter;
  xiiString                            m_sDistortionTexture;
  float                                m_fDistortionStrength = 0;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeTrail final : public xiiParticleType
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeTrail, xiiParticleType);

public:
  xiiParticleTypeTrail();
  ~xiiParticleTypeTrail();

  xiiEnum<xiiParticleTypeRenderMode>   m_RenderMode;
  xiiUInt16                            m_uiMaxPoints;
  xiiTime                              m_UpdateDiff;
  xiiTexture2DResourceHandle           m_hTexture;
  xiiEnum<xiiParticleTextureAtlasType> m_TextureAtlasType;
  xiiUInt8                             m_uiNumSpritesX = 1;
  xiiUInt8                             m_uiNumSpritesY = 1;
  xiiTempHashedString                  m_sTintColorParameter;
  xiiTexture2DResourceHandle           m_hDistortionTexture;
  float                                m_fDistortionStrength = 0;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const override;
  /// \todo This is a hacky guess, one would actually need to inspect the trail positions
  virtual float GetMaxParticleRadius(float fParticleSize) const override { return fParticleSize + m_uiMaxPoints * 0.05f; }

  static xiiUInt16 ComputeTrailPointBucketSize(xiiUInt16 uiMaxTrailPoints);

protected:
  friend class xiiParticleTypeTrailFactory;

  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
  virtual void Process(xiiUInt64 uiNumElements) override;
  void         OnParticleDeath(const xiiStreamGroupElementRemovedEvent& e);

  xiiProcessingStream* m_pStreamLifeTime  = nullptr;
  xiiProcessingStream* m_pStreamPosition  = nullptr;
  xiiProcessingStream* m_pStreamSize      = nullptr;
  xiiProcessingStream* m_pStreamColor     = nullptr;
  xiiProcessingStream* m_pStreamTrailData = nullptr;
  xiiProcessingStream* m_pStreamVariation = nullptr;
  xiiTime              m_LastSnapshot;
  xiiUInt8             m_uiCurFirstIndex = 0;
  float                m_fSnapshotFraction;

  mutable xiiArrayPtr<xiiBaseParticleShaderData>  m_BaseParticleData;
  mutable xiiArrayPtr<xiiTrailParticleShaderData> m_TrailParticleData;
  mutable xiiArrayPtr<xiiVec4>                    m_TrailPointsShared;

  struct TrailData
  {
    xiiUInt16 m_uiNumPoints;
    xiiUInt16 m_uiIndexForTrailPoints;
  };

  xiiUInt16      GetIndexForTrailPoints();
  const xiiVec4* GetTrailPointsPositions(xiiUInt32 index) const;
  xiiVec4*       GetTrailPointsPositions(xiiUInt32 index);

  /// \todo Use a shared freelist across effects instead
  // xiiDynamicArray<xiiTrailParticlePointsData8> m_TrailPoints8;
  // xiiDynamicArray<xiiTrailParticlePointsData16> m_TrailPoints16;
  // xiiDynamicArray<xiiTrailParticlePointsData32> m_TrailPoints32;
  xiiDynamicArray<xiiTrailParticlePointsData64, xiiAlignedAllocatorWrapper> m_TrailPoints64;
  xiiDynamicArray<xiiUInt16>                                                m_FreeTrailData;
};
