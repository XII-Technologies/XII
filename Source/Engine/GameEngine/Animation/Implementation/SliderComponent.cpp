#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/SliderComponent.h>

float CalculateAcceleratedMovement(
  float    fDistanceInMeters,
  float    fAcceleration,
  float    fMaxVelocity,
  float    fDeceleration,
  xiiTime& ref_timeSinceStartInSec);

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSliderComponent, 3, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Axis", xiiBasisAxis, m_Axis)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveZ)),
    XII_MEMBER_PROPERTY("Distance", m_fDistanceToTravel)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Acceleration", m_fAcceleration)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Deceleration", m_fDeceleration)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("RandomStart", m_RandomStart)->AddAttributes(new xiiClampValueAttribute(xiiTime::MakeZero(), xiiVariant())),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiDirectionVisualizerAttribute("Axis", 1.0, xiiColor::MediumPurple, nullptr, "Distance")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSliderComponent::xiiSliderComponent()  = default;
xiiSliderComponent::~xiiSliderComponent() = default;

void xiiSliderComponent::Update()
{
  if (m_Flags.IsAnySet(xiiTransformComponentFlags::Running))
  {
    xiiVec3 vAxis;

    switch (m_Axis)
    {
      case xiiBasisAxis::PositiveX:
        vAxis.Set(1, 0, 0);
        break;
      case xiiBasisAxis::PositiveY:
        vAxis.Set(0, 1, 0);
        break;
      case xiiBasisAxis::PositiveZ:
        vAxis.Set(0, 0, 1);
        break;
      case xiiBasisAxis::NegativeX:
        vAxis.Set(-1, 0, 0);
        break;
      case xiiBasisAxis::NegativeY:
        vAxis.Set(0, -1, 0);
        break;
      case xiiBasisAxis::NegativeZ:
        vAxis.Set(0, 0, -1);
        break;
    }

    if (m_Flags.IsAnySet(xiiTransformComponentFlags::AnimationReversed))
      m_AnimationTime -= GetWorld()->GetClock().GetTimeDiff();
    else
      m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

    const float fNewDistance = CalculateAcceleratedMovement(m_fDistanceToTravel, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

    const float fDistanceDiff = fNewDistance - m_fLastDistance;

    GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + GetOwner()->GetLocalRotation() * vAxis * fDistanceDiff);

    m_fLastDistance = fNewDistance;

    if (!m_Flags.IsAnySet(xiiTransformComponentFlags::AnimationReversed))
    {
      if (fNewDistance >= m_fDistanceToTravel)
      {
        if (!m_Flags.IsSet(xiiTransformComponentFlags::AutoReturnEnd))
        {
          m_Flags.Remove(xiiTransformComponentFlags::Running);
        }

        m_Flags.Add(xiiTransformComponentFlags::AnimationReversed);

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

        // if (PrepareEvent("ANIMATOR_OnReachStart"))
        // RaiseEvent();
      }
    }
  }
}

void xiiSliderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // reset to start state
  m_fLastDistance = 0.0f;

  if (m_RandomStart.IsPositive())
  {
    m_AnimationTime = xiiTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_RandomStart.GetSeconds()));
  }
}

void xiiSliderComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fDistanceToTravel;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_fLastDistance;
  s << m_RandomStart;
}


void xiiSliderComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fDistanceToTravel;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_Axis;
  s >> m_fLastDistance;

  if (uiVersion >= 3)
  {
    s >> m_RandomStart;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiSliderComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiSliderComponentPatch_1_2() :
    xiiGraphPatch("xiiSliderComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    // Base class
    ref_context.PatchBaseClass("xiiTransformComponent", 2, true);
  }
};

xiiSliderComponentPatch_1_2 g_xiiSliderComponentPatch_1_2;


XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SliderComponent);
