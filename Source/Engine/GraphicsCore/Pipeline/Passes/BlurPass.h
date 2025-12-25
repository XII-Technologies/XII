#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

struct XII_GRAPHICSCORE_DLL xiiBlurType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Box = 0U,    ///< Fast, uniform kernel.
    Gaussian,    ///< Separable, sigma-based.
    Kawase,      ///< Iterative, offset-based.
    DualKawase,  ///< Pyramid down/up sample.
    Bilateral,   ///< Edge-preserving, needs normal/depth.
    Bokeh,       ///< Approximate DoF blur with disc kernel.
    Directional, ///< Linear motion blur along a vector.
    Radial,      ///< Zoom or spin blur from a centre.

    ENUM_COUNT,

    Default = Box
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiBlurType);

struct XII_GRAPHICSCORE_DLL xiiBlurSettings
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiBlurType> m_Type             = xiiBlurType::Gaussian; ///< Blur type.
  float                m_fRadiusPixels    = 6.0f;                  ///< Logical radius (pre-clamped).
  float                m_fSigma           = 3.0f;                  ///< For Gaussian; ignored otherwise.
  xiiUInt32            m_uiIterationCount = 1;                     ///< For Box, Kawase; >1 improves quality.
  xiiVec2              m_vDirection       = xiiVec2::MakeZero();   ///< For Directional (normalized).
  xiiVec2              m_vRadialCenter    = {0.5f, 0.5f};          ///< For Radial (UV space).
  float                m_fRadialStrength  = 0.0f;                  ///< Zoom/spin magnitude.
  float                m_fDepthSigma      = 1.0f;                  ///< For Bilateral.
  float                m_fNormalSigma     = 1.0f;                  ///< For Bilateral.
  bool                 m_bUseDownsample   = false;                 ///< For DualKawase/Bokeh to save cost.
  xiiUInt32            m_uiMaxMip         = 2;                     ///< Pyramid depth for DualKawase.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiBlurSettings);

class XII_GRAPHICSCORE_DLL xiiBlurPass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBlurPass, xiiGraphicsPipelinePass);

public:
  xiiBlurPass(xiiStringView sName = "BlurPass");

  virtual ~xiiBlurPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  void PrepareKernel(xiiStaticArray<float, 32U>& out_weights, xiiStaticArray<float, 32U>& out_offsets);

protected:
  // Required pins
  xiiRenderPipelineNodeInputColourAttachmentPin  m_PinInput;  // Main color input
  xiiRenderPipelineNodeOutputColourAttachmentPin m_PinOutput; // Blurred color output

  // Optional pins
  xiiRenderPipelineNodeInputColourAttachmentPin m_PinDepthTexture;  // Depth SRV for bilateral / DoF
  xiiRenderPipelineNodeInputColourAttachmentPin m_PinNormalTexture; // Normal SRV for bilateral / edge-aware
  xiiRenderPipelineNodeInputColourAttachmentPin m_PinMotionVectors; // Motion blur
  xiiRenderPipelineNodeInputColourAttachmentPin m_PinCoCBuffer;     // Circle-of-confusion for DoF
  xiiRenderPipelineNodeInputColourAttachmentPin m_PinMaskTexture;   // Selective blur mask

  xiiBlurSettings m_BlurSettings;

  xiiSharedPtr<xiiGALBuffer> m_pBlurConstantBuffer;

  xiiShaderResourceHandle m_hShader;
};
