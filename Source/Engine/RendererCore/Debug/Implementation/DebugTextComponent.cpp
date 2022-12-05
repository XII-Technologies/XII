#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugTextComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

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
    new xiiCategoryAttribute("Debug"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDebugTextComponent::xiiDebugTextComponent() :
  m_sText("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}"), m_fValue0(0.0f), m_fValue1(0.0f), m_fValue2(0.0f), m_fValue3(0.0f), m_Color(xiiColor::White)
{
}

xiiDebugTextComponent::~xiiDebugTextComponent() = default;

void xiiDebugTextComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sText;
  s << m_fValue0;
  s << m_fValue1;
  s << m_fValue2;
  s << m_fValue3;
  s << m_Color;
}

void xiiDebugTextComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_sText;
  s >> m_fValue0;
  s >> m_fValue1;
  s >> m_fValue2;
  s >> m_fValue3;
  s >> m_Color;
}

void xiiDebugTextComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow)
    return;

  if (!m_sText.IsEmpty())
  {
    xiiStringBuilder sb;
    sb.Format(m_sText, m_fValue0, m_fValue1, m_fValue2, m_fValue3);

    xiiDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), m_Color, 16,
                                 xiiDebugRenderer::HorizontalAlignment::Center, xiiDebugRenderer::VerticalAlignment::Bottom);
  }
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugTextComponent);
