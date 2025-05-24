#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

/// \brief Depth aware blur on input and writes it to an output buffer of the same format.
///
/// In theory it is mathematical nonsense to separate a bilateral blur, but it is common praxis and works good enough.
/// (Thus the name "separated" in contrast to "separable")
class XII_GRAPHICSCORE_DLL xiiSeparatedBilateralBlurPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSeparatedBilateralBlurPass, xiiRenderPipelinePass);

public:
  xiiSeparatedBilateralBlurPass();
  ~xiiSeparatedBilateralBlurPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  void      SetRadius(xiiUInt32 uiRadius);
  xiiUInt32 GetRadius() const;

  void  SetGaussianSigma(float fSigma);
  float GetGaussianSigma() const;

  void  SetSharpness(float fSharpness);
  float GetSharpness() const;

protected:
  xiiRenderPipelineNodeInputPin  m_PinBlurSourceInput;
  xiiRenderPipelineNodeInputPin  m_PinDepthInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiUInt32                      m_uiRadius       = 7;
  float                          m_fGaussianSigma = 3.5f;
  float                          m_fSharpness     = 120.0f;
  xiiConstantBufferStorageHandle m_hBilateralBlurCB;
  xiiShaderResourceHandle        m_hShader;
};
