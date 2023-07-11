#include <GameplayPlugin/GameplayPluginPCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameplayPlugin/Effects/Shake/CameraShakeVolumeComponent.h>

xiiSpatialData::Category xiiCameraShakeVolumeComponent::SpatialDataCategory = xiiSpatialData::RegisterCategory("CameraShakeVolumes", xiiSpatialData::Flags::None);

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiCameraShakeVolumeComponent, 1)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Strength", m_fStrength),
    XII_MEMBER_PROPERTY("BurstDuration", m_BurstDuration),
    XII_ENUM_MEMBER_PROPERTY("OnFinishedAction", xiiOnComponentFinishedAction, m_OnFinishedAction),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgComponentInternalTrigger, OnTriggered),
    XII_MESSAGE_HANDLER(xiiMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects/CameraShake"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiCameraShakeVolumeComponent::xiiCameraShakeVolumeComponent()  = default;
xiiCameraShakeVolumeComponent::~xiiCameraShakeVolumeComponent() = default;

void xiiCameraShakeVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void xiiCameraShakeVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void xiiCameraShakeVolumeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_BurstDuration.IsPositive())
  {
    xiiMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("Suicide");

    PostMessage(msg, m_BurstDuration);
  }
}

void xiiCameraShakeVolumeComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_fStrength;
}

void xiiCameraShakeVolumeComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  // const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_fStrength;
}

float xiiCameraShakeVolumeComponent::ComputeForceAtGlobalPosition(const xiiSimdVec4f& vGlobalPos) const
{
  const xiiSimdTransform t        = GetOwner()->GetGlobalTransformSimd();
  const xiiSimdTransform tInv     = t.GetInverse();
  const xiiSimdVec4f     localPos = tInv.TransformPosition(vGlobalPos);

  return ComputeForceAtLocalPosition(localPos);
}

void xiiCameraShakeVolumeComponent::OnTriggered(xiiMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != xiiTempHashedString("Suicide"))
    return;

  xiiOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);

  SetActiveFlag(false);
}

void xiiCameraShakeVolumeComponent::OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg)
{
  if (m_BurstDuration.IsPositive())
  {
    xiiOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiCameraShakeVolumeSphereComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.1f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSphereVisualizerAttribute("Radius", xiiColor::SaddleBrown),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiCameraShakeVolumeSphereComponent::xiiCameraShakeVolumeSphereComponent()  = default;
xiiCameraShakeVolumeSphereComponent::~xiiCameraShakeVolumeSphereComponent() = default;

void xiiCameraShakeVolumeSphereComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  s << m_fRadius;
}

void xiiCameraShakeVolumeSphereComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  // const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

float xiiCameraShakeVolumeSphereComponent::ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const
{
  xiiSimdFloat lenScaled = vLocalPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  xiiSimdFloat forceFactor = xiiSimdFloat(1.0f) - lenScaled;

  const xiiSimdFloat force = forceFactor.Max(0.0f);

  return m_fStrength * force;
}

void xiiCameraShakeVolumeSphereComponent::SetRadius(float fVal)
{
  m_fRadius       = xiiMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiCameraShakeVolumeSphereComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(xiiBoundingSphere(xiiVec3::ZeroVector(), m_fRadius), xiiCameraShakeVolumeComponent::SpatialDataCategory);
}
