#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <CppProjectPlugin/Components/SampleBounceComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(SampleBounceComponent, 1 /* version */, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(90))),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("CppProject"), // Component menu group
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

SampleBounceComponent::SampleBounceComponent() = default;
SampleBounceComponent::~SampleBounceComponent() = default;

void SampleBounceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // this component doesn't need to anything for initialization
}

void SampleBounceComponent::Update()
{
  const xiiTime curTime = GetWorld()->GetClock().GetAccumulatedTime();
  const xiiAngle curAngle = curTime.AsFloatInSeconds() * m_Speed;
  const float curHeight = xiiMath::Sin(curAngle) * m_fAmplitude;

  GetOwner()->SetLocalPosition(xiiVec3(0, 0, curHeight));
}

void SampleBounceComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fAmplitude;
  s << m_Speed;
}

void SampleBounceComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fAmplitude;
  s >> m_Speed;
}
