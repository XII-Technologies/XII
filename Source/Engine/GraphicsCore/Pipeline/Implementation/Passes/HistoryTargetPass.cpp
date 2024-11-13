#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/HistoryTargetPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistoryTargetPass, 1, xiiRTTIDefaultAllocator<xiiHistoryTargetPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_PinInput),
    XII_MEMBER_PROPERTY("SourcePassName", m_sSourcePassName)->AddAttributes(new xiiDefaultValueAttribute("HistorySourcePass"))
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHistoryTargetPass::xiiHistoryTargetPass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true)
{
}

xiiHistoryTargetPass::~xiiHistoryTargetPass() = default;

bool xiiHistoryTargetPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pData = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassTextureDataProvider>();
  pData->ResetTexture(m_sSourcePassName);
  return true;
}

xiiGALTextureViewHandle xiiHistoryTargetPass::QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc)
{
  auto pData = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassTextureDataProvider>();
  return xiiGALDevice::GetDefaultDevice()->GetTexture(pData->GetOrCreateTexture(m_sSourcePassName, desc))->GetDefaultView(xiiGALTextureViewType::RenderTarget);
}

void xiiHistoryTargetPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
}

xiiResult xiiHistoryTargetPass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sSourcePassName;

  return XII_SUCCESS;
}

xiiResult xiiHistoryTargetPass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sSourcePassName;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_HistoryTargetPass);
