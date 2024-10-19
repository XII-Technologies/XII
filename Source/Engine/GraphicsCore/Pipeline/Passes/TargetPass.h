#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

struct xiiGALRenderTargets;

class XII_GRAPHICSCORE_DLL xiiTargetPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTargetPass, xiiRenderPipelinePass);

public:
  xiiTargetPass(xiiStringView sName = "TargetPass");
  ~xiiTargetPass();

  virtual bool                GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual xiiGALTextureHandle QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc) override;
  virtual void                Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

private:
  bool VerifyInput(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiStringView sPinName);

protected:
  xiiRenderPipelineNodeInputProviderPin m_PinColor0;
  xiiRenderPipelineNodeInputProviderPin m_PinColor1;
  xiiRenderPipelineNodeInputProviderPin m_PinColor2;
  xiiRenderPipelineNodeInputProviderPin m_PinColor3;
  xiiRenderPipelineNodeInputProviderPin m_PinColor4;
  xiiRenderPipelineNodeInputProviderPin m_PinColor5;
  xiiRenderPipelineNodeInputProviderPin m_PinColor6;
  xiiRenderPipelineNodeInputProviderPin m_PinColor7;
  xiiRenderPipelineNodeInputProviderPin m_PinDepthStencil;

  xiiGALRenderTargets   m_RenderTargets;
  xiiGALSwapChainHandle m_hSwapChain;
};
