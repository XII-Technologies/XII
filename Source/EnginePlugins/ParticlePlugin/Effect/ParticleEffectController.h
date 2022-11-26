#pragma once

#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleEffectController
{
public:
  xiiParticleEffectController();
  xiiParticleEffectController(const xiiParticleEffectController& rhs);
  void operator=(const xiiParticleEffectController& rhs);

  void Create(const xiiParticleEffectResourceHandle& hEffectResource, xiiParticleWorldModule* pModule, xiiUInt64 uiRandomSeed, const char* szSharedName /*= nullptr*/, const void* pSharedInstanceOwner /*= nullptr*/, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams);

  bool IsValid() const;
  void Invalidate();

  bool IsAlive() const;
  bool IsSharedInstance() const { return m_pSharedInstanceOwner != nullptr; }

  bool IsContinuousEffect() const { return GetInstance()->IsContinuous(); }

  void SetTransform(const xiiTransform& t, const xiiVec3& vParticleStartVelocity) const;

  void Tick(const xiiTime& tDiff) const;

  void ExtractRenderData(xiiMsgExtractRenderData& msg, const xiiTransform& systemTransform) const;

  void StopImmediate();

  /// \brief Returns the bounding volume of the effect.
  /// The volume is in the local space of the effect.
  void GetBoundingVolume(xiiBoundingBoxSphere& volume) const;

  void UpdateWindSamples();

  /// \brief Ensures that the effect is considered to be 'visible', which affects the update rate.
  void ForceVisible();

  xiiUInt64 GetNumActiveParticles() const;

  /// \name Effect Parameters
  ///@{
public:
  /// \brief Passes an effect parameter on to the effect instance
  void SetParameter(const xiiTempHashedString& name, float value);

  /// \brief Passes an effect parameter on to the effect instance
  void SetParameter(const xiiTempHashedString& name, const xiiColor& value);

  ///@}

private:
  friend class xiiParticleWorldModule;

  xiiParticleEffectController(xiiParticleWorldModule* pModule, xiiParticleEffectHandle hEffect);
  xiiParticleEffectInstance* GetInstance() const;

  const void*             m_pSharedInstanceOwner = nullptr;
  xiiParticleWorldModule* m_pModule              = nullptr;
  xiiParticleEffectHandle m_hEffect;
};
