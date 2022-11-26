#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class xiiView;
class xiiExtractedRenderData;

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeLightFactory final : public xiiParticleTypeFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeLightFactory, xiiParticleTypeFactory);

public:
  xiiParticleTypeLightFactory();

  virtual const xiiRTTI* GetTypeType() const override;
  virtual void           CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  float     m_fSizeFactor;
  float     m_fIntensity;
  xiiUInt32 m_uiPercentage;
  xiiString m_sTintColorParameter;
  xiiString m_sIntensityParameter;
  xiiString m_sSizeScaleParameter;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeLight final : public xiiParticleType
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeLight, xiiParticleType);

public:
  virtual void CreateRequiredStreams() override;

  float               m_fSizeFactor;
  float               m_fIntensity;
  xiiUInt32           m_uiPercentage;
  xiiTempHashedString m_sTintColorParameter;
  xiiTempHashedString m_sIntensityParameter;
  xiiTempHashedString m_sSizeScaleParameter;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.5f * fParticleSize * m_fSizeFactor; }

  virtual void ExtractTypeRenderData(xiiMsgExtractRenderData& msg, const xiiTransform& instanceTransform) const override;

protected:
  virtual void Process(xiiUInt64 uiNumElements) override {}

  xiiProcessingStream* m_pStreamPosition;
  xiiProcessingStream* m_pStreamSize;
  xiiProcessingStream* m_pStreamColor;
  xiiProcessingStream* m_pStreamOnOff;
};
