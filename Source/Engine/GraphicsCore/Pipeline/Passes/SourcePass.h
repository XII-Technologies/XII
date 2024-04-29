#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiSourcePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSourcePass, xiiRenderPipelinePass);

public:
  xiiSourcePass(xiiStringView sName = "SourcePass");
  ~xiiSourcePass();

  virtual bool      GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiEnum<xiiGALTextureFormat>   m_Format;
  xiiEnum<xiiGALMSAASampleCount> m_SampleCount;
  xiiColor                       m_ClearColor;
  bool                           m_bClear;

  xiiGALRenderPassHandle  m_hRenderPass;
  xiiGALFramebufferHandle m_hFramebuffer;
};
