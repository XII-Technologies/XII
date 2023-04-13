#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Components/JoltSettingsComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltSettingsComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ObjectGravity", GetObjectGravity, SetObjectGravity)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0, 0, -9.81f))),
    XII_ACCESSOR_PROPERTY("CharacterGravity", GetCharacterGravity, SetCharacterGravity)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0, 0, -12.0f))),
    XII_ENUM_ACCESSOR_PROPERTY("SteppingMode", xiiJoltSteppingMode, GetSteppingMode, SetSteppingMode),
    XII_ACCESSOR_PROPERTY("FixedFrameRate", GetFixedFrameRate, SetFixedFrameRate)->AddAttributes(new xiiDefaultValueAttribute(60.0f), new xiiClampValueAttribute(1.0f, 1000.0f)),
    XII_ACCESSOR_PROPERTY("MaxSubSteps", GetMaxSubSteps, SetMaxSubSteps)->AddAttributes(new xiiDefaultValueAttribute(4), new xiiClampValueAttribute(1, 100)),
    XII_ACCESSOR_PROPERTY("MaxBodies", GetMaxBodies, SetMaxBodies)->AddAttributes(new xiiDefaultValueAttribute(10000), new xiiClampValueAttribute(500, 1000000)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Misc"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltSettingsComponent::xiiJoltSettingsComponent()  = default;
xiiJoltSettingsComponent::~xiiJoltSettingsComponent() = default;

void xiiJoltSettingsComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_Settings.m_vObjectGravity;
  s << m_Settings.m_vCharacterGravity;
  s << m_Settings.m_SteppingMode;
  s << m_Settings.m_fFixedFrameRate;
  s << m_Settings.m_uiMaxSubSteps;
  s << m_Settings.m_uiMaxBodies;
}


void xiiJoltSettingsComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_Settings.m_vObjectGravity;
  s >> m_Settings.m_vCharacterGravity;
  s >> m_Settings.m_SteppingMode;
  s >> m_Settings.m_fFixedFrameRate;
  s >> m_Settings.m_uiMaxSubSteps;
  s >> m_Settings.m_uiMaxBodies;
}

void xiiJoltSettingsComponent::SetObjectGravity(const xiiVec3& v)
{
  m_Settings.m_vObjectGravity = v;
  SetModified(XII_BIT(0));
}

void xiiJoltSettingsComponent::SetCharacterGravity(const xiiVec3& v)
{
  m_Settings.m_vCharacterGravity = v;
  SetModified(XII_BIT(1));
}

void xiiJoltSettingsComponent::SetSteppingMode(xiiJoltSteppingMode::Enum mode)
{
  m_Settings.m_SteppingMode = mode;
  SetModified(XII_BIT(3));
}

void xiiJoltSettingsComponent::SetFixedFrameRate(float fFixedFrameRate)
{
  m_Settings.m_fFixedFrameRate = fFixedFrameRate;
  SetModified(XII_BIT(4));
}

void xiiJoltSettingsComponent::SetMaxSubSteps(xiiUInt32 uiMaxSubSteps)
{
  m_Settings.m_uiMaxSubSteps = uiMaxSubSteps;
  SetModified(XII_BIT(5));
}

void xiiJoltSettingsComponent::SetMaxBodies(xiiUInt32 uiMaxBodies)
{
  m_Settings.m_uiMaxBodies = uiMaxBodies;
  SetModified(XII_BIT(6));
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltSettingsComponent);

