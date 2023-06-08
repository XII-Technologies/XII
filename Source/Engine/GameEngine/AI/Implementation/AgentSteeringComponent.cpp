#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/AgentSteeringComponent.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiAgentSteeringComponent, 1)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("AI/Experimental"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Alpha),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetTargetPosition),
    XII_SCRIPT_FUNCTION_PROPERTY(ClearTargetPosition),
    //XII_SCRIPT_FUNCTION_PROPERTY(GetPathToTargetState),
  }
  XII_END_FUNCTIONS;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiAgentSteeringComponent::xiiAgentSteeringComponent()  = default;
xiiAgentSteeringComponent::~xiiAgentSteeringComponent() = default;

void xiiAgentSteeringComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();
}

void xiiAgentSteeringComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = ref_stream.GetStream();
}



XII_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_AgentSteeringComponent);
