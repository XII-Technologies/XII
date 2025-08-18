#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/DepthOnlyPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthOnlyPass, 3, xiiRTTIDefaultAllocator<xiiDepthOnlyPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_MEMBER_PROPERTY("FrameConstants", m_PinFrameConstants),
    XII_MEMBER_PROPERTY("RenderStaticObjects", m_bRenderStaticObjects)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("RenderDynamicObjects", m_bRenderDynamicObjects)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("RenderTransparentObjects", m_bRenderTransparentObjects),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiDepthOnlyPass::xiiDepthOnlyPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::StereoAware)
{
}

xiiDepthOnlyPass::~xiiDepthOnlyPass() = default;

xiiResult xiiDepthOnlyPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_bRenderStaticObjects;
  inout_stream << m_bRenderDynamicObjects;
  inout_stream << m_bRenderTransparentObjects;

  return XII_SUCCESS;
}

xiiResult xiiDepthOnlyPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_bRenderStaticObjects;
  inout_stream >> m_bRenderDynamicObjects;
  inout_stream >> m_bRenderTransparentObjects;

  return XII_SUCCESS;
}

xiiResult xiiDepthOnlyPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  if (pInputs[m_PinDepthStencil.m_uiInputIndex])
  {
    pOutputs[m_PinDepthStencil.m_uiOutputIndex] = *pInputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiDepthOnlyPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
}
