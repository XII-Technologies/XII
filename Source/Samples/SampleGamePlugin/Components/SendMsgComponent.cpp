#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/SendMsgComponent.h>
#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(SendMsgComponent, 1, xiiComponentMode::Static /* this component does not move the owner node */)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Strings", m_TextArray)
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("SampleGamePlugin"),
  }
  XII_END_ATTRIBUTES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgComponentInternalTrigger, OnSendText)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

SendMsgComponent::SendMsgComponent()  = default;
SendMsgComponent::~SendMsgComponent() = default;

void SendMsgComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s.WriteArray(m_TextArray).IgnoreResult();
}

void SendMsgComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  auto& s = stream.GetStream();

  s.ReadArray(m_TextArray).IgnoreResult();
}

static xiiHashedString s_sSendNextString = xiiMakeHashedString("SendNextString");

void SendMsgComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // start sending strings shortly
  xiiMsgComponentInternalTrigger msg;
  msg.m_sMessage = s_sSendNextString;
  PostMessage(msg, xiiTime::Milliseconds(100));
}

void SendMsgComponent::OnSendText(xiiMsgComponentInternalTrigger& msg)
{
  // Note: We don't need to take care to stop when the component gets deactivated
  // because messages are only delivered to active components.
  // However, if the component got deactivated and activated again within the 2 second
  // message delay, OnSimulationStarted() above could queue a second message, and now
  // both of them would arrive. We don't handle that case here.
  // if (!IsActiveAndSimulating())
  //  return;

  if (msg.m_sMessage == s_sSendNextString)
  {
    if (!m_TextArray.IsEmpty())
    {
      const xiiUInt32 idx = m_uiNextString % m_TextArray.GetCount();

      // send the message to all components on this node and all child nodes

      xiiGameObject* pGameObject = GetOwner();

      // BEGIN-DOCS-CODE-SNIPPET: message-send-direct
      xiiMsgSetText textMsg;
      textMsg.m_sText = m_TextArray[idx];
      pGameObject->SendMessageRecursive(textMsg);
      // END-DOCS-CODE-SNIPPET

      m_uiNextString++;
    }


    // send the next string in a second
    PostMessage(msg, xiiTime::Seconds(2));
  }
}
