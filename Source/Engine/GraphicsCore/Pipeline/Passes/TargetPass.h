#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

#include <GraphicsCore/RenderContext/RenderTargetSetup.h>

class XII_GRAPHICSCORE_DLL xiiTargetPass : public xiiPresentPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTargetPass, xiiPresentPipelinePass);

public:
  xiiTargetPass(xiiStringView sName = "TargetPass");
  ~xiiTargetPass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColor0;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColor1;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColor2;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColor3;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColor4;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColor5;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColor6;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColor7;
  xiiRenderPipelineNodeInputDepthAttachmentProviderPin  m_PinDepthStencil;

  xiiGALSwapChain* m_pSwapChain = nullptr;
  xiiRenderTargets m_RenderTargets;
};
