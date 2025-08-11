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

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour0;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour1;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour2;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour3;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour4;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour5;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour6;
  xiiRenderPipelineNodeInputColourAttachmentProviderPin m_PinColour7;
  xiiRenderPipelineNodeInputDepthAttachmentProviderPin  m_PinDepthStencil;

  xiiGALSwapChain* m_pSwapChain = nullptr;
  xiiRenderTargets m_RenderTargets;
};
