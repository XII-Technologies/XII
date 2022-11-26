#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/DisplayMsgComponent.h>
#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(DisplayMsgComponent, 1, xiiComponentMode::Static /* this component does not move the owner node */)
{
  //XII_BEGIN_PROPERTIES
  //{
  //}
  //XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("SampleGamePlugin"),
  }
  XII_END_ATTRIBUTES;

  // BEGIN-DOCS-CODE-SNIPPET: message-handler-block
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgSetText, OnSetText),
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnSetColor)
  }
  XII_END_MESSAGEHANDLERS;
  // END-DOCS-CODE-SNIPPET
}
XII_END_COMPONENT_TYPE
// clang-format on

DisplayMsgComponent::DisplayMsgComponent()  = default;
DisplayMsgComponent::~DisplayMsgComponent() = default;

void DisplayMsgComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
}

void DisplayMsgComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
}

void DisplayMsgComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void DisplayMsgComponent::Update()
{
  const xiiTransform ownerTransform = GetOwner()->GetGlobalTransform();

  xiiDebugRenderer::Draw3DText(GetWorld(), m_sCurrentText.GetData(), ownerTransform.m_vPosition, m_TextColor, 32);
}

// BEGIN-DOCS-CODE-SNIPPET: message-handler-impl
void DisplayMsgComponent::OnSetText(xiiMsgSetText& msg)
{
  m_sCurrentText = msg.m_sText;
}

void DisplayMsgComponent::OnSetColor(xiiMsgSetColor& msg)
{
  m_TextColor = msg.m_Color;
}
// END-DOCS-CODE-SNIPPET
