#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Depth aware blur on input and writes it to an output buffer of the same format.
///
/// In theory it is mathematical nonsense to separate a bilateral blur, but it is common praxis and works good enough.
/// (Thus the name "separated" in contrast to "separable")
class XII_RENDERERCORE_DLL xiiSeparatedBilateralBlurPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSeparatedBilateralBlurPass, xiiRenderPipelinePass);

public:
  xiiSeparatedBilateralBlurPass();
  ~xiiSeparatedBilateralBlurPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

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

  xiiUInt32                      m_uiRadius;
  float                          m_fGaussianSigma;
  float                          m_fSharpness;
  xiiConstantBufferStorageHandle m_hBilateralBlurCB;
  xiiShaderResourceHandle        m_hShader;
};
