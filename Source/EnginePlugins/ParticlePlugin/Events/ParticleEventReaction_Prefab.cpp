#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Prefab.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEventReactionFactory_Prefab, 1, xiiRTTIDefaultAllocator<xiiParticleEventReactionFactory_Prefab>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Prefab", m_sPrefab)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab")),
    XII_ENUM_MEMBER_PROPERTY("Alignment", xiiSurfaceInteractionAlignment, m_Alignment),
    //XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("CompatibleAsset_Prefab"), new xiiExposeColorAlphaAttribute),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEventReaction_Prefab, 1, xiiRTTIDefaultAllocator<xiiParticleEventReaction_Prefab>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleEventReactionFactory_Prefab::xiiParticleEventReactionFactory_Prefab()
{
  // m_Parameters = XII_DEFAULT_NEW(xiiParticlePrefabParameters);
}

enum class ReactionPrefabVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  // Version_3, // added Prefab parameters

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleEventReactionFactory_Prefab::Save(xiiStreamWriter& stream) const
{
  SUPER::Save(stream);

  const xiiUInt8 uiVersion = (int)ReactionPrefabVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sPrefab;

  // Version 2
  stream << m_Alignment;

  // Version 3
  // stream << m_Parameters->m_FloatParams.GetCount();
  // for (xiiUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
  //{
  //  stream << m_Parameters->m_FloatParams[i].m_sName;
  //  stream << m_Parameters->m_FloatParams[i].m_Value;
  //}
  // stream << m_Parameters->m_ColorParams.GetCount();
  // for (xiiUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
  //{
  //  stream << m_Parameters->m_ColorParams[i].m_sName;
  //  stream << m_Parameters->m_ColorParams[i].m_Value;
  //}
}

void xiiParticleEventReactionFactory_Prefab::Load(xiiStreamReader& stream)
{
  SUPER::Load(stream);

  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)ReactionPrefabVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sPrefab;

  if (uiVersion >= 2)
  {
    stream >> m_Alignment;
  }

  // if (uiVersion >= 3)
  //{
  //  xiiUInt32 numFloats, numColors;

  //  stream >> numFloats;
  //  m_Parameters->m_FloatParams.SetCountUninitialized(numFloats);

  //  for (xiiUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
  //  {
  //    stream >> m_Parameters->m_FloatParams[i].m_sName;
  //    stream >> m_Parameters->m_FloatParams[i].m_Value;
  //  }

  //  stream >> numColors;
  //  m_Parameters->m_ColorParams.SetCountUninitialized(numColors);

  //  for (xiiUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
  //  {
  //    stream >> m_Parameters->m_ColorParams[i].m_sName;
  //    stream >> m_Parameters->m_ColorParams[i].m_Value;
  //  }
  //}
}


const xiiRTTI* xiiParticleEventReactionFactory_Prefab::GetEventReactionType() const
{
  return xiiGetStaticRTTI<xiiParticleEventReaction_Prefab>();
}


