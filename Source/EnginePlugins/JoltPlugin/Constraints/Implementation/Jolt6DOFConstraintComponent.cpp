#include <JoltPlugin/JoltPluginPCH.h>

#if 0

#  include <Core/WorldSerializer/WorldReader.h>
#  include <Core/WorldSerializer/WorldWriter.h>
#  include <JoltPlugin/Constraints/Jolt6DOFConstraintComponent.h>
#  include <JoltPlugin/System/JoltCore.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiJoltAxis, 1)
  XII_BITFLAGS_CONSTANT(xiiJoltAxis::X),
  XII_BITFLAGS_CONSTANT(xiiJoltAxis::Y),
  XII_BITFLAGS_CONSTANT(xiiJoltAxis::Z),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_COMPONENT_TYPE(xiiJolt6DOFConstraintComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_ACCESSOR_PROPERTY("FreeLinearAxis", xiiJoltAxis, GetFreeLinearAxis, SetFreeLinearAxis),
    XII_ENUM_ACCESSOR_PROPERTY("LinearLimitMode", xiiJoltConstraintLimitMode, GetLinearLimitMode, SetLinearLimitMode),
    XII_ACCESSOR_PROPERTY("LinearRangeX", GetLinearRangeX, SetLinearRangeX),
    XII_ACCESSOR_PROPERTY("LinearRangeY", GetLinearRangeY, SetLinearRangeY),
    XII_ACCESSOR_PROPERTY("LinearRangeZ", GetLinearRangeZ, SetLinearRangeZ),
    XII_ACCESSOR_PROPERTY("LinearStiffness", GetLinearStiffness, SetLinearStiffness)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("LinearDamping", GetLinearDamping, SetLinearDamping)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_BITFLAGS_ACCESSOR_PROPERTY("FreeAngularAxis", xiiJoltAxis, GetFreeAngularAxis, SetFreeAngularAxis),
    XII_ENUM_ACCESSOR_PROPERTY("SwingLimitMode", xiiJoltConstraintLimitMode, GetSwingLimitMode, SetSwingLimitMode),
    XII_ACCESSOR_PROPERTY("SwingLimit", GetSwingLimit, SetSwingLimit)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::Degree(175))),
    XII_ACCESSOR_PROPERTY("SwingStiffness", GetSwingStiffness, SetSwingStiffness)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("SwingDamping", GetSwingDamping, SetSwingDamping)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ENUM_ACCESSOR_PROPERTY("TwistLimitMode", xiiJoltConstraintLimitMode, GetTwistLimitMode, SetTwistLimitMode),
    XII_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new xiiClampValueAttribute(-xiiAngle::Degree(175), xiiAngle::Degree(175))),
    XII_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new xiiClampValueAttribute(-xiiAngle::Degree(175), xiiAngle::Degree(175))),
    XII_ACCESSOR_PROPERTY("TwistStiffness", GetTwistStiffness, SetTwistStiffness)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("TwistDamping", GetTwistDamping, SetTwistDamping)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.2, xiiColor::SlateGray)
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJolt6DOFConstraintComponent::xiiJolt6DOFConstraintComponent() = default;
xiiJolt6DOFConstraintComponent::~xiiJolt6DOFConstraintComponent() = default;

void xiiJolt6DOFConstraintComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_FreeLinearAxis;
  s << m_FreeAngularAxis;
  s << m_fLinearStiffness;
  s << m_fLinearDamping;
  s << m_fSwingStiffness;
  s << m_fSwingDamping;

  s << m_LinearLimitMode;
  s << m_vLinearRangeX;
  s << m_vLinearRangeY;
  s << m_vLinearRangeZ;

  s << m_SwingLimitMode;
  s << m_SwingLimit;

  s << m_TwistLimitMode;
  s << m_LowerTwistLimit;
  s << m_UpperTwistLimit;
  s << m_fTwistStiffness;
  s << m_fTwistDamping;
}

