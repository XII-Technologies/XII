#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/DamageMessage.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgDamage);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgDamage, 1, xiiRTTIDefaultAllocator<xiiMsgDamage>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Damage", m_fDamage),
    XII_MEMBER_PROPERTY("HitObjectName", m_sHitObjectName),
    XII_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    XII_MEMBER_PROPERTY("ImpactDirection", m_vImpactDirection),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiAutoGenVisScriptMsgSender,
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_OnDamage, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_OnDamage>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Event Handler"),
    new xiiTitleAttribute("OnDamageEvent"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Execution Pins
    XII_OUTPUT_EXECUTION_PIN("OnEvent", 0),
    // Data Pins
    XII_OUTPUT_DATA_PIN("Damage", 0, xiiVisualScriptDataPinType::Number),
    XII_OUTPUT_DATA_PIN("HitObjectName", 1, xiiVisualScriptDataPinType::String)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_OnDamage::xiiVisualScriptNode_OnDamage() {}
xiiVisualScriptNode_OnDamage::~xiiVisualScriptNode_OnDamage() {}

void xiiVisualScriptNode_OnDamage::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  XII_CHECK_AT_COMPILETIME_MSG(sizeof(m_Msg.m_fDamage) == 8, "The damage value is directly used by a visual script node, so it must be a double.");

  pInstance->SetOutputPinValue(this, 0, &m_Msg.m_fDamage);
  pInstance->SetOutputPinValue(this, 1, &m_Msg.m_sHitObjectName);
  pInstance->ExecuteConnectedNodes(this, 0);
}

xiiInt32 xiiVisualScriptNode_OnDamage::HandlesMessagesWithID() const
{
  return xiiMsgDamage::GetTypeMsgId();
}

void xiiVisualScriptNode_OnDamage::HandleMessage(xiiMessage* pMsg)
{
  xiiMsgDamage& msg = *static_cast<xiiMsgDamage*>(pMsg);

  m_Msg       = msg;
  m_bStepNode = true;
}



XII_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_DamageMessage);
