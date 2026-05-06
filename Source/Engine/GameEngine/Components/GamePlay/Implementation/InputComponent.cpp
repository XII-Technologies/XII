/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Components/Gameplay/BlackboardComponent.h>
#include <GameEngine/Components/Gameplay/InputComponent.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiInputMessageGranularity, 1)
  XII_ENUM_CONSTANT(xiiInputMessageGranularity::PressOnly),
  XII_ENUM_CONSTANT(xiiInputMessageGranularity::PressAndRelease),
  XII_ENUM_CONSTANT(xiiInputMessageGranularity::PressReleaseAndDown),
XII_END_STATIC_REFLECTED_ENUM;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgInputActionTriggered);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgInputActionTriggered, 1, xiiRTTIDefaultAllocator<xiiMsgInputActionTriggered>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("InputAction", GetInputAction, SetInputAction),
    XII_MEMBER_PROPERTY("KeyPressValue", m_fKeyPressValue),
    XII_ENUM_MEMBER_PROPERTY("TriggerState", xiiTriggerState, m_TriggerState),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiInputComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("InputSet", m_sInputSet)->AddAttributes(new xiiDynamicStringEnumAttribute("InputSet")),
    XII_ENUM_MEMBER_PROPERTY("Granularity", xiiInputMessageGranularity, m_Granularity),
    XII_MEMBER_PROPERTY("ForwardToBlackboard", m_bForwardToBlackboard),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Input"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGESENDERS
  {
    XII_MESSAGE_SENDER(m_InputEventSender)
  }
  XII_END_MESSAGESENDERS;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetCurrentInputState, In, "InputAction", In, "OnlyKeyPressed"),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiInputComponent::xiiInputComponent()  = default;
xiiInputComponent::~xiiInputComponent() = default;

static inline xiiTriggerState::Enum ToTriggerState(xiiKeyState::Enum s)
{
  switch (s)
  {
    case xiiKeyState::Pressed:
      return xiiTriggerState::Activated;

    case xiiKeyState::Released:
      return xiiTriggerState::Deactivated;

    default:
      return xiiTriggerState::Continuing;
  }
}

void xiiInputComponent::Update()
{
  if (m_sInputSet.IsEmpty())
    return;

  xiiHybridArray<xiiString, 24> AllActions;
  xiiInputManager::GetAllInputActions(m_sInputSet, AllActions);

  xiiMsgInputActionTriggered msg;

  xiiBlackboard* pBlackboard = m_bForwardToBlackboard ? xiiBlackboardComponent::FindBlackboard(GetOwner()) : nullptr;

  for (const xiiString& actionName : AllActions)
  {
    float                   fValue = 0.0f;
    const xiiKeyState::Enum state  = xiiInputManager::GetInputActionState(m_sInputSet, actionName, &fValue);

    if (pBlackboard)
    {
      pBlackboard->SetEntryValue(actionName, fValue);
    }

    if (state == xiiKeyState::Up)
      continue;
    if (state == xiiKeyState::Down && m_Granularity < xiiInputMessageGranularity::PressReleaseAndDown)
      continue;
    if (state == xiiKeyState::Released && m_Granularity == xiiInputMessageGranularity::PressOnly)
      continue;

    msg.m_TriggerState = ToTriggerState(state);
    msg.m_sInputAction.Assign(actionName);
    msg.m_fKeyPressValue = fValue;

    m_InputEventSender.SendEventMessage(msg, this, GetOwner());
  }
}

float xiiInputComponent::GetCurrentInputState(const char* szInputAction, bool bOnlyKeyPressed /*= false*/) const
{
  if (m_sInputSet.IsEmpty())
    return 0;

  float                   fValue = 0.0f;
  const xiiKeyState::Enum state  = xiiInputManager::GetInputActionState(m_sInputSet, szInputAction, &fValue);

  if (bOnlyKeyPressed && state != xiiKeyState::Pressed)
    return 0;

  return fValue;
}

void xiiInputComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sInputSet;
  s << m_Granularity;

  // version 3
  s << m_bForwardToBlackboard;
}

void xiiInputComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();


  s >> m_sInputSet;
  s >> m_Granularity;

  if (uiVersion >= 3)
  {
    s >> m_bForwardToBlackboard;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiInputComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiInputComponentPatch_1_2() :
    xiiGraphPatch("xiiInputComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Input Set", "InputSet");
  }
};

xiiInputComponentPatch_1_2 g_xiiInputComponentPatch_1_2;



XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_InputComponent);
