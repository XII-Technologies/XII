#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_SizeCurve final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_SizeCurve, xiiParticleBehaviorFactory);

public:
  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  void        SetSizeCurveFile(const char* szFile);
  const char* GetSizeCurveFile() const;

  float                    m_fBaseSize;
  float                    m_fCurveScale;
  xiiCurve1DResourceHandle m_hCurve;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_SizeCurve final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_SizeCurve, xiiParticleBehavior);

public:
  float                    m_fBaseSize;
  float                    m_fCurveScale;
  xiiCurve1DResourceHandle m_hCurve;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
  virtual void Process(xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamLifeTime         = nullptr;
  xiiProcessingStream* m_pStreamSize             = nullptr;
  xiiUInt8             m_uiFirstToUpdate         = 0;
  xiiUInt8             m_uiCurrentUpdateInterval = 8;
};
