/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Components/Gameplay/MarkerComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMarkerComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Marker", GetMarkerType, SetMarkerType)->AddAttributes(new xiiDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnMsgUpdateLocalBounds)
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
    new xiiSphereVisualizerAttribute("Radius", xiiColor::LightSkyBlue),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMarkerComponent::xiiMarkerComponent()  = default;
xiiMarkerComponent::~xiiMarkerComponent() = default;

void xiiMarkerComponent::SetMarkerType(const char* szType)
{
  m_sMarkerType.Assign(szType);

  UpdateMarker();
}

const char* xiiMarkerComponent::GetMarkerType() const
{
  return m_sMarkerType;
}

void xiiMarkerComponent::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  UpdateMarker();
}

float xiiMarkerComponent::GetRadius() const
{
  return m_fRadius;
}

void xiiMarkerComponent::OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fRadius), m_SpatialCategory);
}

void xiiMarkerComponent::UpdateMarker()
{
  if (!m_sMarkerType.IsEmpty())
  {
    m_SpatialCategory = xiiSpatialData::RegisterCategory(m_sMarkerType.GetString(), xiiSpatialData::Flags::None);
  }
  else
  {
    m_SpatialCategory = xiiInvalidSpatialDataCategory;
  }

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiMarkerComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sMarkerType;
  s << m_fRadius;
}

void xiiMarkerComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sMarkerType;
  s >> m_fRadius;
}

void xiiMarkerComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMarker();
}

void xiiMarkerComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  GetOwner()->UpdateLocalBounds();
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_MarkerComponent);
