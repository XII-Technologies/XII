#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Size.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the miscellaneous texture flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMiscTextureFlags
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None         = 0U,         ///< No miscellaneous texture flags.
    GenerateMips = XII_BIT(0), ///< Allow automatic mipmap generation.
    Memoryless   = XII_BIT(1), ///< The texture will be used as a transient framebuffer attachment.
    SparseAlias  = XII_BIT(2), ///< For sparse textures, allow binding the same memory range in different texture regions or in different sparse textures.
    Subsampled   = XII_BIT(3), ///< The texture will be used as an intermediate render target for rendering with texture-based variable rate shading.

    ENUM_COUNT = 5U,

    Default = None
  };

  struct Bits
  {
    StorageType GenerateMips : 1;
    StorageType Memoryless : 1;
    StorageType SparseAlias : 1;
    StorageType Subsampled : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALMiscTextureFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALMiscTextureFlags);

/// \brief This describes the texture creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureCreationDescription : public xiiHashableStruct<xiiGALTextureCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                       m_sName;
  xiiEnum<xiiGALResourceDimension>    m_Type               = xiiGALResourceDimension::Undefined; ///< Texture type.
  xiiSizeU32                          m_Size               = xiiSizeU32(0, 0);                   ///< Texture width and height in pixels.
  xiiUInt32                           m_uiArraySizeOrDepth = 1U;                                 ///< For a 1D Array or 2D Array, the number of array slices. For a 3D texture, the number of depth slices.
  xiiEnum<xiiGALTextureFormat>        m_Format             = xiiGALTextureFormat::Unknown;       ///< Texture format.
  xiiUInt32                           m_uiMipLevels        = 1U;                                 ///< Number of Mip levels in the texture. Multisampled textures can only have 1 Mip level. Specify 0 to create full mipmap chain.
  xiiUInt32                           m_uiSampleCount      = 1U;                                 ///< Number of samples. Only 2D textures or 2D texture arrays can be multisampled.
  xiiBitflags<xiiGALBindFlags>        m_BindFlags          = xiiGALBindFlags::None;              ///< Bind flags.
  xiiEnum<xiiGALResourceUsage>        m_Usage              = xiiGALResourceUsage::Default;       ///< Texture usage.
  xiiBitflags<xiiGALCPUAccessFlag>    m_CPUAccessFlags     = xiiGALCPUAccessFlag::None;          ///< CPU access flags.
  xiiBitflags<xiiGALMiscTextureFlags> m_MiscFlags          = xiiGALMiscTextureFlags::None;       ///< Miscellaneous flags.
  xiiGALOptimizedClearValue           m_ClearValue;                                              ///< Optimized clear value.
  xiiUInt64                           m_uiImmediateContextMask = XII_BIT(0);                     ///< Defines which immediate contexts are allowed to execute commands that use this texture.
};

// \todo Add xiiGALTextureSubResData, xiiGALTextureData

struct XII_GRAPHICSFOUNDATION_DLL xiiGALMappedTextureSubresource : public xiiHashableStruct<xiiGALMappedTextureSubresource>
{
  XII_DECLARE_POD_TYPE();

  void*     m_pData         = nullptr;
  xiiUInt64 m_uiStride      = 0U;
  xiiUInt64 m_uiDepthStride = 0U;
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALSparseTextureProperties : public xiiHashableStruct<xiiGALSparseTextureProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64                             m_uiAddressSpaceSize = 0U;                      ///< The size of the texture's virtual address space.
  xiiUInt64                             m_uiMipTailOffset    = 0U;                      ///< Specifies where to bind the mip tail memory. Reserved for internal use.
  xiiUInt64                             m_uiMipTailStride    = 0U;                      ///< Specifies how to calculate the mip tail offset for 2D array texture. Reserved for internal use.
  xiiUInt64                             m_uiMipTailSize      = 0U;                      ///< Specifies the mip tail size in bytes.
  xiiUInt32                             m_uiFirstMipInTail   = 0U;                      ///< The first mip level in the mip tail that is packed as a whole into one or multiple memory blocks.
  xiiStaticArray<xiiUInt32, 3U>         m_TailSize;                                     ///< Specifies the dimension of a tile packed into a single memory block.
  xiiUInt32                             m_uiBlockSize = 0U;                             ///< Size of the sparse memory block, in bytes.
  xiiBitflags<xiiGALSparseTextureFlags> m_Flags       = xiiGALSparseTextureFlags::None; ///< Flags that describe additional packing modes.
};

// \todo Add texture resource abstraction.

#include <GraphicsFoundation/Resources/Implementation/Texture_inl.h>
