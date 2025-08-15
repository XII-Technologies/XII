#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/HistorySourcePass.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistorySourcePassResourceProvider, 1, xiiRTTIDefaultAllocator<xiiHistorySourcePassResourceProvider>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiHistorySourcePassResourceProvider::xiiHistorySourcePassResourceProvider() = default;

xiiHistorySourcePassResourceProvider::~xiiHistorySourcePassResourceProvider()
{
  while (!m_Data.IsEmpty())
  {
    ResetResource(m_Data.GetIterator().Key());
  }
}

void xiiHistorySourcePassResourceProvider::ResetResource(xiiStringView sResourceName)
{
  if (xiiSharedPtr<xiiGALDeviceObject>* pDeviceObject = m_Data.GetValue(sResourceName))
  {
    pDeviceObject->Clear();

    m_Data.Remove(sResourceName);
  }
}

xiiSharedPtr<xiiGALDeviceObject> xiiHistorySourcePassResourceProvider::GetOrCreateResource(xiiStringView sResourceName, const xiiRenderPipelineResourceRequest& request)
{
  bool                              bExisted;
  xiiSharedPtr<xiiGALDeviceObject>& pDeviceObject = m_Data.FindOrAdd(sResourceName, &bExisted);

  if (!bExisted)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

    if (request.IsBuffer())
    {
      pDeviceObject = pDevice->CreateBuffer(request.m_Buffer);

      if (!pDeviceObject)
      {
        xiiLog::Error("Failed to create history source pass buffer.");
      }
    }
    else if (request.IsTexture())
    {
      pDeviceObject = pDevice->CreateTexture(request.m_Texture);

      if (!pDeviceObject)
      {
        xiiLog::Error("Failed to create history source pass texture.");
      }
    }
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (pDeviceObject != nullptr)
  {
    if (request.IsBuffer())
    {
      XII_ASSERT_DEBUG(pDeviceObject->IsInstanceOf<xiiGALBuffer>(), "Invalid history source buffer type.");
    }
    else if (request.IsTexture())
    {
      XII_ASSERT_DEBUG(pDeviceObject->IsInstanceOf<xiiGALTexture>(), "Invalid history source texture type.");

      const xiiGALTextureCreationDescription& textureDescription = pDeviceObject.Downcast<xiiGALTexture>()->GetDescription();

      if (request.m_Type == xiiRenderPipelineNodePinResourceType::DepthAttachment)
      {
        XII_ASSERT_DEBUG(xiiGALResourceFormat::IsDepthFormat(textureDescription.m_Format), "Invalid history source depth attachment format.");
      }
      else if (request.m_Type == xiiRenderPipelineNodePinResourceType::ColourAttachment)
      {
        XII_ASSERT_DEBUG(!xiiGALResourceFormat::IsDepthFormat(textureDescription.m_Format), "Invalid history source colour attachment format.");
      }
    }

  }
#endif
  return pDeviceObject;
}

///////////////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistoryBufferSourcePass, 1, xiiRTTIDefaultAllocator<xiiHistoryBufferSourcePass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiHistoryBufferSourcePass::xiiHistoryBufferSourcePass(xiiStringView sName) :
  xiiCreateBufferPass(sName)
{
}

xiiHistoryBufferSourcePass::~xiiHistoryBufferSourcePass() = default;

xiiResult xiiHistoryBufferSourcePass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_SUCCEED_OR_RETURN(SUPER::GetResourceDescriptions(view, pInputs, pOutputs));

  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  pResourceProvider->ResetResource(GetName());

  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiHistoryBufferSourcePass::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  return pResourceProvider->GetOrCreateResource(GetName(), request);
}

///////////////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistoryColourAttachmentSourcePass, 1, xiiRTTIDefaultAllocator<xiiHistoryColourAttachmentSourcePass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiHistoryColourAttachmentSourcePass::xiiHistoryColourAttachmentSourcePass(xiiStringView sName) :
  xiiCreateColourAttachmentPass(sName)
{
}

xiiHistoryColourAttachmentSourcePass::~xiiHistoryColourAttachmentSourcePass() = default;

xiiResult xiiHistoryColourAttachmentSourcePass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_SUCCEED_OR_RETURN(SUPER::GetResourceDescriptions(view, pInputs, pOutputs));

  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  pResourceProvider->ResetResource(GetName());

  m_bFirstExecute = true;

  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiHistoryColourAttachmentSourcePass::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  return pResourceProvider->GetOrCreateResource(GetName(), request);
}

void xiiHistoryColourAttachmentSourcePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  if (!m_bFirstExecute)
    return;

  m_bFirstExecute = false;

  SUPER::Execute(renderViewContext, pInputs, pOutputs);
}

///////////////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHistoryDepthAttachmentSourcePass, 1, xiiRTTIDefaultAllocator<xiiHistoryDepthAttachmentSourcePass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiHistoryDepthAttachmentSourcePass::xiiHistoryDepthAttachmentSourcePass(xiiStringView sName) :
  xiiCreateDepthAttachmentPass(sName)
{
}

xiiHistoryDepthAttachmentSourcePass::~xiiHistoryDepthAttachmentSourcePass() = default;

xiiResult xiiHistoryDepthAttachmentSourcePass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_SUCCEED_OR_RETURN(SUPER::GetResourceDescriptions(view, pInputs, pOutputs));

  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  pResourceProvider->ResetResource(GetName());

  m_bFirstExecute = true;

  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALDeviceObject> xiiHistoryDepthAttachmentSourcePass::QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request)
{
  auto pResourceProvider = GetPipeline()->GetFrameDataProvider<xiiHistorySourcePassResourceProvider>();
  return pResourceProvider->GetOrCreateResource(GetName(), request);
}

void xiiHistoryDepthAttachmentSourcePass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  if (!m_bFirstExecute)
    return;

  m_bFirstExecute = false;

  SUPER::Execute(renderViewContext, pInputs, pOutputs);
}
