#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Type/Effect/ParticleTypeEffect.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeEffectFactory, 1, xiiRTTIDefaultAllocator<xiiParticleTypeEffectFactory>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Particle_Effect")),
    // XII_MEMBER_PROPERTY("Shared Instance Name", m_sSharedInstanceName), // there is currently no way (I can think of) to uniquely identify each sub-system for the 'shared owner'
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeEffect, 1, xiiRTTIDefaultAllocator<xiiParticleTypeEffect>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleTypeEffectFactory::xiiParticleTypeEffectFactory()  = default;
xiiParticleTypeEffectFactory::~xiiParticleTypeEffectFactory() = default;

const xiiRTTI* xiiParticleTypeEffectFactory::GetTypeType() const
{
  return xiiGetStaticRTTI<xiiParticleTypeEffect>();
}

void xiiParticleTypeEffectFactory::CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const
{
  xiiParticleTypeEffect* pType = static_cast<xiiParticleTypeEffect*>(pObject);

  pType->m_hEffect.Invalidate();

  if (!m_sEffect.IsEmpty())
    pType->m_hEffect = xiiResourceManager::LoadResource<xiiParticleEffectResource>(m_sEffect);

  // pType->m_sSharedInstanceName = m_sSharedInstanceName;


  if (bFirstTime)
  {
    pType->GetOwnerSystem()->AddParticleDeathEventHandler(xiiMakeDelegate(&xiiParticleTypeEffect::OnParticleDeath, pType));
  }
}

enum class TypeEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleTypeEffectFactory::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = (int)TypeEffectVersion::Version_Current;
  stream << uiVersion;

  xiiUInt64 m_uiRandomSeed = 0;

  stream << m_sEffect;
  stream << m_uiRandomSeed;
  stream << m_sSharedInstanceName;
}

void xiiParticleTypeEffectFactory::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)TypeEffectVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sEffect;

  if (uiVersion >= 2)
  {
    xiiUInt64 m_uiRandomSeed = 0;

    stream >> m_uiRandomSeed;
    stream >> m_sSharedInstanceName;
  }
}


xiiParticleTypeEffect::xiiParticleTypeEffect() = default;

xiiParticleTypeEffect::~xiiParticleTypeEffect()
{
  if (m_pStreamPosition != nullptr)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(xiiMakeDelegate(&xiiParticleTypeEffect::OnParticleDeath, this));

    ClearEffects(true);
  }
}

void xiiParticleTypeEffect::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("EffectID", xiiProcessingStream::DataType::Int, &m_pStreamEffectID, false);
}

void xiiParticleTypeEffect::ExtractTypeRenderData(xiiMsgExtractRenderData& msg, const xiiTransform& instanceTransform) const
{
  XII_PROFILE_SCOPE("PFX: Effect");

  const xiiUInt32 numParticles = (xiiUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  const xiiUInt32* pEffectID = m_pStreamEffectID->GetData<xiiUInt32>();

  const xiiParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  for (xiiUInt32 i = 0; i < numParticles; ++i)
  {
    xiiParticleEffectHandle hInstance = xiiParticleEffectHandle(xiiParticleEffectId(pEffectID[i]));

    const xiiParticleEffectInstance* pEffect = nullptr;
    if (pWorldModule->TryGetEffectInstance(hInstance, pEffect))
    {
      pWorldModule->ExtractEffectRenderData(pEffect, msg, pEffect->GetTransform());
    }
  }
}

void xiiParticleTypeEffect::OnReset()
{
  ClearEffects(true);
}

void xiiParticleTypeEffect::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Effect");

  if (!m_hEffect.IsValid())
    return;

  const xiiVec4* pPosition = m_pStreamPosition->GetData<xiiVec4>();
  xiiUInt32*     pEffectID = m_pStreamEffectID->GetWritableData<xiiUInt32>();

  xiiParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  m_fMaxEffectRadius = 0.0f;

  const xiiUInt64 uiRandomSeed = GetOwnerEffect()->GetRandomSeed();

  for (xiiUInt32 i = 0; i < uiNumElements; ++i)
  {
    if (pEffectID[i] == 0) // always an invalid ID
    {
      const void*             pDummy    = nullptr;
      xiiParticleEffectHandle hInstance = pWorldModule->CreateEffectInstance(m_hEffect, uiRandomSeed, /*m_sSharedInstanceName*/ nullptr, pDummy, xiiArrayPtr<xiiParticleEffectFloatParam>(), xiiArrayPtr<xiiParticleEffectColorParam>());

      pEffectID[i] = hInstance.GetInternalID().m_Data;
    }

    xiiParticleEffectHandle hInstance = xiiParticleEffectHandle(xiiParticleEffectId(pEffectID[i]));

    xiiParticleEffectInstance* pEffect = nullptr;
    if (pWorldModule->TryGetEffectInstance(hInstance, pEffect))
    {
      xiiTransform t;
      t.m_qRotation.SetIdentity();
      t.m_vScale.Set(1.0f);
      t.m_vPosition = pPosition[i].GetAsVec3();

      // TODO: pass through velocity
      pEffect->SetVisibleIf(GetOwnerEffect());
      pEffect->SetTransformForNextFrame(t, xiiVec3::ZeroVector());

      xiiBoundingBoxSphere bounds;
      pEffect->GetBoundingVolume(bounds);

      m_fMaxEffectRadius = xiiMath::Max(m_fMaxEffectRadius, bounds.m_fSphereRadius);
    }
  }
}

void xiiParticleTypeEffect::OnParticleDeath(const xiiStreamGroupElementRemovedEvent& e)
{
  xiiParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  const xiiUInt32* pEffectID = m_pStreamEffectID->GetData<xiiUInt32>();

  xiiParticleEffectHandle hInstance = xiiParticleEffectHandle(xiiParticleEffectId(pEffectID[e.m_uiElementIndex]));

  pWorldModule->DestroyEffectInstance(hInstance, false, nullptr);
}

void xiiParticleTypeEffect::ClearEffects(bool bInterruptImmediately)
{
  // delete all effects that are still in the processing group

  xiiParticleWorldModule* pWorldModule   = GetOwnerEffect()->GetOwnerWorldModule();
  const xiiUInt64         uiNumParticles = GetOwnerSystem()->GetNumActiveParticles();

  if (uiNumParticles == 0 || m_pStreamEffectID == nullptr)
    return;

  xiiUInt32* pEffectID = m_pStreamEffectID->GetWritableData<xiiUInt32>();

  for (xiiUInt32 elemIdx = 0; elemIdx < uiNumParticles; ++elemIdx)
  {
    xiiParticleEffectHandle hInstance = xiiParticleEffectHandle(xiiParticleEffectId(pEffectID[elemIdx]));
    pEffectID[elemIdx]                = 0;

    pWorldModule->DestroyEffectInstance(hInstance, bInterruptImmediately, nullptr);
  }
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Effect_ParticleTypeEffect);
