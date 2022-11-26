#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiStageSpaceComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("StageSpace", xiiXRStageSpace, GetStageSpace, SetStageSpace)->AddAttributes(new xiiDefaultValueAttribute((xiiInt32)xiiXRStageSpace::Enum::Standing)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("XR"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Beta),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiStageSpaceComponent::xiiStageSpaceComponent()  = default;
xiiStageSpaceComponent::~xiiStageSpaceComponent() = default;

xiiEnum<xiiXRStageSpace> xiiStageSpaceComponent::GetStageSpace() const
{
  return m_Space;
}

void xiiStageSpaceComponent::SetStageSpace(xiiEnum<xiiXRStageSpace> space)
{
  m_Space = space;
}

void xiiStageSpaceComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_Space;
}

void xiiStageSpaceComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = stream.GetStream();

  s >> m_Space;
}

void xiiStageSpaceComponent::OnActivated() {}

void xiiStageSpaceComponent::OnDeactivated() {}

XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_StageSpaceComponent);
