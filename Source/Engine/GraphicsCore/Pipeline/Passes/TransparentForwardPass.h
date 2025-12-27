#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/Passes/ForwardPass.h>

class XII_GRAPHICSCORE_DLL xiiTransparentForwardRenderPass : public xiiForwardRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransparentForwardRenderPass, xiiForwardRenderPass);

public:
  xiiTransparentForwardRenderPass(xiiStringView sName = "TransparentForwardRenderPass");

  virtual ~xiiTransparentForwardRenderPass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  virtual void SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;
  virtual void RenderObjects(const xiiRenderViewContext& renderViewContext) override;

  void UpdateSceneColorTexture(xiiSharedPtr<xiiGALTexture> pSceneColorTexture, xiiSharedPtr<xiiGALTexture> pCurrentColorTexture);

private:
  xiiRenderPipelineNodeInputColourAttachmentPin m_PinResolvedDepth;
  xiiRenderPipelineNodeInputSamplerPin          m_PinSceneColourSampler;
};
