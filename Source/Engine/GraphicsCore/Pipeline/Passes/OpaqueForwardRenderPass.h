#pragma once

#include <GraphicsCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all opaque objects into the color target.
class XII_GRAPHICSCORE_DLL xiiOpaqueForwardRenderPass : public xiiForwardRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOpaqueForwardRenderPass, xiiForwardRenderPass);

public:
  xiiOpaqueForwardRenderPass(xiiStringView sName = "OpaqueForwardRenderPass");
  ~xiiOpaqueForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

protected:
  virtual void SetupResources(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void SetupPermutationVars(const xiiRenderViewContext& renderViewContext) override;

  virtual void RenderObjects(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList) override;

  xiiRenderPipelineNodeInputPin m_PinSSAO;
  // xiiRenderPipelineNodeOutputPin m_PinNormal;
  // xiiRenderPipelineNodeOutputPin m_PinSpecularColorRoughness;

  bool m_bWriteDepth = true;

  xiiTexture2DResourceHandle m_hWhiteTexture;
};