void xiiJolt6DOFConstraintComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_FreeLinearAxis;
  s >> m_FreeAngularAxis;
  s >> m_fLinearStiffness;
  s >> m_fLinearDamping;
  s >> m_fSwingStiffness;
  s >> m_fSwingDamping;

  s >> m_LinearLimitMode;
  s >> m_vLinearRangeX;
  s >> m_vLinearRangeY;
  s >> m_vLinearRangeZ;
  s >> m_SwingLimitMode;
  s >> m_SwingLimit;

  s >> m_TwistLimitMode;
  s >> m_LowerTwistLimit;
  s >> m_UpperTwistLimit;
  s >> m_fTwistStiffness;
  s >> m_fTwistDamping;
}

void xiiJolt6DOFConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  //XII_ASSERT_DEV(localFrame0.isFinite() && localFrame0.isValid() && localFrame0.isSane(), "frame 0");
  //XII_ASSERT_DEV(localFrame1.isFinite() && localFrame1.isValid() && localFrame1.isSane(), "frame 1");
  //
  //  m_pJoint = PxD6JointCreate(*(xiiJolt::GetSingleton()->GetJoltAPI()), actor0, localFrame0, actor1, localFrame1);
}

void xiiJolt6DOFConstraintComponent::ApplySettings()
{
  xiiJoltConstraintComponent::ApplySettings();

  //JoltD6Joint* pJoint = static_cast<PxD6Joint*>(m_pJoint);

  //if (m_LinearLimitMode == xiiJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eX, m_FreeLinearAxis.IsSet(xiiJoltAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eY, m_FreeLinearAxis.IsSet(xiiJoltAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eZ, m_FreeLinearAxis.IsSet(xiiJoltAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeLinearAxis;

  //  if (m_LinearLimitMode == xiiJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (xiiMath::IsEqual(m_vLinearRangeX.x, m_vLinearRangeX.y, 0.05f))
  //      freeAxis.Remove(xiiJoltAxis::X);
  //    if (xiiMath::IsEqual(m_vLinearRangeY.x, m_vLinearRangeY.y, 0.05f))
  //      freeAxis.Remove(xiiJoltAxis::Y);
  //    if (xiiMath::IsEqual(m_vLinearRangeZ.x, m_vLinearRangeZ.y, 0.05f))
  //      freeAxis.Remove(xiiJoltAxis::Z);
  //  }

  //  pJoint->setMotion(PxD6Axis::eX, freeAxis.IsSet(xiiJoltAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eY, freeAxis.IsSet(xiiJoltAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eZ, freeAxis.IsSet(xiiJoltAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  PxJointLinearLimitPair l(0, 0, PxSpring(0, 0));

  //  if (m_LinearLimitMode == xiiJoltConstraintLimitMode::SoftLimit)
  //  {
  //    l.stiffness = m_fLinearStiffness;
  //    l.damping = m_fLinearDamping;
  //  }
  //  else
  //  {
  //    l.restitution = m_fLinearStiffness;
  //    l.bounceThreshold = m_fLinearDamping;
  //  }

  //  if (freeAxis.IsSet(xiiJoltAxis::X))
  //  {
  //    l.lower = m_vLinearRangeX.x;
  //    l.upper = m_vLinearRangeX.y;

  //    if (l.lower > l.upper)
  //      xiiMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eX, l);
  //  }

  //  if (freeAxis.IsSet(xiiJoltAxis::Y))
  //  {
  //    l.lower = m_vLinearRangeY.x;
  //    l.upper = m_vLinearRangeY.y;

  //    if (l.lower > l.upper)
  //      xiiMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eY, l);
  //  }

  //  if (freeAxis.IsSet(xiiJoltAxis::Z))
  //  {
  //    l.lower = m_vLinearRangeZ.x;
  //    l.upper = m_vLinearRangeZ.y;

  //    if (l.lower > l.upper)
  //      xiiMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eZ, l);
  //  }
  //}


  //if (m_SwingLimitMode == xiiJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eSWING1, m_FreeAngularAxis.IsSet(xiiJoltAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eSWING2, m_FreeAngularAxis.IsSet(xiiJoltAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeAngularAxis;

  //  if (m_SwingLimitMode == xiiJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (xiiMath::IsZero(m_SwingLimit.GetDegree(), 1.0f))
  //    {
  //      freeAxis.Remove(xiiJoltAxis::Y);
  //      freeAxis.Remove(xiiJoltAxis::Z);
  //    }
  //  }

  //  pJoint->setMotion(PxD6Axis::eSWING1, freeAxis.IsSet(xiiJoltAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eSWING2, freeAxis.IsSet(xiiJoltAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  if (freeAxis.IsAnySet(xiiJoltAxis::Y | xiiJoltAxis::Z))
  //  {
  //    const float fSwingLimit = xiiMath::Max(xiiAngle::Degree(0.5f).GetRadian(), m_SwingLimit.GetRadian());

  //    PxJointLimitCone l(fSwingLimit, fSwingLimit);

  //    if (m_SwingLimitMode == xiiJoltConstraintLimitMode::SoftLimit)
  //    {
  //      l.stiffness = m_fSwingStiffness;
  //      l.damping = m_fSwingDamping;
  //    }
  //    else
  //    {
  //      l.restitution = m_fSwingStiffness;
  //      l.bounceThreshold = m_fSwingDamping;
  //    }

  //    pJoint->setSwingLimit(l);
  //  }
  //}

  //if (m_TwistLimitMode == xiiJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eTWIST, m_FreeAngularAxis.IsSet(xiiJoltAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeAngularAxis;

  //  if (m_SwingLimitMode == xiiJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (xiiMath::IsEqual(m_LowerTwistLimit.GetDegree(), m_UpperTwistLimit.GetDegree(), 1.0f))
  //    {
  //      freeAxis.Remove(xiiJoltAxis::X);
  //    }
  //  }

  //  pJoint->setMotion(PxD6Axis::eTWIST, freeAxis.IsSet(xiiJoltAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  if (freeAxis.IsSet(xiiJoltAxis::X))
  //  {
  //    PxJointAngularLimitPair l(m_LowerTwistLimit.GetRadian(), m_UpperTwistLimit.GetRadian());

  //    if (l.lower > l.upper)
  //    {
  //      xiiMath::Swap(l.lower, l.upper);
  //    }

  //    if (xiiMath::IsEqual(l.lower, l.upper, xiiAngle::Degree(0.5f).GetRadian()))
  //    {
  //      l.lower -= xiiAngle::Degree(0.5f).GetRadian();
  //      l.upper += xiiAngle::Degree(0.5f).GetRadian();
  //    }

  //    if (m_TwistLimitMode == xiiJoltConstraintLimitMode::SoftLimit)
  //    {
  //      l.stiffness = m_fTwistStiffness;
  //      l.damping = m_fTwistDamping;
  //    }
  //    else
  //    {
  //      l.restitution = m_fTwistStiffness;
  //      l.bounceThreshold = m_fTwistDamping;
  //    }

  //    pJoint->setTwistLimit(l);
  //  }
  //}
}

