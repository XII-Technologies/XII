#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

struct XII_GRAPHICSCORE_DLL xiiSourceFormat
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Color4Channel8BitNormalized_sRGB,
    Color4Channel8BitNormalized,
    Color4Channel16BitFloat,
    Color4Channel32BitFloat,
    Color3Channel11_11_10BitFloat,
    Depth16Bit,
    Depth24BitStencil8Bit,
    Depth32BitFloat,

    ENUM_COUNT,

    Default = Color4Channel8BitNormalized_sRGB
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSourceFormat);

class XII_GRAPHICSCORE_DLL xiiSourcePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSourcePass, xiiRenderPipelinePass);

public:
  xiiSourcePass(xiiStringView sName = "SourcePass");
  ~xiiSourcePass();

  static xiiGALTextureCreationDescription GetOutputDescription(const xiiView& view, xiiEnum<xiiSourceFormat> format, xiiEnum<xiiGALMSAASampleCount> msaaSampleCount);
  virtual bool                            GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> pInputs, xiiArrayPtr<xiiGALTextureCreationDescription> pOutputs) override;
  virtual void                            InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;
  virtual void                            Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;
  virtual xiiResult                       Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult                       Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiEnum<xiiSourceFormat>                m_Format                          = xiiSourceFormat::Default;
  xiiEnum<xiiGALMSAASampleCount>          m_SampleCount                     = xiiGALMSAASampleCount::OneSample;
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentLoadOperation         = xiiGALAttachmentLoadOperation::Load;
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStoreOperation        = xiiGALAttachmentStoreOperation::Store;
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentStencilLoadOperation  = xiiGALAttachmentLoadOperation::Load;
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStencilStoreOperation = xiiGALAttachmentStoreOperation::Store;
  xiiColor                                m_ClearColor                      = xiiColor::Black;
  float                                   m_fDepthClearValue                = 1.0f;
  xiiUInt8                                m_uiStencilClearValue             = 0U;

  xiiSharedPtr<xiiGALRenderPass>  m_pRenderPass;
  xiiSharedPtr<xiiGALFramebuffer> m_pFramebuffer;
};
