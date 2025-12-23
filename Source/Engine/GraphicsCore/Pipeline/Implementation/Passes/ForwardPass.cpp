#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/ForwardPass.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiForwardRenderShadingQuality, 1)
  XII_ENUM_CONSTANT(xiiForwardRenderShadingQuality::Low),
  XII_ENUM_CONSTANT(xiiForwardRenderShadingQuality::Medium),
  XII_ENUM_CONSTANT(xiiForwardRenderShadingQuality::High),
  XII_ENUM_CONSTANT(xiiForwardRenderShadingQuality::Ultra)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiForwardRenderPass, 3, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Colour", m_PinColour),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    XII_MEMBER_PROPERTY("FrameConstants", m_PinFrameConstants),
    XII_ENUM_MEMBER_PROPERTY("ShadingQuality", xiiForwardRenderShadingQuality, m_ShadingQuality),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiForwardRenderPass::xiiForwardRenderPass(xiiStringView sName) :
  xiiGraphicsPipelinePass(sName, xiiRenderPipelinePassCapabilityFlags::StereoAware)
{
}

xiiForwardRenderPass::~xiiForwardRenderPass() = default;

xiiResult xiiForwardRenderPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_ShadingQuality;

  return XII_SUCCESS;
}

xiiResult xiiForwardRenderPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_ShadingQuality;

  return XII_SUCCESS;
}

xiiResult xiiForwardRenderPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);

  // Colour attachment.
  if (pInputs[m_PinColour.m_uiInputIndex])
  {
    pOutputs[m_PinColour.m_uiOutputIndex] = *pInputs[m_PinColour.m_uiInputIndex];
  }
  else
  {
    xiiLog::Error("No colour attachment input connected to pass '{0}'!", GetName());
    return XII_FAILURE;
  }

  // Depth stencil attachment.
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

void xiiForwardRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pOutputs);

  auto pColourAttachment = pInputs[m_PinColour.m_uiInputIndex];
  if (pColourAttachment == nullptr)
    return;

  auto pDepthStencil = pInputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthStencil == nullptr)
    return;
}
