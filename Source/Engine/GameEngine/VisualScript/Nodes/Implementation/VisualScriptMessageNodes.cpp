#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/World.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptMessageNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_ScriptStartEvent, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_ScriptStartEvent>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Event Handler"),
    new xiiTitleAttribute("OnScriptStart"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_OUTPUT_EXECUTION_PIN("OnStart", 0),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_ScriptStartEvent::xiiVisualScriptNode_ScriptStartEvent()
{
  m_bStepNode = true;
}

xiiVisualScriptNode_ScriptStartEvent::~xiiVisualScriptNode_ScriptStartEvent() = default;

void xiiVisualScriptNode_ScriptStartEvent::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_ScriptUpdateEvent, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_ScriptUpdateEvent>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Event Handler"),
    new xiiTitleAttribute("OnScriptUpdate"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_OUTPUT_EXECUTION_PIN("OnUpdate", 0),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_ScriptUpdateEvent::xiiVisualScriptNode_ScriptUpdateEvent()
{
  m_bStepNode = true;
}

xiiVisualScriptNode_ScriptUpdateEvent::~xiiVisualScriptNode_ScriptUpdateEvent() = default;

void xiiVisualScriptNode_ScriptUpdateEvent::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);

  // Make sure to be updated again next frame
  m_bStepNode = true;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_GenericEvent, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_GenericEvent>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Event Handler"),
    new xiiTitleAttribute("Generic Event '{Message}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Message", GetMessage, SetMessage),
    XII_OUTPUT_EXECUTION_PIN("OnEvent", 0),
    XII_OUTPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Variant),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_GenericEvent::xiiVisualScriptNode_GenericEvent()  = default;
xiiVisualScriptNode_GenericEvent::~xiiVisualScriptNode_GenericEvent() = default;

void xiiVisualScriptNode_GenericEvent::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  pInstance->SetOutputPinValue(this, 0, &m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}


xiiInt32 xiiVisualScriptNode_GenericEvent::HandlesMessagesWithID() const
{
  return xiiMsgGenericEvent::GetTypeMsgId();
}

void xiiVisualScriptNode_GenericEvent::HandleMessage(xiiMessage* pMsg)
{
  xiiMsgGenericEvent& msg = *static_cast<xiiMsgGenericEvent*>(pMsg);

  if (msg.m_sMessage == m_sMessage)
  {
    m_bStepNode = true;

    m_Value = msg.m_Value;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_PhysicsTriggerEvent, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_PhysicsTriggerEvent>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Events"),
    new xiiTitleAttribute("Trigger Event '{TriggerMessage}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage),
    XII_OUTPUT_EXECUTION_PIN("OnActivated", 0),
    XII_OUTPUT_EXECUTION_PIN("OnDeactivated", 2),
    XII_OUTPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_PhysicsTriggerEvent::xiiVisualScriptNode_PhysicsTriggerEvent() = default;

xiiVisualScriptNode_PhysicsTriggerEvent::~xiiVisualScriptNode_PhysicsTriggerEvent() = default;

void xiiVisualScriptNode_PhysicsTriggerEvent::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_State == xiiTriggerState::Activated)
  {
    pInstance->SetOutputPinValue(this, 0, &m_hObject);
    pInstance->ExecuteConnectedNodes(this, 0);
  }
  else if (m_State == xiiTriggerState::Deactivated)
  {
    pInstance->SetOutputPinValue(this, 0, &m_hObject);
    pInstance->ExecuteConnectedNodes(this, 2);
  }
}


xiiInt32 xiiVisualScriptNode_PhysicsTriggerEvent::HandlesMessagesWithID() const
{
  return xiiMsgTriggerTriggered::GetTypeMsgId();
}

void xiiVisualScriptNode_PhysicsTriggerEvent::HandleMessage(xiiMessage* pMsg)
{
  xiiMsgTriggerTriggered& msg = *static_cast<xiiMsgTriggerTriggered*>(pMsg);

  if (msg.m_sMessage == m_sTriggerMessage)
  {
    m_bStepNode = true;

    m_State   = msg.m_TriggerState;
    m_hObject = msg.m_hTriggeringObject;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_InputState, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_InputState>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Input"),
    new xiiTitleAttribute("Input '{InputAction}'")
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("InputAction", m_sInputAction),
    XII_MEMBER_PROPERTY("OnlyKeyPressed", m_bOnlyKeyPressed),
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    XII_OUTPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_InputState::xiiVisualScriptNode_InputState()  = default;
xiiVisualScriptNode_InputState::~xiiVisualScriptNode_InputState() = default;

void xiiVisualScriptNode_InputState::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_hComponent.IsInvalidated())
    return;

  xiiComponent* pComponent;
  if (!pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    m_hComponent.Invalidate();
    return;
  }

  xiiInputComponent* pInput = xiiDynamicCast<xiiInputComponent*>(pComponent);
  if (pInput == nullptr)
  {
    m_hComponent.Invalidate();
    return;
  }

  const double fValue = pInput->GetCurrentInputState(m_sInputAction, m_bOnlyKeyPressed);
  pInstance->SetOutputPinValue(this, 0, &fValue);
}

void* xiiVisualScriptNode_InputState::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_hComponent;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_InputEvent, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_InputEvent>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Input"),
    new xiiTitleAttribute("InputEvent '{InputAction}'"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("InputAction", GetInputAction, SetInputAction),
    XII_OUTPUT_EXECUTION_PIN("OnPressed", 0),
    XII_OUTPUT_EXECUTION_PIN("OnDown", 1),
    XII_OUTPUT_EXECUTION_PIN("OnReleased", 2),
    XII_OUTPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
    XII_OUTPUT_DATA_PIN("Component", 1, xiiVisualScriptDataPinType::ComponentHandle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_InputEvent::xiiVisualScriptNode_InputEvent() = default;

xiiVisualScriptNode_InputEvent::~xiiVisualScriptNode_InputEvent() = default;

void xiiVisualScriptNode_InputEvent::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_State == xiiTriggerState::Activated)
  {
    pInstance->SetOutputPinValue(this, 0, &m_hSenderObject);
    pInstance->ExecuteConnectedNodes(this, 0);
  }
  else if (m_State == xiiTriggerState::Continuing)
  {
    pInstance->SetOutputPinValue(this, 0, &m_hSenderObject);
    pInstance->ExecuteConnectedNodes(this, 1);
  }
  else if (m_State == xiiTriggerState::Deactivated)
  {
    pInstance->SetOutputPinValue(this, 0, &m_hSenderObject);
    pInstance->ExecuteConnectedNodes(this, 2);
  }
}

xiiInt32 xiiVisualScriptNode_InputEvent::HandlesMessagesWithID() const
{
  return xiiMsgInputActionTriggered::GetTypeMsgId();
}

void xiiVisualScriptNode_InputEvent::HandleMessage(xiiMessage* pMsg)
{
  xiiMsgInputActionTriggered& msg = *static_cast<xiiMsgInputActionTriggered*>(pMsg);

  if (msg.m_sInputAction == m_sInputAction)
  {
    m_bStepNode = true;

    m_State            = msg.m_TriggerState;
    m_hSenderObject    = msg.m_hSenderObject;
    m_hSenderComponent = msg.m_hSenderComponent;
  }
}

//////////////////////////////////////////////////////////////////////////



XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_Implementation_VisualScriptMessageNodes);
