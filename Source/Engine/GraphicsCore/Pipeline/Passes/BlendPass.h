#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

/// \brief Blends the two inputs by the given blend factor and writes the result to output.
/// Note that while the output format is taken from InputA, both inputs should have the same size and format.
class XII_GRAPHICSCORE_DLL xiiBlendPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBlendPass, xiiRenderPipelinePass);

public:
  xiiBlendPass();
  ~xiiBlendPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodeInputPin  m_PinInputA;
  xiiRenderPipelineNodeInputPin  m_PinInputB;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  float                   m_fBlendFactor = 0.5f;
  xiiShaderResourceHandle m_hShader;
};
