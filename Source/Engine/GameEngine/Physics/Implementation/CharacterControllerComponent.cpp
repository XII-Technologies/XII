#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgMoveCharacterController);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgMoveCharacterController, 1, xiiRTTIDefaultAllocator<xiiMsgMoveCharacterController>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MoveForwards", m_fMoveForwards),
    XII_MEMBER_PROPERTY("MoveBackwards", m_fMoveBackwards),
    XII_MEMBER_PROPERTY("StrafeLeft", m_fStrafeLeft),
    XII_MEMBER_PROPERTY("StrafeRight", m_fStrafeRight),
    XII_MEMBER_PROPERTY("RotateLeft", m_fRotateLeft),
    XII_MEMBER_PROPERTY("RotateRight", m_fRotateRight),
    XII_MEMBER_PROPERTY("Run", m_bRun),
    XII_MEMBER_PROPERTY("Jump", m_bJump),
    XII_MEMBER_PROPERTY("Crouch", m_bCrouch),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiAutoGenVisScriptMsgSender
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiCharacterControllerComponent, 1)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(RawMove, In, "moveDeltaGlobal"),
    XII_SCRIPT_FUNCTION_PROPERTY(TeleportCharacter, In, "globalFootPosition"),
    XII_SCRIPT_FUNCTION_PROPERTY(IsDestinationUnobstructed, In, "globalFootPosition", In, "characterHeight"),
    XII_SCRIPT_FUNCTION_PROPERTY(IsTouchingGround),
    XII_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgMoveCharacterController, MoveCharacter),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiCharacterControllerComponent::xiiCharacterControllerComponent() {}

void xiiCharacterControllerComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  // auto& s = stream.GetStream();
}

void xiiCharacterControllerComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}



XII_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_CharacterControllerComponent);
