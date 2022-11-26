#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class XII_RENDERERCORE_DLL xiiSelectionHighlightPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSelectionHighlightPass, xiiRenderPipelinePass);

public:
  xiiSelectionHighlightPass(const char* szName = "SelectionHighlightPass");
  ~xiiSelectionHighlightPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  xiiRenderPipelineNodePassThrougPin m_PinColor;
  xiiRenderPipelineNodeInputPin      m_PinDepthStencil;

  xiiShaderResourceHandle        m_hShader;
  xiiConstantBufferStorageHandle m_hConstantBuffer;

  xiiColor m_HighlightColor  = xiiColorScheme::LightUI(xiiColorScheme::Yellow);
  float    m_fOverlayOpacity = 0.1f;
};