void xiiJolt6DOFConstraintComponent::SetFreeLinearAxis(xiiBitflags<xiiJoltAxis> flags)
{
  m_FreeLinearAxis = flags;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetFreeAngularAxis(xiiBitflags<xiiJoltAxis> flags)
{
  m_FreeAngularAxis = flags;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetLinearLimitMode(xiiJoltConstraintLimitMode::Enum mode)
{
  m_LinearLimitMode = mode;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetLinearRangeX(const xiiVec2& value)
{
  m_vLinearRangeX = value;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetLinearRangeY(const xiiVec2& value)
{
  m_vLinearRangeY = value;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetLinearRangeZ(const xiiVec2& value)
{
  m_vLinearRangeZ = value;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetLinearStiffness(float f)
{
  m_fLinearStiffness = f;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetLinearDamping(float f)
{
  m_fLinearDamping = f;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetSwingLimitMode(xiiJoltConstraintLimitMode::Enum mode)
{
  m_SwingLimitMode = mode;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetSwingLimit(xiiAngle f)
{
  m_SwingLimit = f;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetSwingStiffness(float f)
{
  m_fSwingStiffness = f;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetSwingDamping(float f)
{
  m_fSwingDamping = f;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetTwistLimitMode(xiiJoltConstraintLimitMode::Enum mode)
{
  m_TwistLimitMode = mode;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetLowerTwistLimit(xiiAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetUpperTwistLimit(xiiAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetTwistStiffness(float f)
{
  m_fTwistStiffness = f;
  QueueApplySettings();
}

void xiiJolt6DOFConstraintComponent::SetTwistDamping(float f)
{
  m_fTwistDamping = f;
  QueueApplySettings();
}

#endif


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_Jolt6DOFConstraintComponent);

