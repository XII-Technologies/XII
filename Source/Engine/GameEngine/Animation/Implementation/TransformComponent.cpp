#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/TransformComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransformComponent, 3, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Speed", m_fAnimationSpeed), // How many units per second the animation should do.
    XII_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new xiiDefaultValueAttribute(true)), // Whether the animation should start right away.
    XII_ACCESSOR_PROPERTY("ReverseAtEnd", GetReverseAtEnd, SetReverseAtEnd)->AddAttributes(new xiiDefaultValueAttribute(true)), // If true, after coming back to the start point, the animation won't stop but turn around and continue.
    XII_ACCESSOR_PROPERTY("ReverseAtStart", GetReverseAtStart, SetReverseAtStart)->AddAttributes(new xiiDefaultValueAttribute(true)), // If true, it will not stop at the end, but turn around and continue.
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    XII_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    XII_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiTransformComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream() << m_Flags.GetValue();
  inout_stream.GetStream() << m_AnimationTime;
  inout_stream.GetStream() << m_fAnimationSpeed;
}


void xiiTransformComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiTransformComponentFlags::StorageType flags;
  inout_stream.GetStream() >> flags;
  m_Flags.SetValue(flags);

  inout_stream.GetStream() >> m_AnimationTime;
  inout_stream.GetStream() >> m_fAnimationSpeed;
}

void xiiTransformComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // reset to start state
  m_AnimationTime = xiiTime::MakeZero();
  m_Flags.AddOrRemove(xiiTransformComponentFlags::CurrentlyRunning, m_Flags.IsSet(xiiTransformComponentFlags::Running));
  m_Flags.Remove(xiiTransformComponentFlags::AnimationReversed);
}

bool xiiTransformComponent::IsRunning(void) const
{
  return m_Flags.IsAnySet(xiiTransformComponentFlags::CurrentlyRunning);
}

void xiiTransformComponent::SetRunning(bool b)
{
  m_Flags.AddOrRemove(xiiTransformComponentFlags::Running, b);
  m_Flags.AddOrRemove(xiiTransformComponentFlags::CurrentlyRunning, b);
}

bool xiiTransformComponent::GetReverseAtStart(void) const
{
  return (m_Flags.IsAnySet(xiiTransformComponentFlags::AutoReturnStart));
}

void xiiTransformComponent::SetReverseAtStart(bool b)
{
  m_Flags.AddOrRemove(xiiTransformComponentFlags::AutoReturnStart, b);
}

bool xiiTransformComponent::GetReverseAtEnd(void) const
{
  return (m_Flags.IsAnySet(xiiTransformComponentFlags::AutoReturnEnd));
}

void xiiTransformComponent::SetReverseAtEnd(bool b)
{
  m_Flags.AddOrRemove(xiiTransformComponentFlags::AutoReturnEnd, b);
}

xiiTransformComponent::xiiTransformComponent()  = default;
xiiTransformComponent::~xiiTransformComponent() = default;

void xiiTransformComponent::SetDirectionForwards(bool bForwards)
{
  m_Flags.AddOrRemove(xiiTransformComponentFlags::AnimationReversed, !bForwards);
}

void xiiTransformComponent::ToggleDirection()
{
  m_Flags.AddOrRemove(xiiTransformComponentFlags::AnimationReversed, !m_Flags.IsAnySet(xiiTransformComponentFlags::AnimationReversed));
}

bool xiiTransformComponent::IsDirectionForwards() const
{
  return !m_Flags.IsAnySet(xiiTransformComponentFlags::AnimationReversed);
}

