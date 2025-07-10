#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief Allocates a sampler consumed by graphics passes.
class xiiCreateSamplerPass : public xiiUtilityPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateSamplerPass, xiiUtilityPipelinePass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCreateSamplerPass);

public:
  xiiCreateSamplerPass(xiiStringView sName = "CreateSamplerPass");

  virtual ~xiiCreateSamplerPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodeOutputSamplerPin m_PinOutput;

  xiiEnum<xiiGALFilterType>         m_MinFilter;
  xiiEnum<xiiGALFilterType>         m_MagFilter;
  xiiEnum<xiiGALFilterType>         m_MipFilter;
  xiiEnum<xiiGALTextureAddressMode> m_AddressU;
  xiiEnum<xiiGALTextureAddressMode> m_AddressV;
  xiiEnum<xiiGALTextureAddressMode> m_AddressW;
  xiiEnum<xiiGALComparisonFunction> m_ComparisonFunction;
  xiiBitflags<xiiGALSamplerFlags>   m_Flags;
  bool                              m_bUnormalizedCoords = false;
  float                             m_fMipLodBias        = 0.0f;
  xiiUInt32                         m_uiMaxAnisotropy    = 0U;
  xiiColor                          m_BorderColor        = xiiColor::Black;
  float                             m_fMinLod            = 0.0f;
  float                             m_fMaxLod            = xiiMath::MaxValue<float>();
};
