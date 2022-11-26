#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Effect.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEventReactionFactory_Effect, 1, xiiRTTIDefaultAllocator<xiiParticleEventReactionFactory_Effect>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Particle_Effect")),
    XII_ENUM_MEMBER_PROPERTY("Alignment", xiiSurfaceInteractionAlignment, m_Alignment),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("Effect"), new xiiExposeColorAlphaAttribute),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEventReaction_Effect, 1, xiiRTTIDefaultAllocator<xiiParticleEventReaction_Effect>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleEventReactionFactory_Effect::xiiParticleEventReactionFactory_Effect()
{
  m_pParameters = XII_DEFAULT_NEW(xiiParticleEffectParameters);
}

enum class ReactionEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added effect parameters
  Version_3, // added alignment

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleEventReactionFactory_Effect::Save(xiiStreamWriter& stream) const
{
  SUPER::Save(stream);

  const xiiUInt8 uiVersion = (int)ReactionEffectVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sEffect;

  // Version 2
  stream << m_pParameters->m_FloatParams.GetCount();
  for (xiiUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
  {
    stream << m_pParameters->m_FloatParams[i].m_sName;
    stream << m_pParameters->m_FloatParams[i].m_Value;
  }
  stream << m_pParameters->m_ColorParams.GetCount();
  for (xiiUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
  {
    stream << m_pParameters->m_ColorParams[i].m_sName;
    stream << m_pParameters->m_ColorParams[i].m_Value;
  }

  // Version 3
  stream << m_Alignment;
}

void xiiParticleEventReactionFactory_Effect::Load(xiiStreamReader& stream)
{
  SUPER::Load(stream);

  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)ReactionEffectVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sEffect;

  if (uiVersion >= 2)
  {
    xiiUInt32 numFloats, numColors;

    stream >> numFloats;
    m_pParameters->m_FloatParams.SetCountUninitialized(numFloats);

    for (xiiUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
    {
      stream >> m_pParameters->m_FloatParams[i].m_sName;
      stream >> m_pParameters->m_FloatParams[i].m_Value;
    }

    stream >> numColors;
    m_pParameters->m_ColorParams.SetCountUninitialized(numColors);

    for (xiiUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
    {
      stream >> m_pParameters->m_ColorParams[i].m_sName;
      stream >> m_pParameters->m_ColorParams[i].m_Value;
    }
  }

  if (uiVersion >= 3)
  {
    stream >> m_Alignment;
  }
}


const xiiRTTI* xiiParticleEventReactionFactory_Effect::GetEventReactionType() const
{
  return xiiGetStaticRTTI<xiiParticleEventReaction_Effect>();
}


void xiiParticleEventReactionFactory_Effect::CopyReactionProperties(xiiParticleEventReaction* pObject, bool bFirstTime) const
{
  xiiParticleEventReaction_Effect* pReaction = static_cast<xiiParticleEventReaction_Effect*>(pObject);

  pReaction->m_hEffect.Invalidate();
  pReaction->m_Alignment = m_Alignment;

  if (!m_sEffect.IsEmpty())
    pReaction->m_hEffect = xiiResourceManager::LoadResource<xiiParticleEffectResource>(m_sEffect);

  pReaction->m_Parameters = m_pParameters;
}

const xiiRangeView<const char*, xiiUInt32> xiiParticleEventReactionFactory_Effect::GetParameters() const
{
  return xiiRangeView<const char*, xiiUInt32>([this]() -> xiiUInt32 { return 0; },
                                              [this]() -> xiiUInt32 { return m_pParameters->m_FloatParams.GetCount() + m_pParameters->m_ColorParams.GetCount(); }, [this](xiiUInt32& it) { ++it; },
                                              [this](const xiiUInt32& it) -> const char* {
                                                if (it < m_pParameters->m_FloatParams.GetCount())
                                                  return m_pParameters->m_FloatParams[it].m_sName.GetData();
                                                else
                                                  return m_pParameters->m_ColorParams[it - m_pParameters->m_FloatParams.GetCount()].m_sName.GetData();
                                              });
}

void xiiParticleEventReactionFactory_Effect::SetParameter(const char* szKey, const xiiVariant& var)
{
  const xiiTempHashedString th(szKey);
  if (var.CanConvertTo<float>())
  {
    float value = var.ConvertTo<float>();

    for (xiiUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
    {
      if (m_pParameters->m_FloatParams[i].m_sName == th)
      {
        if (m_pParameters->m_FloatParams[i].m_Value != value)
        {
          m_pParameters->m_FloatParams[i].m_Value = value;
        }
        return;
      }
    }

    auto& e = m_pParameters->m_FloatParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }

  if (var.CanConvertTo<xiiColor>())
  {
    xiiColor value = var.ConvertTo<xiiColor>();

    for (xiiUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
    {
      if (m_pParameters->m_ColorParams[i].m_sName == th)
      {
        if (m_pParameters->m_ColorParams[i].m_Value != value)
        {
          m_pParameters->m_ColorParams[i].m_Value = value;
        }
        return;
      }
    }

    auto& e = m_pParameters->m_ColorParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }
}

void xiiParticleEventReactionFactory_Effect::RemoveParameter(const char* szKey)
{
  const xiiTempHashedString th(szKey);

  for (xiiUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
  {
    if (m_pParameters->m_FloatParams[i].m_sName == th)
    {
      m_pParameters->m_FloatParams.RemoveAtAndSwap(i);
      return;
    }
  }

  for (xiiUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
  {
    if (m_pParameters->m_ColorParams[i].m_sName == th)
    {
      m_pParameters->m_ColorParams.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool xiiParticleEventReactionFactory_Effect::GetParameter(const char* szKey, xiiVariant& out_value) const
{
  const xiiTempHashedString th(szKey);

  for (const auto& e : m_pParameters->m_FloatParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  for (const auto& e : m_pParameters->m_ColorParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

xiiParticleEventReaction_Effect::xiiParticleEventReaction_Effect()  = default;
xiiParticleEventReaction_Effect::~xiiParticleEventReaction_Effect() = default;

void xiiParticleEventReaction_Effect::ProcessEvent(const xiiParticleEvent& e)
{
  if (!m_hEffect.IsValid())
    return;

  xiiGameObjectDesc god;
  god.m_bDynamic      = true;
  god.m_LocalPosition = e.m_vPosition;

  xiiVec3 vAlignDir = e.m_vNormal;

  switch (m_Alignment)
  {
    case xiiSurfaceInteractionAlignment::IncidentDirection:
      vAlignDir = -e.m_vDirection;
      break;

    case xiiSurfaceInteractionAlignment::ReflectedDirection:
      vAlignDir = e.m_vDirection.GetReflectedVector(e.m_vNormal);
      break;

    case xiiSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vAlignDir = -e.m_vNormal;
      break;

    case xiiSurfaceInteractionAlignment::ReverseIncidentDirection:
      vAlignDir = e.m_vDirection;
      ;
      break;

    case xiiSurfaceInteractionAlignment::ReverseReflectedDirection:
      vAlignDir = -e.m_vDirection.GetReflectedVector(e.m_vNormal);
      break;

    case xiiSurfaceInteractionAlignment::SurfaceNormal:
      break;
  }

  if (!vAlignDir.IsZero())
  {
    god.m_LocalRotation.SetShortestRotation(xiiVec3(0, 0, 1), vAlignDir);
  }

  xiiGameObject* pObject = nullptr;
  m_pOwnerEffect->GetWorld()->CreateObject(god, pObject);

  xiiParticleComponent* pComponent = nullptr;
  xiiParticleComponent::CreateComponent(pObject, pComponent);

  pComponent->m_uiRandomSeed = m_pOwnerEffect->GetRandomSeed();

  pComponent->m_bIfContinuousStopRightAway = true;
  pComponent->m_OnFinishedAction           = xiiOnComponentFinishedAction2::DeleteGameObject;
  pComponent->SetParticleEffect(m_hEffect);

  if (!m_Parameters->m_FloatParams.IsEmpty())
  {
    pComponent->m_bFloatParamsChanged = true;
    pComponent->m_FloatParams         = m_Parameters->m_FloatParams;
  }

  if (!m_Parameters->m_ColorParams.IsEmpty())
  {
    pComponent->m_bColorParamsChanged = true;
    pComponent->m_ColorParams         = m_Parameters->m_ColorParams;
  }
}
