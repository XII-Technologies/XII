#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the sampler flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSamplerFlags
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None                           = 0U,         ///< No sampler flags.
    Subsampled                     = XII_BIT(0), ///< Specifies that the sampler will read from a subsampled texture created with the miscallanous texture subsampled flag.
    SubsampledCoarseReconstruction = XII_BIT(1), ///< Specifies that the GPU is allowed to use fast approximation when reconstructing full-resolution value from the subsampled texture accessed by the sampler.

    ENUM_COUNT = 3U,

    Default = None
  };

  struct Bits
  {
    StorageType Subsampled : 1;
    StorageType SubsampledCoarseReconstruction : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSamplerFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSamplerFlags);

/// \brief This describes the sampler creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSamplerCreationDescription : public xiiHashableStruct<xiiGALSamplerCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALFilterType>         m_MinFilter          = xiiGALFilterType::Linear;        ///< Texture minification filter. The default is Linear.
  xiiEnum<xiiGALFilterType>         m_MagFilter          = xiiGALFilterType::Linear;        ///< Texture magnification filter. The default is Linear.
  xiiEnum<xiiGALFilterType>         m_MipFilter          = xiiGALFilterType::Linear;        ///< Texture mip filter. The default is Linear.
  xiiEnum<xiiGALTextureAddressMode> m_AddressU           = xiiGALTextureAddressMode::Clamp; ///< Texture address mode for U coordinate. The default is Clamp.
  xiiEnum<xiiGALTextureAddressMode> m_AddressV           = xiiGALTextureAddressMode::Clamp; ///< Texture address mode for V coordinate. The default is Clamp.
  xiiEnum<xiiGALTextureAddressMode> m_AddressW           = xiiGALTextureAddressMode::Clamp; ///< Texture address mode for W coordinate. The default is Clamp.
  xiiBitflags<xiiGALSamplerFlags>   m_Flags              = xiiGALSamplerFlags::None;        ///< Sampler flags.
  float                             m_fMipLODBias        = 0.0f;                            ///< Offset from the calculated mipmap level. The default is 0;
  xiiUInt32                         m_uiMaxAnisotropy    = 0U;                              ///< Maximum anisotropy level for the anisotropic filter. The default is 0.
  xiiEnum<xiiGALComparisonFunction> m_ComparisonFunction = xiiGALComparisonFunction::Never; ///< A function that compares sampled data against existing sampled data when comparisons filter used. The default is Never.
  xiiColor                          m_BorderColor        = xiiColor::Black;                 ///< Border color to use if the texture address border is specified for AddressU, AddressV, or AddressW. The default is xiiColor::Black.
  float                             m_fMinLOD            = 0.0f;                            ///< Specifies the minimum value that LOD is clamped to before accessing the texture MIP levels. The default is 0.
  float                             m_fMaxLOD            = xiiMath::MaxValue<float>();      ///< Specifies the maximum value that LOD is clamped to before accessing the texture MIP levels. The default is xiiMath::MaxValue<float>().
};

#include <GraphicsFoundation/Resources/Implementation/Sampler_inl.h>
