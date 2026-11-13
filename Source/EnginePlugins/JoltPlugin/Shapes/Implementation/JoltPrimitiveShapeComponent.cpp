/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Math.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Shapes/JoltPrimitiveShapeComponent.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiJoltShapeComponent, 1)
XII_END_ABSTRACT_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltBoxShapeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Dimensions", GetDimensions, SetDimensions)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f)), new xiiClampValueAttribute(xiiVec3(0.001f), xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltSphereShapeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.001f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

void xiiJoltShapeComponent::OnActivated()
{
  SUPER::OnActivated();
  QueueBodyUpdate();
}

void xiiJoltShapeComponent::OnDeactivated()
{
  QueueBodyUpdate();
  SUPER::OnDeactivated();
}

void xiiJoltShapeComponent::QueueBodyUpdate()
{
  xiiJoltActorComponent::QueueAncestorBodyUpdate(GetOwner());
}

xiiJoltBoxShapeComponent::xiiJoltBoxShapeComponent()  = default;
xiiJoltBoxShapeComponent::~xiiJoltBoxShapeComponent() = default;

void xiiJoltBoxShapeComponent::SetDimensions(const xiiVec3& vDimensions)
{
  if (!vDimensions.IsValid() || vDimensions.x <= 0.0f || vDimensions.y <= 0.0f || vDimensions.z <= 0.0f)
  {
    xiiLog::Warning("Jolt box dimensions must be finite and positive.");
    return;
  }

  if (m_vDimensions != vDimensions)
  {
    m_vDimensions = vDimensions;
    if (IsActiveAndInitialized())
      QueueBodyUpdate();
  }
}

void xiiJoltBoxShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_vDimensions;
}

void xiiJoltBoxShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiVec3 vDimensions = m_vDimensions;
  inout_stream.GetStream() >> vDimensions;
  SetDimensions(vDimensions);
}

xiiJoltSphereShapeComponent::xiiJoltSphereShapeComponent()  = default;
xiiJoltSphereShapeComponent::~xiiJoltSphereShapeComponent() = default;

void xiiJoltSphereShapeComponent::SetRadius(float fRadius)
{
  if (!xiiMath::IsFinite(fRadius) || fRadius <= 0.0f)
  {
    xiiLog::Warning("Jolt sphere radius must be finite and positive.");
    return;
  }

  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;
    if (IsActiveAndInitialized())
      QueueBodyUpdate();
  }
}

void xiiJoltSphereShapeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_fRadius;
}

void xiiJoltSphereShapeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  float fRadius = m_fRadius;
  inout_stream.GetStream() >> fRadius;
  SetRadius(fRadius);
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltPrimitiveShapeComponent);
