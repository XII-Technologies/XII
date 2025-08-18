#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/HistoryTargetPass.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistoryBufferTargetPass, 1, xiiRTTIDefaultAllocator<xiiHistoryBufferTargetPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("SourcePassName", m_sSourcePassName)->AddAttributes(new xiiDefaultValueAttribute("HistoryBufferSourcePass")),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHistoryBufferTargetPass::xiiHistoryBufferTargetPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiHistoryBufferTargetPass::~xiiHistoryBufferTargetPass() = default;

xiiResult xiiHistoryBufferTargetPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sSourcePassName;

  return XII_SUCCESS;
}

xiiResult xiiHistoryBufferTargetPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sSourcePassName;

  return XII_SUCCESS;
}

xiiResult xiiHistoryBufferTargetPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  pResourceProvider->ResetResource(GetName());

  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiHistoryBufferTargetPass::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  return pResourceProvider->GetOrCreateResource(m_sSourcePassName, request);
}

void xiiHistoryBufferTargetPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);
  XII_IGNORE_UNUSED(pInputs);
  XII_IGNORE_UNUSED(pOutputs);
}

///////////////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistoryColourAttachmentTargetPass, 1, xiiRTTIDefaultAllocator<xiiHistoryColourAttachmentTargetPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("SourcePassName", m_sSourcePassName)->AddAttributes(new xiiDefaultValueAttribute("HistoryColourAttachmentSourcePass")),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHistoryColourAttachmentTargetPass::xiiHistoryColourAttachmentTargetPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiHistoryColourAttachmentTargetPass::~xiiHistoryColourAttachmentTargetPass() = default;

xiiResult xiiHistoryColourAttachmentTargetPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sSourcePassName;

  return XII_SUCCESS;
}

xiiResult xiiHistoryColourAttachmentTargetPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sSourcePassName;

  return XII_SUCCESS;
}

xiiResult xiiHistoryColourAttachmentTargetPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  pResourceProvider->ResetResource(GetName());

  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiHistoryColourAttachmentTargetPass::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  return pResourceProvider->GetOrCreateResource(m_sSourcePassName, request);
}

void xiiHistoryColourAttachmentTargetPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);
  XII_IGNORE_UNUSED(pInputs);
  XII_IGNORE_UNUSED(pOutputs);
}

///////////////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistoryDepthAttachmentTargetPass, 1, xiiRTTIDefaultAllocator<xiiHistoryDepthAttachmentTargetPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("SourcePassName", m_sSourcePassName)->AddAttributes(new xiiDefaultValueAttribute("HistoryDepthAttachmentSourcePass")),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHistoryDepthAttachmentTargetPass::xiiHistoryDepthAttachmentTargetPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiHistoryDepthAttachmentTargetPass::~xiiHistoryDepthAttachmentTargetPass() = default;

xiiResult xiiHistoryDepthAttachmentTargetPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sSourcePassName;

  return XII_SUCCESS;
}

xiiResult xiiHistoryDepthAttachmentTargetPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sSourcePassName;

  return XII_SUCCESS;
}

xiiResult xiiHistoryDepthAttachmentTargetPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  pResourceProvider->ResetResource(GetName());

  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiHistoryDepthAttachmentTargetPass::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  return pResourceProvider->GetOrCreateResource(m_sSourcePassName, request);
}

void xiiHistoryDepthAttachmentTargetPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(renderViewContext);
  XII_IGNORE_UNUSED(pInputs);
  XII_IGNORE_UNUSED(pOutputs);
}
