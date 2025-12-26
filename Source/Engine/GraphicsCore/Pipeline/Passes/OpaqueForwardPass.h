#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/Passes/ForwardPass.h>

class XII_GRAPHICSCORE_DLL xiiOpaqueForwardRenderPass : public xiiForwardRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOpaqueForwardRenderPass, xiiForwardRenderPass);

public:
  xiiOpaqueForwardRenderPass(xiiStringView sName = "OpaqueForwardRenderPass");

  virtual ~xiiOpaqueForwardRenderPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

protected:
  virtual void SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void SetupPermutationVariables(const xiiRenderViewContext& renderViewContext) override;
  virtual void RenderObjects(const xiiRenderViewContext& renderViewContext) override;

private:
  xiiRenderPipelineNodeInputColourAttachmentPin m_PinSSAO;

  bool                       m_bWriteDepth = true;
  xiiTexture2DResourceHandle m_hWhiteTexture;
};
