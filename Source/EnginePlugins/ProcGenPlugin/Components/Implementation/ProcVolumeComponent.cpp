#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>

namespace
{
  xiiSpatialData::Category s_ProcVolumeCategory = xiiSpatialData::RegisterCategory("ProcVolume", xiiSpatialData::Flags::None);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiProcVolumeComponent, 1)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Value", GetValue, SetValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new xiiClampValueAttribute(-64.0f, 64.0f)),
    XII_ENUM_ACCESSOR_PROPERTY("BlendMode", xiiProcGenBlendMode, GetBlendMode, SetBlendMode)->AddAttributes(new xiiDefaultValueAttribute(xiiProcGenBlendMode::Set)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgTransformChanged, OnTransformChanged)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiEvent<const xiiProcGenInternal::InvalidatedArea&> xiiProcVolumeComponent::s_AreaInvalidatedEvent;

xiiProcVolumeComponent::xiiProcVolumeComponent()  = default;
xiiProcVolumeComponent::~xiiProcVolumeComponent() = default;

void xiiProcVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();

  if (GetUniqueID() != xiiInvalidIndex)
  {
    // Only necessary in Editor
    InvalidateArea();
  }
}

void xiiProcVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (GetUniqueID() != xiiInvalidIndex)
  {
    // Only necessary in Editor
    xiiBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
    if (globalBounds.IsValid())
    {
      InvalidateArea(globalBounds.GetBox());
    }
  }

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();
}

void xiiProcVolumeComponent::SetValue(float fValue)
{
  if (m_fValue != fValue)
  {
    m_fValue = fValue;

    InvalidateArea();
  }
}

void xiiProcVolumeComponent::SetSortOrder(float fOrder)
{
  if (m_fSortOrder != fOrder)
  {
    m_fSortOrder = fOrder;

    InvalidateArea();
  }
}

void xiiProcVolumeComponent::SetBlendMode(xiiEnum<xiiProcGenBlendMode> blendMode)
{
  if (m_BlendMode != blendMode)
  {
    m_BlendMode = blendMode;

    InvalidateArea();
  }
}

void xiiProcVolumeComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  xiiStreamWriter& s = ref_stream.GetStream();

  s << m_fValue;
  s << m_fSortOrder;
  s << m_BlendMode;
}

void xiiProcVolumeComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = ref_stream.GetStream();

  s >> m_fValue;
  s >> m_fSortOrder;
  s >> m_BlendMode;
}

void xiiProcVolumeComponent::OnTransformChanged(xiiMsgTransformChanged& ref_msg)
{
  xiiBoundingBoxSphere combined = GetOwner()->GetLocalBounds();
  combined.Transform(ref_msg.m_OldGlobalTransform.GetAsMat4());

  combined.ExpandToInclude(GetOwner()->GetGlobalBounds());

  InvalidateArea(combined.GetBox());
}

void xiiProcVolumeComponent::InvalidateArea()
{
  if (!IsActiveAndInitialized())
    return;

  xiiBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  if (globalBounds.IsValid())
  {
    InvalidateArea(globalBounds.GetBox());
  }
}

void xiiProcVolumeComponent::InvalidateArea(const xiiBoundingBox& box)
{
  xiiProcGenInternal::InvalidatedArea area;
  area.m_Box    = box;
  area.m_pWorld = GetWorld();

  s_AreaInvalidatedEvent.Broadcast(area);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiProcVolumeSphereComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractVolumes, OnExtractVolumes)
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Procedural Generation"),
    new xiiSphereManipulatorAttribute("Radius"),
    new xiiSphereVisualizerAttribute("Radius", xiiColor::LimeGreen),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiProcVolumeSphereComponent::xiiProcVolumeSphereComponent()  = default;
xiiProcVolumeSphereComponent::~xiiProcVolumeSphereComponent() = default;

void xiiProcVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();
  }
}

void xiiProcVolumeSphereComponent::SetFadeOutStart(float fFadeOutStart)
{
  if (m_fFadeOutStart != fFadeOutStart)
  {
    m_fFadeOutStart = fFadeOutStart;

    InvalidateArea();
  }
}

void xiiProcVolumeSphereComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  xiiStreamWriter& s = ref_stream.GetStream();

  s << m_fRadius;
  s << m_fFadeOutStart;
}

void xiiProcVolumeSphereComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = ref_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFadeOutStart;
}

void xiiProcVolumeSphereComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(xiiBoundingSphere(xiiVec3::ZeroVector(), m_fRadius), s_ProcVolumeCategory);
}

void xiiProcVolumeSphereComponent::OnExtractVolumes(xiiMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddSphere(GetOwner()->GetGlobalTransformSimd(), m_fRadius, m_BlendMode, m_fSortOrder, m_fValue, m_fFadeOutStart);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiProcVolumeBoxComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(10.0f)), new xiiClampValueAttribute(xiiVec3(0), xiiVariant())),
    XII_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.5f)), new xiiClampValueAttribute(xiiVec3(0.0f), xiiVec3(1.0f))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractVolumes, OnExtractVolumes)
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Procedural Generation"),
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
    new xiiBoxVisualizerAttribute("Extents", 1.0f, xiiColor::LimeGreen),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiProcVolumeBoxComponent::xiiProcVolumeBoxComponent()  = default;
xiiProcVolumeBoxComponent::~xiiProcVolumeBoxComponent() = default;

void xiiProcVolumeBoxComponent::SetExtents(const xiiVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();
  }
}

void xiiProcVolumeBoxComponent::SetFadeOutStart(const xiiVec3& vFadeOutStart)
{
  if (m_vFadeOutStart != vFadeOutStart)
  {
    m_vFadeOutStart = vFadeOutStart;

    InvalidateArea();
  }
}

void xiiProcVolumeBoxComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  xiiStreamWriter& s = ref_stream.GetStream();

  s << m_vExtents;
  s << m_vFadeOutStart;
}

void xiiProcVolumeBoxComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = ref_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFadeOutStart;
}

void xiiProcVolumeBoxComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(xiiBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), s_ProcVolumeCategory);
}

void xiiProcVolumeBoxComponent::OnExtractVolumes(xiiMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddBox(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vFadeOutStart);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiProcVolumeImageComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Image", GetImageFile, SetImageFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_2D")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractVolumes, OnExtractVolumes)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiProcVolumeImageComponent::xiiProcVolumeImageComponent()  = default;
xiiProcVolumeImageComponent::~xiiProcVolumeImageComponent() = default;

void xiiProcVolumeImageComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  xiiStreamWriter& s = ref_stream.GetStream();

  s << m_hImage;
}

void xiiProcVolumeImageComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = ref_stream.GetStream();

  s >> m_hImage;
}

void xiiProcVolumeImageComponent::OnExtractVolumes(xiiMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddImage(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vFadeOutStart, m_hImage);
}

void xiiProcVolumeImageComponent::SetImageFile(const char* szFile)
{
  xiiImageDataResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiImageDataResource>(szFile);
  }

  SetImage(hResource);
}

const char* xiiProcVolumeImageComponent::GetImageFile() const
{
  if (!m_hImage.IsValid())
    return "";

  return m_hImage.GetResourceID();
}

void xiiProcVolumeImageComponent::SetImage(const xiiImageDataResourceHandle& hResource)
{
  m_hImage = hResource;
}
