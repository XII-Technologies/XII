#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/RotorComponent.h>

float CalculateAcceleratedMovement(
  float    fDistanceInMeters,
  float    fAcceleration,
  float    fMaxVelocity,
  float    fDeceleration,
  xiiTime& ref_timeSinceStartInSec);

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRotorComponent, 3, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Axis", xiiBasisAxis, m_Axis),
    XII_MEMBER_PROPERTY("AxisDeviation", m_AxisDeviation)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(-180), xiiAngle::Degree(180))),
    XII_MEMBER_PROPERTY("DegreesToRotate", m_iDegreeToRotate),
    XII_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
    XII_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiRotorComponent::xiiRotorComponent()  = default;
xiiRotorComponent::~xiiRotorComponent() = default;

void xiiRotorComponent::Update()
{
  if (m_Flags.IsAnySet(xiiTransformComponentFlags::Running) && m_fAnimationSpeed > 0.0f)
  {
    if (m_Flags.IsAnySet(xiiTransformComponentFlags::AnimationReversed))
      m_AnimationTime -= GetWorld()->GetClock().GetTimeDiff();
    else
      m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

    if (m_iDegreeToRotate > 0)
    {
      const float fNewDistance =
        CalculateAcceleratedMovement((float)m_iDegreeToRotate, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

      xiiQuat qRotation;
      qRotation.SetFromAxisAndAngle(m_vRotationAxis, xiiAngle::Degree(fNewDistance));

      GetOwner()->SetLocalRotation(GetOwner()->GetLocalRotation() * -m_qLastRotation * qRotation);

      m_qLastRotation = qRotation;

      if (!m_Flags.IsAnySet(xiiTransformComponentFlags::AnimationReversed))
      {
        if (fNewDistance >= m_iDegreeToRotate)
        {
          if (!m_Flags.IsSet(xiiTransformComponentFlags::AutoReturnEnd))
          {
            m_Flags.Remove(xiiTransformComponentFlags::Running);
          }

          m_Flags.Add(xiiTransformComponentFlags::AnimationReversed);

          /// \todo Scripting integration
          // if (PrepareEvent("ANIMATOR_OnReachEnd"))
          // RaiseEvent();
        }
      }
      else
      {
        if (fNewDistance <= 0.0f)
        {
          if (!m_Flags.IsSet(xiiTransformComponentFlags::AutoReturnStart))
          {
            m_Flags.Remove(xiiTransformComponentFlags::Running);
          }

          m_Flags.Remove(xiiTransformComponentFlags::AnimationReversed);

          /// \todo Scripting integration
          // if (PrepareEvent("ANIMATOR_OnReachStart"))
          // RaiseEvent();
        }
      }
    }
    else
    {
      /// \todo This will probably give precision issues pretty quickly

      xiiQuat qRotation;
      qRotation.SetFromAxisAndAngle(m_vRotationAxis, xiiAngle::Degree(m_fAnimationSpeed * GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds()));

      GetOwner()->SetLocalRotation(GetOwner()->GetLocalRotation() * qRotation);
    }
  }
}

void xiiRotorComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  s << m_iDegreeToRotate;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_qLastRotation;
  s << m_AxisDeviation;
}


void xiiRotorComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = ref_stream.GetStream();

  s >> m_iDegreeToRotate;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_Axis;
  s >> m_qLastRotation;

  if (uiVersion >= 3)
  {
    s >> m_AxisDeviation;
  }
}

void xiiRotorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  switch (m_Axis)
  {
    case xiiBasisAxis::PositiveX:
      m_vRotationAxis.Set(1, 0, 0);
      break;
    case xiiBasisAxis::PositiveY:
      m_vRotationAxis.Set(0, 1, 0);
      break;
    case xiiBasisAxis::PositiveZ:
      m_vRotationAxis.Set(0, 0, 1);
      break;
    case xiiBasisAxis::NegativeX:
      m_vRotationAxis.Set(-1, 0, 0);
      break;
    case xiiBasisAxis::NegativeY:
      m_vRotationAxis.Set(0, -1, 0);
      break;
    case xiiBasisAxis::NegativeZ:
      m_vRotationAxis.Set(0, 0, -1);
      break;
  }

  if (m_AxisDeviation.GetRadian() != 0.0f)
  {
    if (m_AxisDeviation > xiiAngle::Degree(179))
    {
      m_vRotationAxis = xiiVec3::CreateRandomDirection(GetWorld()->GetRandomNumberGenerator());
    }
    else
    {
      m_vRotationAxis = xiiVec3::CreateRandomDeviation(GetWorld()->GetRandomNumberGenerator(), m_AxisDeviation, m_vRotationAxis);

      if (m_AxisDeviation.GetRadian() > 0 && GetWorld()->GetRandomNumberGenerator().Bool())
        m_vRotationAxis = -m_vRotationAxis;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiRotorComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiRotorComponentPatch_1_2() :
    xiiGraphPatch("xiiRotorComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    // Base class
    ref_context.PatchBaseClass("xiiTransformComponent", 2, true);

    // this class
    pNode->RenameProperty("Degrees to Rotate", "DegreesToRotate");
  }
};

xiiRotorComponentPatch_1_2 g_xiiRotorComponentPatch_1_2;


XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_RotorComponent);
