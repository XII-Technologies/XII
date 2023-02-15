#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/BakedProbes/BakedProbesVolumeComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiBakedProbesVolumeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(10.0f)), new xiiClampValueAttribute(xiiVec3(0), xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Beta),
    new xiiCategoryAttribute("Rendering/Baking"),
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
    new xiiBoxVisualizerAttribute("Extents", 1.0f, xiiColor::OrangeRed),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiBakedProbesVolumeComponent::xiiBakedProbesVolumeComponent()  = default;
xiiBakedProbesVolumeComponent::~xiiBakedProbesVolumeComponent() = default;

void xiiBakedProbesVolumeComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void xiiBakedProbesVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void xiiBakedProbesVolumeComponent::SetExtents(const xiiVec3& extents)
{
  if (m_vExtents != extents)
  {
    m_vExtents = extents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void xiiBakedProbesVolumeComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  xiiStreamWriter& s = stream.GetStream();

  s << m_vExtents;
}

void xiiBakedProbesVolumeComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = stream.GetStream();

  s >> m_vExtents;
}

void xiiBakedProbesVolumeComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(xiiBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), xiiInvalidSpatialDataCategory);
}


XII_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesVolumeComponent);
