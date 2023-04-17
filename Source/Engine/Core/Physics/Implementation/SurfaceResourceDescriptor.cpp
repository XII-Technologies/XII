#include <Core/CorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSurfaceInteractionAlignment, 2)
  XII_ENUM_CONSTANTS(xiiSurfaceInteractionAlignment::SurfaceNormal, xiiSurfaceInteractionAlignment::IncidentDirection, xiiSurfaceInteractionAlignment::ReflectedDirection)
  XII_ENUM_CONSTANTS(xiiSurfaceInteractionAlignment::ReverseSurfaceNormal, xiiSurfaceInteractionAlignment::ReverseIncidentDirection, xiiSurfaceInteractionAlignment::ReverseReflectedDirection)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSurfaceInteraction, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSurfaceInteraction>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sInteractionType)->AddAttributes(new xiiDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    XII_ACCESSOR_PROPERTY("Prefab", GetPrefab, SetPrefab)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
    // this does not work yet (asset transform fails)
    //XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("CompatibleAsset_Prefab")),
    XII_ENUM_MEMBER_PROPERTY("Alignment", xiiSurfaceInteractionAlignment, m_Alignment),
    XII_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new xiiClampValueAttribute(xiiVariant(xiiAngle::Degree(0.0f)), xiiVariant(xiiAngle::Degree(90.0f)))),
    XII_MEMBER_PROPERTY("ImpulseThreshold", m_fImpulseThreshold),
    XII_MEMBER_PROPERTY("ImpulseScale", m_fImpulseScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceResourceDescriptor, 2, xiiRTTIDefaultAllocator<xiiSurfaceResourceDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BaseSurface", GetBaseSurfaceFile, SetBaseSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")), // Package + Thumbnail So that circular dependencies are caught.
    XII_MEMBER_PROPERTY("Restitution", m_fPhysicsRestitution)->AddAttributes(new xiiDefaultValueAttribute(0.25f)),
    XII_MEMBER_PROPERTY("StaticFriction", m_fPhysicsFrictionStatic)->AddAttributes(new xiiDefaultValueAttribute(0.6f)),
    XII_MEMBER_PROPERTY("DynamicFriction", m_fPhysicsFrictionDynamic)->AddAttributes(new xiiDefaultValueAttribute(0.4f)),
    XII_ACCESSOR_PROPERTY("OnCollideInteraction", GetCollisionInteraction, SetCollisionInteraction)->AddAttributes(new xiiDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    XII_ACCESSOR_PROPERTY("SlideReaction", GetSlideReactionPrefabFile, SetSlideReactionPrefabFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("RollReaction", GetRollReactionPrefabFile, SetRollReactionPrefabFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
    XII_ARRAY_MEMBER_PROPERTY("Interactions", m_Interactions),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSurfaceInteraction::SetPrefab(const char* szPrefab)
{
  xiiPrefabResourceHandle hPrefab;

  if (!xiiStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>(szPrefab);
  }

  m_hPrefab = hPrefab;
}

const char* xiiSurfaceInteraction::GetPrefab() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

const xiiRangeView<const char*, xiiUInt32> xiiSurfaceInteraction::GetParameters() const
{
  return xiiRangeView<const char*, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                              [this]() -> xiiUInt32 { return m_Parameters.GetCount(); },
                                              [](xiiUInt32& it) { ++it; },
                                              [this](const xiiUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void xiiSurfaceInteraction::SetParameter(const char* szKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != xiiInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void xiiSurfaceInteraction::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(xiiTempHashedString(szKey));
}

bool xiiSurfaceInteraction::GetParameter(const char* szKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Parameters.Find(szKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void xiiSurfaceResourceDescriptor::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;

  stream >> uiVersion;
  XII_ASSERT_DEV(uiVersion <= 7, "Invalid version {0} for surface resource", uiVersion);

  stream >> m_fPhysicsRestitution;
  stream >> m_fPhysicsFrictionStatic;
  stream >> m_fPhysicsFrictionDynamic;
  stream >> m_hBaseSurface;

  if (uiVersion >= 4)
  {
    stream >> m_sOnCollideInteraction;
  }

  if (uiVersion >= 7)
  {
    stream >> m_sSlideInteractionPrefab;
    stream >> m_sRollInteractionPrefab;
  }

  if (uiVersion > 2)
  {
    xiiUInt32 count = 0;
    stream >> count;
    m_Interactions.SetCount(count);

    xiiStringBuilder sTemp;
    for (xiiUInt32 i = 0; i < count; ++i)
    {
      auto& ia = m_Interactions[i];

      stream >> sTemp;
      ia.m_sInteractionType = sTemp;

      stream >> ia.m_hPrefab;
      stream >> ia.m_Alignment;
      stream >> ia.m_Deviation;

      if (uiVersion >= 4)
      {
        stream >> ia.m_fImpulseThreshold;
      }

      if (uiVersion >= 5)
      {
        stream >> ia.m_fImpulseScale;
      }

      if (uiVersion >= 6)
      {
        xiiUInt8 uiNumParams;
        stream >> uiNumParams;

        ia.m_Parameters.Clear();
        ia.m_Parameters.Reserve(uiNumParams);

        xiiHashedString key;
        xiiVariant      value;

        for (xiiUInt32 i2 = 0; i2 < uiNumParams; ++i2)
        {
          stream >> key;
          stream >> value;

          ia.m_Parameters.Insert(key, value);
        }
      }
    }
  }
}

void xiiSurfaceResourceDescriptor::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = 7;

  stream << uiVersion;
  stream << m_fPhysicsRestitution;
  stream << m_fPhysicsFrictionStatic;
  stream << m_fPhysicsFrictionDynamic;
  stream << m_hBaseSurface;

  // version 4
  stream << m_sOnCollideInteraction;

  // version 7
  stream << m_sSlideInteractionPrefab;
  stream << m_sRollInteractionPrefab;

  stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    stream << ia.m_sInteractionType;
    stream << ia.m_hPrefab;
    stream << ia.m_Alignment;
    stream << ia.m_Deviation;

    // version 4
    stream << ia.m_fImpulseThreshold;

    // version 5
    stream << ia.m_fImpulseScale;

    // version 6
    const xiiUInt8 uiNumParams = static_cast<xiiUInt8>(ia.m_Parameters.GetCount());
    stream << uiNumParams;
    for (xiiUInt32 i = 0; i < uiNumParams; ++i)
    {
      stream << ia.m_Parameters.GetKey(i);
      stream << ia.m_Parameters.GetValue(i);
    }
  }
}

void xiiSurfaceResourceDescriptor::SetBaseSurfaceFile(const char* szFile)
{
  xiiSurfaceResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiSurfaceResource>(szFile);
  }

  m_hBaseSurface = hResource;
}

const char* xiiSurfaceResourceDescriptor::GetBaseSurfaceFile() const
{
  if (!m_hBaseSurface.IsValid())
    return "";

  return m_hBaseSurface.GetResourceID();
}

void xiiSurfaceResourceDescriptor::SetCollisionInteraction(const char* name)
{
  m_sOnCollideInteraction.Assign(name);
}

const char* xiiSurfaceResourceDescriptor::GetCollisionInteraction() const
{
  return m_sOnCollideInteraction.GetData();
}

void xiiSurfaceResourceDescriptor::SetSlideReactionPrefabFile(const char* szFile)
{
  m_sSlideInteractionPrefab.Assign(szFile);
}

const char* xiiSurfaceResourceDescriptor::GetSlideReactionPrefabFile() const
{
  return m_sSlideInteractionPrefab.GetData();
}

void xiiSurfaceResourceDescriptor::SetRollReactionPrefabFile(const char* szFile)
{
  m_sRollInteractionPrefab.Assign(szFile);
}

const char* xiiSurfaceResourceDescriptor::GetRollReactionPrefabFile() const
{
  return m_sRollInteractionPrefab.GetData();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiSurfaceResourceDescriptorPatch_1_2 : public xiiGraphPatch
{
public:
  xiiSurfaceResourceDescriptorPatch_1_2() :
    xiiGraphPatch("xiiSurfaceResourceDescriptor", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Base Surface", "BaseSurface");
    pNode->RenameProperty("Static Friction", "StaticFriction");
    pNode->RenameProperty("Dynamic Friction", "DynamicFriction");
  }
};

xiiSurfaceResourceDescriptorPatch_1_2 g_xiiSurfaceResourceDescriptorPatch_1_2;


XII_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResourceDescriptor);
