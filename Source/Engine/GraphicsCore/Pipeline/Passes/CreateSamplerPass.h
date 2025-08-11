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

public:
  /// \brief Gets the minification filter type.
  XII_ALWAYS_INLINE xiiEnum<xiiGALFilterType> GetMinFilter() const { return m_MinFilter; }

  /// \brief Sets the minification filter type.
  XII_ALWAYS_INLINE void SetMinFilter(xiiEnum<xiiGALFilterType> filter) { m_MinFilter = filter; }


  /// \brief Gets the magnification filter type.
  XII_ALWAYS_INLINE xiiEnum<xiiGALFilterType> GetMagFilter() const { return m_MagFilter; }

  /// \brief Sets the magnification filter type.
  XII_ALWAYS_INLINE void SetMagFilter(xiiEnum<xiiGALFilterType> filter) { m_MagFilter = filter; }


  /// \brief Gets the mipmap filter type.
  XII_ALWAYS_INLINE xiiEnum<xiiGALFilterType> GetMipFilter() const { return m_MipFilter; }

  /// \brief Sets the mipmap filter type.
  XII_ALWAYS_INLINE void SetMipFilter(xiiEnum<xiiGALFilterType> filter) { m_MipFilter = filter; }


  /// \brief Gets the texture address mode for U coordinate.
  XII_ALWAYS_INLINE xiiEnum<xiiGALTextureAddressMode> GetAddressU() const { return m_AddressU; }

  /// \brief Sets the texture address mode for U coordinate.
  XII_ALWAYS_INLINE void SetAddressU(xiiEnum<xiiGALTextureAddressMode> mode) { m_AddressU = mode; }


  /// \brief Gets the texture address mode for V coordinate.
  XII_ALWAYS_INLINE xiiEnum<xiiGALTextureAddressMode> GetAddressV() const { return m_AddressV; }

  /// \brief Sets the texture address mode for V coordinate.
  XII_ALWAYS_INLINE void SetAddressV(xiiEnum<xiiGALTextureAddressMode> mode) { m_AddressV = mode; }


  /// \brief Gets the texture address mode for W coordinate.
  XII_ALWAYS_INLINE xiiEnum<xiiGALTextureAddressMode> GetAddressW() const { return m_AddressW; }

  /// \brief Sets the texture address mode for W coordinate.
  XII_ALWAYS_INLINE void SetAddressW(xiiEnum<xiiGALTextureAddressMode> mode) { m_AddressW = mode; }


  /// \brief Gets the comparison function used for sampling.
  XII_ALWAYS_INLINE xiiEnum<xiiGALComparisonFunction> GetComparisonFunction() const { return m_ComparisonFunction; }

  /// \brief Sets the comparison function used for sampling.
  XII_ALWAYS_INLINE void SetComparisonFunction(xiiEnum<xiiGALComparisonFunction> func) { m_ComparisonFunction = func; }


  /// \brief Gets the sampler flags.
  XII_ALWAYS_INLINE xiiBitflags<xiiGALSamplerFlags> GetFlags() const { return m_Flags; }

  /// \brief Sets the sampler flags.
  XII_ALWAYS_INLINE void SetFlags(xiiBitflags<xiiGALSamplerFlags> flags) { m_Flags = flags; }


  /// \brief Returns whether unnormalized texture coordinates are used.
  XII_ALWAYS_INLINE bool GetUseUnnormalizedCoords() const { return m_bUnormalizedCoords; }

  /// \brief Sets whether unnormalized texture coordinates are used.
  XII_ALWAYS_INLINE void SetUseUnnormalizedCoords(bool bUnnormalized) { m_bUnormalizedCoords = bUnnormalized; }


  /// \brief Gets the mip LOD bias.
  XII_ALWAYS_INLINE float GetMipLodBias() const { return m_fMipLodBias; }

  /// \brief Sets the mip LOD bias.
  XII_ALWAYS_INLINE void SetMipLodBias(float bias) { m_fMipLodBias = bias; }


  /// \brief Gets the maximum anisotropy level.
  XII_ALWAYS_INLINE xiiUInt32 GetMaxAnisotropy() const { return m_uiMaxAnisotropy; }

  /// \brief Sets the maximum anisotropy level.
  XII_ALWAYS_INLINE void SetMaxAnisotropy(xiiUInt32 anisotropy) { m_uiMaxAnisotropy = anisotropy; }


  /// \brief Gets the border color used for texture sampling.
  XII_ALWAYS_INLINE xiiColor GetBorderColor() const { return m_BorderColor; }

  /// \brief Sets the border color used for texture sampling.
  XII_ALWAYS_INLINE void SetBorderColor(const xiiColor& color) { m_BorderColor = color; }


  /// \brief Gets the minimum LOD value.
  XII_ALWAYS_INLINE float GetMinLod() const { return m_fMinLod; }

  /// \brief Sets the minimum LOD value.
  XII_ALWAYS_INLINE void SetMinLod(float minLod) { m_fMinLod = minLod; }


  /// \brief Gets the maximum LOD value.
  XII_ALWAYS_INLINE float GetMaxLod() const { return m_fMaxLod; }

  /// \brief Sets the maximum LOD value.
  XII_ALWAYS_INLINE void SetMaxLod(float maxLod) { m_fMaxLod = maxLod; }

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
