#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Debug/DebugTextComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiDebugTextComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new xiiDefaultValueAttribute("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")),
    XII_MEMBER_PROPERTY("Value0", m_fValue0),
    XII_MEMBER_PROPERTY("Value1", m_fValue1),
    XII_MEMBER_PROPERTY("Value2", m_fValue2),
    XII_MEMBER_PROPERTY("Value3", m_fValue3),
    XII_MEMBER_PROPERTY("Color", m_Color),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Utilities/Debug"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDebugTextComponent::xiiDebugTextComponent() :
  m_sText("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}"), m_Color(xiiColor::White)
{
}

xiiDebugTextComponent::~xiiDebugTextComponent() = default;

void xiiDebugTextComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sText;
  s << m_fValue0;
  s << m_fValue1;
  s << m_fValue2;
  s << m_fValue3;
  s << m_Color;
}

void xiiDebugTextComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_sText;
  s >> m_fValue0;
  s >> m_fValue1;
  s >> m_fValue2;
  s >> m_fValue3;
  s >> m_Color;
}

void xiiDebugTextComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (m_sText.IsEmpty())
    return;

  xiiStringBuilder sb;
  sb.SetFormat(m_sText, m_fValue0, m_fValue1, m_fValue2, m_fValue3);

  xiiDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), m_Color);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Debug_Implementation_DebugTextComponent);
