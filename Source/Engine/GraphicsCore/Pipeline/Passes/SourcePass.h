#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

struct xiiSourceFormat
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Color4Channel8BitNormalized_sRGB,
    Color4Channel8BitNormalized,
    Color4Channel16BitFloat,
    Color4Channel32BitFloat,
    Color3Channel11_11_10BitFloat,
    Depth16Bit,
    Depth24BitStencil8Bit,
    Depth32BitFloat,

    Default = Color4Channel8BitNormalized_sRGB
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSourceFormat);


class XII_GRAPHICSCORE_DLL xiiSourcePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSourcePass, xiiRenderPipelinePass);

public:
  xiiSourcePass(const char* szName = "SourcePass");
  ~xiiSourcePass();

  virtual bool      GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiEnum<xiiSourceFormat>       m_Format;
  xiiEnum<xiiGALSampleCount> m_MsaaMode = xiiGALSampleCount::OneSample;
  xiiColor                       m_ClearColor;
  bool                           m_bClear;
};