/*! Distance should be given in meters, but can be anything else, too. E.g. "angles" or "radians". All other values need to use the same
units. For example, when distance is given in angles, acceleration has to be in "angles per square seconds". Deceleration can be positive or
negative, internally the absolute value is used. Distance, acceleration, max velocity and time need to be positive. Time is expected to be
"in seconds". The returned value is 0, if time is negative. It is clamped to fDistanceInMeters, if time is too big.
*/

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, xiiTime& ref_timeSinceStartInSec)
{
  // linear motion, if no acceleration or deceleration is present
  if ((fAcceleration <= 0.0f) && (fDeceleration <= 0.0f))
  {
    const float fDist = fMaxVelocity * (float)ref_timeSinceStartInSec.GetSeconds();

    if (fDist > fDistanceInMeters)
    {
      ref_timeSinceStartInSec = xiiTime::MakeFromSeconds(fDistanceInMeters / fMaxVelocity);
      return fDistanceInMeters;
    }

    return xiiMath::Max(0.0f, fDist);
  }

  // do some sanity-checks
  if ((ref_timeSinceStartInSec.GetSeconds() <= 0.0) || (fMaxVelocity <= 0.0f) || (fDistanceInMeters <= 0.0f))
    return 0.0f;

  // calculate the duration and distance of accelerated movement
  double fAccTime = 0.0;
  if (fAcceleration > 0.0)
    fAccTime = fMaxVelocity / fAcceleration;
  double fAccDist = fMaxVelocity * fAccTime * 0.5;

  // calculate the duration and distance of decelerated movement
  double fDecTime = 0.0f;
  if (fDeceleration > 0.0f)
    fDecTime = fMaxVelocity / fDeceleration;
  double fDecDist = fMaxVelocity * fDecTime * 0.5f;

  // if acceleration and deceleration take longer, than the whole path is long
  if (fAccDist + fDecDist > fDistanceInMeters)
  {
    double fFactor = fDistanceInMeters / (fAccDist + fDecDist);

    // shorten the acceleration path
    if (fAcceleration > 0.0f)
    {
      fAccDist *= fFactor;
      fAccTime = xiiMath::Sqrt(2 * fAccDist / fAcceleration);
    }

    // shorten the deceleration path
    if (fDeceleration > 0.0f)
    {
      fDecDist *= fFactor;
      fDecTime = xiiMath::Sqrt(2 * fDecDist / fDeceleration);
    }
  }

  // if the time is still within the acceleration phase, return accelerated distance
  if (ref_timeSinceStartInSec.GetSeconds() <= fAccTime)
    return static_cast<float>(0.5 * fAcceleration * xiiMath::Square(ref_timeSinceStartInSec.GetSeconds()));

  // calculate duration and length of the path, that has maximum velocity
  const double fMaxVelDistance = fDistanceInMeters - (fAccDist + fDecDist);
  const double fMaxVelTime     = fMaxVelDistance / fMaxVelocity;

  // if the time is within this phase, return the accelerated path plus the constant velocity path
  if (ref_timeSinceStartInSec.GetSeconds() <= fAccTime + fMaxVelTime)
    return static_cast<float>(fAccDist + (ref_timeSinceStartInSec.GetSeconds() - fAccTime) * fMaxVelocity);

  // if the time is, however, outside the whole path, just return the upper end
  if (ref_timeSinceStartInSec.GetSeconds() >= fAccTime + fMaxVelTime + fDecTime)
  {
    ref_timeSinceStartInSec = xiiTime::MakeFromSeconds(fAccTime + fMaxVelTime + fDecTime); // clamp the time
    return fDistanceInMeters;
  }

  // calculate the time into the decelerated movement
  const double fDecTime2 = ref_timeSinceStartInSec.GetSeconds() - (fAccTime + fMaxVelTime);

  // return the distance with the decelerated movement
  return static_cast<float>(fDistanceInMeters - 0.5 * fDeceleration * xiiMath::Square(fDecTime - fDecTime2));
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiTransformComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiTransformComponentPatch_1_2() :
    xiiGraphPatch("xiiTransformComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Run at Startup", "RunAtStartup");
    pNode->RenameProperty("Reverse at Start", "ReverseAtStart");
    pNode->RenameProperty("Reverse at End", "ReverseAtEnd");
  }
};

xiiTransformComponentPatch_1_2 g_xiiTransformComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class xiiTransformComponentPatch_2_3 : public xiiGraphPatch
{
public:
  xiiTransformComponentPatch_2_3() :
    xiiGraphPatch("xiiTransformComponent", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("RunAtStartup", "Running");
  }
};

xiiTransformComponentPatch_2_3 g_xiiTransformComponentPatch_2_3;

XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_TransformComponent);
