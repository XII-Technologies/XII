#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

xiiSpatialData::Category xiiWindVolumeComponent::SpatialDataCategory = xiiSpatialData::RegisterCategory("WindVolumes", xiiSpatialData::Flags::None);

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiWindVolumeComponent, 2)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Strength", xiiWindStrength, m_Strength),
    XII_MEMBER_PROPERTY("ReverseDirection", m_bReverseDirection),
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
    new xiiCategoryAttribute("Effects/Wind"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiWindVolumeComponent::xiiWindVolumeComponent()  = default;
xiiWindVolumeComponent::~xiiWindVolumeComponent() = default;

void xiiWindVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void xiiWindVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void xiiWindVolumeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_BurstDuration.IsPositive())
  {
    xiiMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("Suicide");

    PostMessage(msg, m_BurstDuration);
  }
}

void xiiWindVolumeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_Strength;
  s << m_bReverseDirection;
}

void xiiWindVolumeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_Strength;

  if (uiVersion >= 2)
  {
    s >> m_bReverseDirection;
  }
}

xiiSimdVec4f xiiWindVolumeComponent::ComputeForceAtGlobalPosition(const xiiSimdVec4f& vGlobalPos) const
{
  const xiiSimdTransform t        = GetOwner()->GetGlobalTransformSimd();
  const xiiSimdTransform tInv     = t.GetInverse();
  const xiiSimdVec4f     localPos = tInv.TransformPosition(vGlobalPos);

  const xiiSimdVec4f force = ComputeForceAtLocalPosition(localPos);

  return t.TransformDirection(force);
}

void xiiWindVolumeComponent::OnTriggered(xiiMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != xiiTempHashedString("Suicide"))
    return;

  xiiOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);

  SetActiveFlag(false);
}

void xiiWindVolumeComponent::OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg)
{
  if (m_BurstDuration.IsPositive())
  {
    xiiOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
  }
}

float xiiWindVolumeComponent::GetWindInMetersPerSecond() const
{
  return m_bReverseDirection ? -xiiWindStrength::GetInMetersPerSecond(m_Strength) : xiiWindStrength::GetInMetersPerSecond(m_Strength);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiWindVolumeSphereComponent, 1, xiiComponentMode::Static)
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
    new xiiSphereVisualizerAttribute("Radius", xiiColor::CornflowerBlue),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiWindVolumeSphereComponent::xiiWindVolumeSphereComponent()  = default;
xiiWindVolumeSphereComponent::~xiiWindVolumeSphereComponent() = default;

void xiiWindVolumeSphereComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void xiiWindVolumeSphereComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

xiiSimdVec4f xiiWindVolumeSphereComponent::ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const
{
  // TODO: could do this computation in global space

  xiiSimdFloat lenScaled = vLocalPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  xiiSimdFloat forceFactor = xiiSimdFloat(1.0f) - (lenScaled * lenScaled);

  const xiiSimdFloat force = GetWindInMetersPerSecond() * forceFactor.Max(0.0f);

  xiiSimdVec4f dir = vLocalPos;
  dir.NormalizeIfNotZero<3>();

  return dir * force;
}

void xiiWindVolumeSphereComponent::SetRadius(float fVal)
{
  m_fRadius       = xiiMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiWindVolumeSphereComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(xiiBoundingSphere(xiiVec3::MakeZero(), m_fRadius), xiiWindVolumeComponent::SpatialDataCategory);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiWindVolumeCylinderMode, 1)
  XII_ENUM_CONSTANTS(xiiWindVolumeCylinderMode::Directional, xiiWindVolumeCylinderMode::Vortex)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_COMPONENT_TYPE(xiiWindVolumeCylinderComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.1f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.1f, xiiVariant())),
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiWindVolumeCylinderMode, m_Mode),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCylinderVisualizerAttribute(xiiBasisAxis::PositiveX, "Length", "Radius", xiiColor::CornflowerBlue),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 1.0f, xiiColor::DeepSkyBlue),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiWindVolumeCylinderComponent::xiiWindVolumeCylinderComponent()  = default;
xiiWindVolumeCylinderComponent::~xiiWindVolumeCylinderComponent() = default;

void xiiWindVolumeCylinderComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fLength;
  s << m_Mode;
}

void xiiWindVolumeCylinderComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;

  s >> m_fLength;
  s >> m_Mode;
}

xiiSimdVec4f xiiWindVolumeCylinderComponent::ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const
{
  const xiiSimdFloat fCylDist = vLocalPos.x();

  if (fCylDist <= -m_fLength * 0.5f || fCylDist >= m_fLength * 0.5f)
    return xiiSimdVec4f::MakeZero();

  xiiSimdVec4f orthoDir = vLocalPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= xiiMath::Square(m_fRadius))
    return xiiSimdVec4f::MakeZero();

  if (m_Mode == xiiWindVolumeCylinderMode::Vortex)
  {
    xiiSimdVec4f forceDir = xiiSimdVec4f(1, 0, 0, 0).CrossRH(orthoDir);
    forceDir.NormalizeIfNotZero<3>();
    return forceDir * GetWindInMetersPerSecond();
  }

  return xiiSimdVec4f(GetWindInMetersPerSecond(), 0, 0);
}

void xiiWindVolumeCylinderComponent::SetRadius(float fVal)
{
  m_fRadius       = xiiMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiWindVolumeCylinderComponent::SetLength(float fVal)
{
  m_fLength = xiiMath::Max(fVal, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiWindVolumeCylinderComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  const xiiVec3 corner(m_fLength * 0.5f, m_fRadius, m_fRadius);

  msg.AddBounds(xiiBoundingBoxSphere(xiiBoundingBox(-corner, corner)), xiiWindVolumeComponent::SpatialDataCategory);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiWindVolumeConeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(45)), new xiiClampValueAttribute(xiiAngle::MakeFromDegree(1), xiiAngle::MakeFromDegree(179))),
    XII_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.1f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiConeVisualizerAttribute(xiiBasisAxis::PositiveX, "Angle", 1.0f, "Length", xiiColor::CornflowerBlue),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiWindVolumeConeComponent::xiiWindVolumeConeComponent()  = default;
xiiWindVolumeConeComponent::~xiiWindVolumeConeComponent() = default;

void xiiWindVolumeConeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fLength;
  s << m_Angle;
}

void xiiWindVolumeConeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fLength;
  s >> m_Angle;
}

xiiSimdVec4f xiiWindVolumeConeComponent::ComputeForceAtLocalPosition(const xiiSimdVec4f& vLocalPos) const
{
  const xiiSimdFloat fConeDist = vLocalPos.x();

  if (fConeDist <= xiiSimdFloat::MakeZero() || fConeDist >= m_fLength)
    return xiiSimdVec4f::MakeZero();

  // TODO: precompute base radius
  const float fBaseRadius = xiiMath::Tan(m_Angle * 0.5f) * m_fLength;

  // TODO: precompute 1/length
  const xiiSimdFloat fConeRadius = (fConeDist / xiiSimdFloat(m_fLength)) * xiiSimdFloat(fBaseRadius);

  xiiSimdVec4f orthoDir = vLocalPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= fConeRadius * fConeRadius)
    return xiiSimdVec4f::MakeZero();

  return vLocalPos.GetNormalized<3>() * GetWindInMetersPerSecond();
}

void xiiWindVolumeConeComponent::SetLength(float fVal)
{
  m_fLength = xiiMath::Max(fVal, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiWindVolumeConeComponent::SetAngle(xiiAngle val)
{
  m_Angle = xiiMath::Max(val, xiiAngle::MakeFromDegree(1.0f));

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiWindVolumeConeComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  xiiVec3 c0, c1;
  c0.x = 0;
  c0.y = -xiiMath::Tan(m_Angle * 0.5f) * m_fLength;
  c0.z = c0.y;

  c1.x = m_fLength;
  c1.y = xiiMath::Tan(m_Angle * 0.5f) * m_fLength;
  c1.z = c1.y;

  msg.AddBounds(xiiBoundingBoxSphere(xiiBoundingBox(c0, c1)), xiiWindVolumeComponent::SpatialDataCategory);
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_WindVolumeComponent);
