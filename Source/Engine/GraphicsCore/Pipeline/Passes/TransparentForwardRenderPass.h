#pragma once

#include <GraphicsCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all transparent objects into the color target.
class XII_GRAPHICSCORE_DLL xiiTransparentForwardRenderPass : public xiiForwardRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransparentForwardRenderPass, xiiForwardRenderPass);

public:
  xiiTransparentForwardRenderPass(xiiStringView sName = "TransparentForwardRenderPass");
  ~xiiTransparentForwardRenderPass();

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  virtual void SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void RenderObjects(const xiiRenderViewContext& renderViewContext) override;

  void UpdateSceneColorTexture(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALTexture> pSceneColorTexture, xiiSharedPtr<xiiGALTexture> pCurrentColorTexture);
  void CreateSampler();

  xiiRenderPipelineNodeInputPin m_PinResolvedDepth;

  xiiSharedPtr<xiiGALSampler> m_pSceneColorSampler;
};
