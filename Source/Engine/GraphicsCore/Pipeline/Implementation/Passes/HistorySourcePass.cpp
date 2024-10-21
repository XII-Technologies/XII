#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/Passes/HistorySourcePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistorySourcePassTextureDataProvider, 1, xiiRTTIDefaultAllocator<xiiHistorySourcePassTextureDataProvider>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistorySourcePass, 1, xiiRTTIDefaultAllocator<xiiHistorySourcePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiSourceFormat, m_Format),
    XII_ENUM_MEMBER_PROPERTY("MSAA_Mode", xiiGALMSAASampleCount, m_MsaaMode),
    XII_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new xiiExposeColorAlphaAttribute())
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHistorySourcePassTextureDataProvider::xiiHistorySourcePassTextureDataProvider() = default;
xiiHistorySourcePassTextureDataProvider::~xiiHistorySourcePassTextureDataProvider()
{
  while (!m_Data.IsEmpty())
  {
    ResetTexture(m_Data.GetIterator().Key());
  }
}

void xiiHistorySourcePassTextureDataProvider::ResetTexture(xiiStringView sSourcePassName)
{
  if (xiiGALTextureHandle* pHandle = m_Data.GetValue(sSourcePassName))
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
    pDevice->DestroyTexture(*pHandle);
    m_Data.Remove(sSourcePassName);
  }
}

xiiGALTextureHandle xiiHistorySourcePassTextureDataProvider::GetOrCreateTexture(xiiStringView sSourcePassName, const xiiGALTextureCreationDescription& desc)
{
  bool                 bExisted;
  xiiGALTextureHandle& hTexture = m_Data.FindOrAdd(sSourcePassName, &bExisted);
  if (!bExisted)
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
    hTexture              = pDevice->CreateTexture(desc);
    if (hTexture.IsInvalidated())
    {
      xiiLog::Error("Failed to create history source pass texture.");
    }
  }
  return hTexture;
}


xiiHistorySourcePass::xiiHistorySourcePass(xiiStringView sName) :
  xiiRenderPipelinePass(sName, true)
{
}

xiiHistorySourcePass::~xiiHistorySourcePass() = default;

bool xiiHistorySourcePass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  auto pData = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassTextureDataProvider>();
  pData->ResetTexture(GetName());

  m_bFirstExecute                      = true;
  outputs[m_PinOutput.m_uiOutputIndex] = xiiSourcePass::GetOutputDescription(view, m_Format, m_MsaaMode);
  return true;
}

xiiGALTextureViewHandle xiiHistorySourcePass::QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc)
{
  auto pData = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassTextureDataProvider>();
  return xiiGALDevice::GetDefaultDevice()->GetTexture(pData->GetOrCreateTexture(GetName(), desc))->GetDefaultView(xiiGALTextureViewType::RenderTarget);
}

void xiiHistorySourcePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr || !m_bFirstExecute)
    return;

  m_bFirstExecute = false;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Setup render target
  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_ClearColor              = m_ClearColor;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth             = true;
  renderingSetup.m_bClearStencil           = true;

  if (xiiGALTextureFormat::IsDepthFormat(pOutput->m_Desc.m_Format))
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetTexture(pOutput->m_TextureHandle)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }

  auto pCommandEncoder = xiiRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName());
}

xiiResult xiiHistorySourcePass::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Format;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  return XII_SUCCESS;
}

xiiResult xiiHistorySourcePass::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_Format;
  inout_stream >> m_MsaaMode;
  inout_stream >> m_ClearColor;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_HistorySourcePass);
