#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Pipeline/Passes/FrameConstantsPass.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiFrameConstantsPass, 1, xiiRTTIDefaultAllocator<xiiFrameConstantsPass>)
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Output", m_PinOutput),
  }
  XII_END_PROPERTIES;
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiFrameConstantsPass::xiiFrameConstantsPass(xiiStringView sName) :
  xiiUtilityPipelinePass(sName)
{
}

xiiFrameConstantsPass::~xiiFrameConstantsPass()
{
  if (!m_pGlobalConstants.IsEmpty())
  {
    xiiFoundation::GetAlignedAllocator()->Deallocate(m_pGlobalConstants.GetPtr());
  }

  m_pGlobalConstants.Clear();
}

xiiResult xiiFrameConstantsPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  XII_IGNORE_UNUSED(view);
  XII_IGNORE_UNUSED(pInputs);

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiElementByteStride = sizeof(xiiGlobalConstants);
  bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::UniformBuffer | xiiGALBindFlags::ShaderResource;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
  bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  bufferDescription.m_MiscFlags           = xiiGALMiscBufferFlags::None;
  pOutputs[m_PinOutput.m_uiOutputIndex]   = xiiRenderPipelinePassResource(m_PinOutput.m_ResourceType, bufferDescription);

  return XII_SUCCESS;
}

xiiResult xiiFrameConstantsPass::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  xiiSharedPtr<xiiGALDevice>                    pDevice                   = xiiGALDevice::GetDefaultDevice();
  const xiiGALGraphicsDeviceAdapterDescription& graphicsAdapterProperties = pDevice->GetGraphicsDeviceAdapterProperties();

  m_pGlobalConstants = xiiMakeBlobPtr(reinterpret_cast<xiiGlobalConstants*>(xiiFoundation::GetAlignedAllocator()->Allocate(sizeof(xiiGlobalConstants), graphicsAdapterProperties.m_BufferProperties.m_uiConstantBufferAlignment)), 1U);

  xiiMemoryUtils::ZeroFill(m_pGlobalConstants.GetPtr(), 1U);

  if (m_pGlobalConstants.IsEmpty())
  {
    xiiLog::Error("Failed to create frame constants storage in pass {}.", GetName());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiFrameConstantsPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  XII_IGNORE_UNUSED(pInputs);

  auto pOutput = pOutputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
    return;

  {
    xiiGlobalConstants* pGlobalConstants = m_pGlobalConstants.GetPtr();

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      pGlobalConstants->CameraToScreenMatrix[i] = renderViewContext.m_pViewData->m_ProjectionMatrix[i];
      pGlobalConstants->ScreenToCameraMatrix[i] = renderViewContext.m_pViewData->m_InverseProjectionMatrix[i];
      pGlobalConstants->WorldToCameraMatrix[i]  = renderViewContext.m_pViewData->m_ViewMatrix[i];
      pGlobalConstants->CameraToWorldMatrix[i]  = renderViewContext.m_pViewData->m_InverseViewMatrix[i];
      pGlobalConstants->WorldToScreenMatrix[i]  = renderViewContext.m_pViewData->m_ViewProjectionMatrix[i];
      pGlobalConstants->ScreenToWorldMatrix[i]  = renderViewContext.m_pViewData->m_InverseViewProjectionMatrix[i];
    }

    const xiiRectFloat& viewport   = renderViewContext.m_pViewData->m_ViewPortRect;
    pGlobalConstants->ViewportSize = xiiVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);

    float fNear                  = renderViewContext.m_pCamera->GetNearPlane();
    float fFar                   = renderViewContext.m_pCamera->GetFarPlane();
    pGlobalConstants->ClipPlanes = xiiVec4(fNear, fFar, 1.0f / fFar, 0.0f);

    const bool bIsDirectionalLightShadow = renderViewContext.m_pViewData->m_CameraUsageHint == xiiCameraUsageHint::Shadow && renderViewContext.m_pCamera->IsOrthographic();
    pGlobalConstants->MaxZValue          = bIsDirectionalLightShadow ? 0.0f : xiiMath::MinValue<float>();

    pGlobalConstants->Exposure = renderViewContext.m_pCamera->GetExposure();

    // Wrap around to prevent floating point issues. A wrap around of 1000 allows all frequencies with 3 digits after the decimal.
    const double fWrapAround     = 1000.0;
    pGlobalConstants->DeltaTime  = (float)xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
    pGlobalConstants->GlobalTime = (float)xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), fWrapAround);
    pGlobalConstants->WorldTime  = (float)xiiMath::Mod(GetPipeline()->GetRenderData().GetWorldTime().GetSeconds(), fWrapAround);
  }

  renderViewContext.m_pCommandList->Begin();
  {
    xiiGALScopedDebugGroup scope(renderViewContext.m_pCommandList, GetName());

    renderViewContext.m_pCommandList->UpdateBuffer(pOutput->m_Resource.m_Buffer.m_pBuffer, 0U, xiiMakeByteArrayPtr(m_pGlobalConstants.GetPtr(), 1U));
  }
  renderViewContext.m_pCommandList->End();
}
