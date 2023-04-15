#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleTypePointFactory final : public xiiParticleTypeFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypePointFactory, xiiParticleTypeFactory);

public:
  virtual const xiiRTTI* GetTypeType() const override;
  virtual void           CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleTypePoint final : public xiiParticleType
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypePoint, xiiParticleType);

public:
  xiiParticleTypePoint() {}

  virtual void CreateRequiredStreams() override;

  virtual void ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const override;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.0f; }

protected:
  virtual void Process(xiiUInt64 uiNumElements) override {}

  xiiProcessingStream* m_pStreamPosition;
  xiiProcessingStream* m_pStreamColor;

  mutable xiiArrayPtr<xiiBaseParticleShaderData>          m_BaseParticleData;
  mutable xiiArrayPtr<xiiBillboardQuadParticleShaderData> m_BillboardParticleData;
};
