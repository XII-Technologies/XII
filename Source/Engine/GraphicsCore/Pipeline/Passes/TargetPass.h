#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

struct xiiGALRenderTargets;

class XII_GRAPHICSCORE_DLL xiiTargetPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTargetPass, xiiRenderPipelinePass);

public:
  xiiTargetPass(xiiStringView sName = "TargetPass");
  ~xiiTargetPass();

  const xiiGALTextureViewHandle* GetTextureViewHandle(const xiiGALRenderTargets& renderTargets, const xiiRenderPipelineNodePin* pPin);

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

private:
  bool VerifyInput(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiStringView sPinName);

protected:
  xiiRenderPipelineNodeInputPin m_PinColor0;
  xiiRenderPipelineNodeInputPin m_PinColor1;
  xiiRenderPipelineNodeInputPin m_PinColor2;
  xiiRenderPipelineNodeInputPin m_PinColor3;
  xiiRenderPipelineNodeInputPin m_PinColor4;
  xiiRenderPipelineNodeInputPin m_PinColor5;
  xiiRenderPipelineNodeInputPin m_PinColor6;
  xiiRenderPipelineNodeInputPin m_PinColor7;
  xiiRenderPipelineNodeInputPin m_PinDepthStencil;
};
