#include <Core/CorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSurfaceInteractionAlignment, 1)
  XII_ENUM_CONSTANTS(xiiSurfaceInteractionAlignment::SurfaceNormal, xiiSurfaceInteractionAlignment::IncidentDirection, xiiSurfaceInteractionAlignment::ReflectedDirection)
  XII_ENUM_CONSTANTS(xiiSurfaceInteractionAlignment::ReverseSurfaceNormal, xiiSurfaceInteractionAlignment::ReverseIncidentDirection, xiiSurfaceInteractionAlignment::ReverseReflectedDirection)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSurfaceInteraction, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSurfaceInteraction>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sInteractionType)->AddAttributes(new xiiDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    XII_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("Prefab")),
    XII_ENUM_MEMBER_PROPERTY("Alignment", xiiSurfaceInteractionAlignment, m_Alignment),
    XII_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new xiiClampValueAttribute(xiiVariant(xiiAngle::MakeFromDegree(0.0f)), xiiVariant(xiiAngle::MakeFromDegree(90.0f)))),
    XII_MEMBER_PROPERTY("ImpulseThreshold", m_fImpulseThreshold),
    XII_MEMBER_PROPERTY("ImpulseScale", m_fImpulseScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceResourceDescriptor, 1, xiiRTTIDefaultAllocator<xiiSurfaceResourceDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("BaseSurface", m_hBaseSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface")), // Package + Thumbnail So that circular dependencies are caught.
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

const xiiRangeView<xiiStringView, xiiUInt32> xiiSurfaceInteraction::GetParameters() const
{
  return xiiRangeView<xiiStringView, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                                [this]() -> xiiUInt32 { return m_Parameters.GetCount(); },
                                                [](xiiUInt32& ref_uiIt) { ++ref_uiIt; },
                                                [this](const xiiUInt32& uiIt) -> xiiStringView { return m_Parameters.GetKey(uiIt).GetString(); });
}

void xiiSurfaceInteraction::SetParameter(xiiStringView sKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(sKey);

  auto it = m_Parameters.Find(hs);
  if (it != xiiInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void xiiSurfaceInteraction::RemoveParameter(xiiStringView sKey)
{
  m_Parameters.RemoveAndCopy(xiiTempHashedString(sKey));
}

bool xiiSurfaceInteraction::GetParameter(xiiStringView sKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Parameters.Find(sKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void xiiSurfaceResourceDescriptor::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = 0;
  ref_stream >> uiVersion;

  XII_IGNORE_UNUSED(uiVersion);

  ref_stream >> m_fPhysicsRestitution;
  ref_stream >> m_fPhysicsFrictionStatic;
  ref_stream >> m_fPhysicsFrictionDynamic;
  ref_stream >> m_hBaseSurface;
  ref_stream >> m_sOnCollideInteraction;
  ref_stream >> m_sSlideInteractionPrefab;
  ref_stream >> m_sRollInteractionPrefab;

  {
    xiiUInt32 count = 0;
    ref_stream >> count;
    m_Interactions.SetCount(count);

    xiiStringBuilder sTemp;
    for (xiiUInt32 i = 0; i < count; ++i)
    {
      auto& ia = m_Interactions[i];

      ref_stream >> sTemp;
      ia.m_sInteractionType = sTemp;

      ref_stream >> ia.m_hPrefab;
      ref_stream >> ia.m_Alignment;
      ref_stream >> ia.m_Deviation;
      ref_stream >> ia.m_fImpulseThreshold;
      ref_stream >> ia.m_fImpulseScale;

      {
        xiiUInt8 uiNumParams;
        ref_stream >> uiNumParams;

        ia.m_Parameters.Clear();
        ia.m_Parameters.Reserve(uiNumParams);

        xiiHashedString key;
        xiiVariant      value;

        for (xiiUInt32 i2 = 0; i2 < uiNumParams; ++i2)
        {
          ref_stream >> key;
          ref_stream >> value;

          ia.m_Parameters.Insert(key, value);
        }
      }
    }
  }
}

void xiiSurfaceResourceDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8 uiVersion = 1;
  ref_stream << uiVersion;

  ref_stream << m_fPhysicsRestitution;
  ref_stream << m_fPhysicsFrictionStatic;
  ref_stream << m_fPhysicsFrictionDynamic;
  ref_stream << m_hBaseSurface;
  ref_stream << m_sOnCollideInteraction;
  ref_stream << m_sSlideInteractionPrefab;
  ref_stream << m_sRollInteractionPrefab;

  ref_stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    ref_stream << ia.m_sInteractionType;
    ref_stream << ia.m_hPrefab;
    ref_stream << ia.m_Alignment;
    ref_stream << ia.m_Deviation;
    ref_stream << ia.m_fImpulseThreshold;
    ref_stream << ia.m_fImpulseScale;

    const xiiUInt8 uiNumParams = static_cast<xiiUInt8>(ia.m_Parameters.GetCount());
    ref_stream << uiNumParams;
    for (xiiUInt32 i = 0; i < uiNumParams; ++i)
    {
      ref_stream << ia.m_Parameters.GetKey(i);
      ref_stream << ia.m_Parameters.GetValue(i);
    }
  }
}

void xiiSurfaceResourceDescriptor::SetCollisionInteraction(xiiStringView sName)
{
  m_sOnCollideInteraction.Assign(sName);
}

xiiStringView xiiSurfaceResourceDescriptor::GetCollisionInteraction() const
{
  return m_sOnCollideInteraction.GetData();
}

void xiiSurfaceResourceDescriptor::SetSlideReactionPrefabFile(xiiStringView sFile)
{
  m_sSlideInteractionPrefab.Assign(sFile);
}

xiiStringView xiiSurfaceResourceDescriptor::GetSlideReactionPrefabFile() const
{
  return m_sSlideInteractionPrefab.GetData();
}

void xiiSurfaceResourceDescriptor::SetRollReactionPrefabFile(xiiStringView sFile)
{
  m_sRollInteractionPrefab.Assign(sFile);
}

xiiStringView xiiSurfaceResourceDescriptor::GetRollReactionPrefabFile() const
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

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    XII_IGNORE_UNUSED(ref_context);
    XII_IGNORE_UNUSED(pGraph);

    pNode->RenameProperty("Base Surface", "BaseSurface");
    pNode->RenameProperty("Static Friction", "StaticFriction");
    pNode->RenameProperty("Dynamic Friction", "DynamicFriction");
  }
};

xiiSurfaceResourceDescriptorPatch_1_2 g_xiiSurfaceResourceDescriptorPatch_1_2;

XII_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResourceDescriptor);
