#pragma once

#include <Core/Curves/ColorGradientResource.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_ColorGradient final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_ColorGradient, xiiParticleBehaviorFactory);

public:
  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void                    SetColorGradient(const xiiColorGradientResourceHandle& hResource) { m_hGradient = hResource; }
  XII_ALWAYS_INLINE const xiiColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  void        SetColorGradientFile(const char* szFile);
  const char* GetColorGradientFile() const;

  xiiEnum<xiiParticleColorGradientMode> m_GradientMode;
  float                                 m_fMaxSpeed = 1.0f;
  xiiColor                              m_TintColor = xiiColor::White;

private:
  xiiColorGradientResourceHandle m_hGradient;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_ColorGradient final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_ColorGradient, xiiParticleBehavior);

public:
  xiiColorGradientResourceHandle        m_hGradient;
  xiiEnum<xiiParticleColorGradientMode> m_GradientMode;
  float                                 m_fMaxSpeed = 1.0f;
  xiiColor                              m_TintColor;

  virtual void CreateRequiredStreams() override;

protected:
  friend class xiiParticleBehaviorFactory_ColorGradient;

  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
  virtual void Process(xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamLifeTime = nullptr;
  xiiProcessingStream* m_pStreamColor    = nullptr;
  xiiProcessingStream* m_pStreamVelocity = nullptr;
  xiiColor             m_InitColor;
  xiiUInt8             m_uiFirstToUpdate         = 0;
  xiiUInt8             m_uiCurrentUpdateInterval = 8;
};
