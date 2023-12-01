#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ConstantBufferStorage.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiSelectionHighlightPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSelectionHighlightPass, xiiRenderPipelinePass);

public:
  xiiSelectionHighlightPass(const char* szName = "SelectionHighlightPass");
  ~xiiSelectionHighlightPass();

  virtual bool      GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodePassThrougPin m_PinColor;
  xiiRenderPipelineNodeInputPin      m_PinDepthStencil;

  xiiShaderResourceHandle        m_hShader;
  xiiConstantBufferStorageHandle m_hConstantBuffer;

  xiiColor m_HighlightColor  = xiiColorScheme::LightUI(xiiColorScheme::Yellow);
  float    m_fOverlayOpacity = 0.1f;
};
