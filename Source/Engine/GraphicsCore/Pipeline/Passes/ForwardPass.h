#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

struct xiiForwardRenderShadingQuality
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Low = 0U,
    Medium,
    High,
    Ultra,

    ENUM_OUNT,

    Default = Medium,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiForwardRenderShadingQuality);

class XII_GRAPHICSCORE_DLL xiiForwardRenderPass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiForwardRenderPass, xiiGraphicsPipelinePass);

public:
  xiiForwardRenderPass(xiiStringView sName = "ForwardRenderPass");

  virtual ~xiiForwardRenderPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  virtual void SetupResources(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs);
  virtual void SetupPermutationVariables(const xiiRenderViewContext& renderViewContext);
  virtual void SetupLighting(const xiiRenderViewContext& renderViewContext);
  virtual void RenderObjects(const xiiRenderViewContext& renderViewContext) = 0;

protected:
  xiiRenderPipelineNodePassThroughColourAttachmentPin m_PinColour;
  xiiRenderPipelineNodePassThroughDepthAttachmentPin  m_PinDepthStencil;

  xiiEnum<xiiForwardRenderShadingQuality> m_ShadingQuality;
};
