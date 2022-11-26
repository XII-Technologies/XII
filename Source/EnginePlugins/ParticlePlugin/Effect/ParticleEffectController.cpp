#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

xiiParticleEffectController::xiiParticleEffectController()
{
  m_hEffect.Invalidate();
}

xiiParticleEffectController::xiiParticleEffectController(const xiiParticleEffectController& rhs)
{
  m_pModule              = rhs.m_pModule;
  m_hEffect              = rhs.m_hEffect;
  m_pSharedInstanceOwner = rhs.m_pSharedInstanceOwner;
}

xiiParticleEffectController::xiiParticleEffectController(xiiParticleWorldModule* pModule, xiiParticleEffectHandle hEffect)
{
  m_pModule = pModule;
  m_hEffect = hEffect;
}

void xiiParticleEffectController::operator=(const xiiParticleEffectController& rhs)
{
  m_pModule              = rhs.m_pModule;
  m_hEffect              = rhs.m_hEffect;
  m_pSharedInstanceOwner = rhs.m_pSharedInstanceOwner;
}

xiiParticleEffectInstance* xiiParticleEffectController::GetInstance() const
{
  if (m_pModule == nullptr)
    return nullptr;

  xiiParticleEffectInstance* pEffect = nullptr;
  m_pModule->TryGetEffectInstance(m_hEffect, pEffect);
  return pEffect;
}

void xiiParticleEffectController::Create(const xiiParticleEffectResourceHandle& hEffectResource, xiiParticleWorldModule* pModule, xiiUInt64 uiRandomSeed, const char* szSharedName, const void* pSharedInstanceOwner, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams)
{
  m_pSharedInstanceOwner = pSharedInstanceOwner;

  // first get the new effect, to potentially increase a refcount to the same effect instance, before we decrease the refcount of our
  // current one
  xiiParticleEffectHandle hNewEffect;
  if (pModule != nullptr && hEffectResource.IsValid())
  {
    hNewEffect = pModule->CreateEffectInstance(hEffectResource, uiRandomSeed, szSharedName, m_pSharedInstanceOwner, floatParams, colorParams);
  }

  Invalidate();

  m_hEffect = hNewEffect;

  if (!m_hEffect.IsInvalidated())
    m_pModule = pModule;
}

bool xiiParticleEffectController::IsValid() const
{
  return (m_pModule != nullptr && !m_hEffect.IsInvalidated());
}

bool xiiParticleEffectController::IsAlive() const
{
  xiiParticleEffectInstance* pEffect = GetInstance();
  return pEffect != nullptr;
}

void xiiParticleEffectController::SetTransform(const xiiTransform& t, const xiiVec3& vParticleStartVelocity) const
{
  xiiParticleEffectInstance* pEffect = GetInstance();

  // shared effects are always simulated at the origin
  if (pEffect && m_pSharedInstanceOwner == nullptr)
  {
    pEffect->SetTransform(t, vParticleStartVelocity);
  }
}

void xiiParticleEffectController::Tick(const xiiTime& tDiff) const
{
  xiiParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->PreSimulate();
    pEffect->Update(tDiff);
  }
}

void xiiParticleEffectController::ExtractRenderData(xiiMsgExtractRenderData& msg, const xiiTransform& systemTransform) const
{
  if (const xiiParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->SetIsVisible();

    m_pModule->ExtractEffectRenderData(pEffect, msg, systemTransform);
  }
}

void xiiParticleEffectController::StopImmediate()
{
  if (m_pModule)
  {
    m_pModule->DestroyEffectInstance(m_hEffect, true, m_pSharedInstanceOwner);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

void xiiParticleEffectController::GetBoundingVolume(xiiBoundingBoxSphere& volume) const
{
  if (xiiParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->GetBoundingVolume(volume);
  }
}

void xiiParticleEffectController::UpdateWindSamples()
{
  if (xiiParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->UpdateWindSamples();
  }
}

void xiiParticleEffectController::ForceVisible()
{
  if (xiiParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->SetIsVisible();
  }
}

xiiUInt64 xiiParticleEffectController::GetNumActiveParticles() const
{
  if (xiiParticleEffectInstance* pEffect = GetInstance())
  {
    return pEffect->GetNumActiveParticles();
  }

  return 0;
}

void xiiParticleEffectController::SetParameter(const xiiTempHashedString& name, float value)
{
  xiiParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->SetParameter(name, value);
  }
}

void xiiParticleEffectController::SetParameter(const xiiTempHashedString& name, const xiiColor& value)
{
  xiiParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->SetParameter(name, value);
  }
}

void xiiParticleEffectController::Invalidate()
{
  if (m_pModule)
  {
    m_pModule->DestroyEffectInstance(m_hEffect, false, m_pSharedInstanceOwner);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectController);
