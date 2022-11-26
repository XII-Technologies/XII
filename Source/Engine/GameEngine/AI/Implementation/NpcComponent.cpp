#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/NpcComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiNpcComponent, 1)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("AI/Experimental"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Alpha),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiNpcComponent::xiiNpcComponent()  = default;
xiiNpcComponent::~xiiNpcComponent() = default;

void xiiNpcComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();
}

void xiiNpcComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();
}



XII_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_NpcComponent);
