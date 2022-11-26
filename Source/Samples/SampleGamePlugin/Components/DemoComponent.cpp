#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <SampleGamePlugin/Components/DemoComponent.h>

// BEGIN-DOCS-CODE-SNIPPET: customcomp-reflection
// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: component-reflection
XII_BEGIN_COMPONENT_TYPE(DemoComponent, 3 /* version */, xiiComponentMode::Dynamic)
// END-DOCS-CODE-SNIPPET
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(90))),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("SampleGamePlugin"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: customcomp-basics
DemoComponent::DemoComponent()  = default;
DemoComponent::~DemoComponent() = default;

void DemoComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // this component doesn't need to anything for initialization
}

void DemoComponent::Update()
{
  const xiiTime  curTime   = GetWorld()->GetClock().GetAccumulatedTime();
  const xiiAngle curAngle  = curTime.AsFloatInSeconds() * m_Speed;
  const float    curHeight = xiiMath::Sin(curAngle) * m_fAmplitude;

  GetOwner()->SetLocalPosition(xiiVec3(0, 0, curHeight));
}

// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: component-serialize
void DemoComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fAmplitude;
  s << m_Speed;
}
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: component-deserialize
void DemoComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fAmplitude;

  if (uiVersion <= 2)
  {
    // up to version 2 the angle was stored as a float in degree
    // convert this to xiiAngle
    float fDegree;
    s >> fDegree;
    m_Speed = xiiAngle::Degree(fDegree);
  }
  else
  {
    s >> m_Speed;
  }
}
// END-DOCS-CODE-SNIPPET
