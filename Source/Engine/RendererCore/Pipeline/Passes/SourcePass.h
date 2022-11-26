#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

class XII_RENDERERCORE_DLL xiiSourcePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSourcePass, xiiRenderPipelinePass);

public:
  xiiSourcePass(const char* szName = "SourcePass");
  ~xiiSourcePass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiGALResourceFormat::Enum  m_Format;
  xiiGALMSAASampleCount::Enum m_MsaaMode;
  xiiColor                    m_ClearColor;
  bool                        m_bClear;
};