void xiiParticleEventReactionFactory_Prefab::CopyReactionProperties(xiiParticleEventReaction* pObject, bool bFirstTime) const
{
  xiiParticleEventReaction_Prefab* pReaction = static_cast<xiiParticleEventReaction_Prefab*>(pObject);

  pReaction->m_hPrefab.Invalidate();
  pReaction->m_Alignment = m_Alignment;

  if (!m_sPrefab.IsEmpty())
    pReaction->m_hPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>(m_sPrefab);

  // pReaction->m_Parameters = m_Parameters;
}
//
// const xiiRangeView<const char*, xiiUInt32> xiiParticleEventReactionFactory_Prefab::GetParameters() const
//{
//  return xiiRangeView<const char*, xiiUInt32>(
//      [this]() -> xiiUInt32 { return 0; },
//      [this]() -> xiiUInt32 { return m_Parameters->m_FloatParams.GetCount() + m_Parameters->m_ColorParams.GetCount(); },
//      [this](xiiUInt32& it) { ++it; },
//      [this](const xiiUInt32& it) -> const char* {
//        if (it < m_Parameters->m_FloatParams.GetCount())
//          return m_Parameters->m_FloatParams[it].m_sName.GetData();
//        else
//          return m_Parameters->m_ColorParams[it - m_Parameters->m_FloatParams.GetCount()].m_sName.GetData();
//      });
//}
//
// void xiiParticleEventReactionFactory_Prefab::SetParameter(const char* szKey, const xiiVariant& var)
//{
//  const xiiTempHashedString th(szKey);
//  if (var.CanConvertTo<float>())
//  {
//    float value = var.ConvertTo<float>();
//
//    for (xiiUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
//    {
//      if (m_Parameters->m_FloatParams[i].m_sName == th)
//      {
//        if (m_Parameters->m_FloatParams[i].m_Value != value)
//        {
//          m_Parameters->m_FloatParams[i].m_Value = value;
//        }
//        return;
//      }
//    }
//
//    auto& e = m_Parameters->m_FloatParams.ExpandAndGetRef();
//    e.m_sName.Assign(szKey);
//    e.m_Value = value;
//
//    return;
//  }
//
//  if (var.CanConvertTo<xiiColor>())
//  {
//    xiiColor value = var.ConvertTo<xiiColor>();
//
//    for (xiiUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
//    {
//      if (m_Parameters->m_ColorParams[i].m_sName == th)
//      {
//        if (m_Parameters->m_ColorParams[i].m_Value != value)
//        {
//          m_Parameters->m_ColorParams[i].m_Value = value;
//        }
//        return;
//      }
//    }
//
//    auto& e = m_Parameters->m_ColorParams.ExpandAndGetRef();
//    e.m_sName.Assign(szKey);
//    e.m_Value = value;
//
//    return;
//  }
//}
//
// void xiiParticleEventReactionFactory_Prefab::RemoveParameter(const char* szKey)
//{
//  const xiiTempHashedString th(szKey);
//
//  for (xiiUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
//  {
//    if (m_Parameters->m_FloatParams[i].m_sName == th)
//    {
//      m_Parameters->m_FloatParams.RemoveAtAndSwap(i);
//      return;
//    }
//  }
//
//  for (xiiUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
//  {
//    if (m_Parameters->m_ColorParams[i].m_sName == th)
//    {
//      m_Parameters->m_ColorParams.RemoveAtAndSwap(i);
//      return;
//    }
//  }
//}
//
// bool xiiParticleEventReactionFactory_Prefab::GetParameter(const char* szKey, xiiVariant& out_value) const
//{
//  const xiiTempHashedString th(szKey);
//
//  for (const auto& e : m_Parameters->m_FloatParams)
//  {
//    if (e.m_sName == th)
//    {
//      out_value = e.m_Value;
//      return true;
//    }
//  }
//  for (const auto& e : m_Parameters->m_ColorParams)
//  {
//    if (e.m_sName == th)
//    {
//      out_value = e.m_Value;
//      return true;
//    }
//  }
//  return false;
//}

//////////////////////////////////////////////////////////////////////////

xiiParticleEventReaction_Prefab::xiiParticleEventReaction_Prefab()  = default;
xiiParticleEventReaction_Prefab::~xiiParticleEventReaction_Prefab() = default;

void xiiParticleEventReaction_Prefab::ProcessEvent(const xiiParticleEvent& e)
{
  if (!m_hPrefab.IsValid())
    return;

  xiiTransform trans;
  trans.m_vScale.Set(1.0f);
  trans.m_vPosition = e.m_vPosition;

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

  // rotate the prefab randomly along its main axis (the X axis)
  xiiQuat qRot;
  qRot.SetFromAxisAndAngle(xiiVec3(1, 0, 0), xiiAngle::Radian((float)m_pOwnerEffect->GetRNG().DoubleZeroToOneInclusive() * xiiMath::Pi<float>() * 2.0f));

  vAlignDir.NormalizeIfNotZero(xiiVec3::UnitXAxis()).IgnoreResult();

  trans.m_qRotation.SetShortestRotation(xiiVec3(1, 0, 0), vAlignDir);
  trans.m_qRotation = trans.m_qRotation * qRot;

  xiiResourceLock<xiiPrefabResource> pPrefab(m_hPrefab, xiiResourceAcquireMode::BlockTillLoaded);

  xiiPrefabInstantiationOptions options;

  pPrefab->InstantiatePrefab(*m_pOwnerEffect->GetWorld(), trans, options);
}
