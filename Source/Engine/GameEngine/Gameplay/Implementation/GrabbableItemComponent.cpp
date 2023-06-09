#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGrabbableItemGrabPoint, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGrabbableItemGrabPoint>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    XII_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiTransformManipulatorAttribute("LocalPosition", "LocalRotation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE


XII_BEGIN_COMPONENT_TYPE(xiiGrabbableItemComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_ACCESSOR_PROPERTY("GrabPoints", GrabPoints_GetCount, GrabPoints_GetValue, GrabPoints_SetValue, GrabPoints_Insert, GrabPoints_Remove),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGrabbableItemComponent::xiiGrabbableItemComponent()  = default;
xiiGrabbableItemComponent::~xiiGrabbableItemComponent() = default;

xiiUInt32 xiiGrabbableItemComponent::GrabPoints_GetCount() const
{
  return m_GrabPoints.GetCount();
}

xiiGrabbableItemGrabPoint xiiGrabbableItemComponent::GrabPoints_GetValue(xiiUInt32 uiIndex) const
{
  return m_GrabPoints[uiIndex];
}

void xiiGrabbableItemComponent::GrabPoints_SetValue(xiiUInt32 uiIndex, xiiGrabbableItemGrabPoint value)
{
  m_GrabPoints[uiIndex] = value;
}

void xiiGrabbableItemComponent::GrabPoints_Insert(xiiUInt32 uiIndex, xiiGrabbableItemGrabPoint value)
{
  m_GrabPoints.Insert(value, uiIndex);
}

void xiiGrabbableItemComponent::GrabPoints_Remove(xiiUInt32 uiIndex)
{
  m_GrabPoints.RemoveAtAndCopy(uiIndex);
}

void xiiGrabbableItemComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  const xiiUInt8 uiNumGrabPoints = static_cast<xiiUInt8>(m_GrabPoints.GetCount());
  s << uiNumGrabPoints;
  for (const auto& gb : m_GrabPoints)
  {
    s << gb.m_vLocalPosition;
    s << gb.m_qLocalRotation;
  }
}

void xiiGrabbableItemComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = ref_stream.GetStream();

  xiiUInt8 uiNumGrabPoints;
  s >> uiNumGrabPoints;
  m_GrabPoints.SetCount(uiNumGrabPoints);
  for (auto& gb : m_GrabPoints)
  {
    s >> gb.m_vLocalPosition;
    s >> gb.m_qLocalRotation;
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GrabbableItemComponent);
