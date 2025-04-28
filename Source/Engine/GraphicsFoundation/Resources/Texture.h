#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Containers/Blob.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/TextureView.h>

/// \brief This describes the miscellaneous texture flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMiscTextureFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None         = 0U,         ///< No miscellaneous texture flags.
    GenerateMips = XII_BIT(0), ///< Allow automatic mipmap generation.
                               ///<
                               ///< \note The texture must be created with the xiiGALBindFlags::RenderTarget bind flag.

    Memoryless = XII_BIT(1), ///< The texture will be used as a transient framebuffer attachment.
                             ///<
                             ///< \note Memoryless textures may only be used within a render pass in a framebuffer; the corresponding sub pass load operation must be Clear or Discard, and the sub pass store operation must be Discard.

    SparseAlias = XII_BIT(2), ///< For sparse textures, allow binding the same memory range in different texture regions or in different sparse textures.
    Subsampled  = XII_BIT(3), ///< The texture will be used as an intermediate render target for rendering with texture-based variable rate shading. This requires the xiiGALShadingRateCapabilityFlags::SubSampledRenderTarget capability.
                              ///<
                              ///< \note Copy operations are not supported for subsampled textures.

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

  xiiEnum<xiiGALResourceDimension>    m_Type               = xiiGALResourceDimension::Undefined; ///< Texture type. The default is Undefined.
  xiiSizeU32                          m_Size               = xiiSizeU32(0, 0);                   ///< Texture width and height in pixels. The default is (0, 0).
  xiiUInt32                           m_uiArraySizeOrDepth = 1U;                                 ///< For a 1D Array or 2D Array, the number of array slices. For a 3D texture, the number of depth slices. The default is 1.
  xiiEnum<xiiGALResourceFormat>       m_Format             = xiiGALResourceFormat::Unknown;      ///< Texture format. The default is Unknown.
  xiiUInt32                           m_uiMipLevels        = 1U;                                 ///< Number of Mip levels in the texture. Multi-sampled textures can only have 1 Mip level. Specify 0 to create full mipmap chain. The default is 1.
  xiiUInt32                           m_uiSampleCount      = 1U;                                 ///< Number of samples. Only 2D textures or 2D texture arrays can be multi-sampled. The default is 1.
  xiiBitflags<xiiGALBindFlags>        m_BindFlags          = xiiGALBindFlags::None;              ///< Bind flags. The default is None.
  xiiEnum<xiiGALResourceUsage>        m_Usage              = xiiGALResourceUsage::Default;       ///< Texture usage. The default is Default.
  xiiBitflags<xiiGALCPUAccessFlag>    m_CPUAccessFlags     = xiiGALCPUAccessFlag::None;          ///< CPU access flags. The default is None.
  xiiBitflags<xiiGALMiscTextureFlags> m_MiscFlags          = xiiGALMiscTextureFlags::None;       ///< Miscellaneous flags. The default is None.
  xiiGALOptimizedClearValue           m_ClearValue;                                              ///< Optimized clear value.
  xiiUInt64                           m_uiCommandQueueMask = XII_BIT(0);                         ///< Defines which command queues are allowed to execute commands that use this texture. The default is the main command queue.
                                                                                                 ///< Only specify the bits that indicate those command queues where the resource will be used, setting unnecessary bits will result in extra overhead.
  void* m_pExisitingNativeObject = nullptr;                                                      ///< Can be used to encapsulate existing native textures in objects usable by the GAL

  constexpr XII_ALWAYS_INLINE bool      IsArray() const { return m_Type == xiiGALResourceDimension::Texture1DArray || m_Type == xiiGALResourceDimension::Texture2DArray || m_Type == xiiGALResourceDimension::TextureCube || m_Type == xiiGALResourceDimension::TextureCubeArray; }
  constexpr XII_ALWAYS_INLINE bool      Is1D() const { return m_Type == xiiGALResourceDimension::Texture1D || m_Type == xiiGALResourceDimension::Texture1DArray; }
  constexpr XII_ALWAYS_INLINE bool      Is2D() const { return m_Type == xiiGALResourceDimension::Texture2D || m_Type == xiiGALResourceDimension::Texture2DArray || m_Type == xiiGALResourceDimension::TextureCube || m_Type == xiiGALResourceDimension::TextureCubeArray; }
  constexpr XII_ALWAYS_INLINE bool      Is3D() const { return m_Type == xiiGALResourceDimension::Texture3D; }
  constexpr XII_ALWAYS_INLINE bool      IsCube() const { return m_Type == xiiGALResourceDimension::TextureCube || m_Type == xiiGALResourceDimension::TextureCubeArray; }
  constexpr XII_ALWAYS_INLINE xiiUInt32 GetArraySize() const { return IsArray() ? m_uiArraySizeOrDepth : 1U; }
  constexpr XII_ALWAYS_INLINE xiiUInt32 GetWidth() const { return m_Size.width; }
  constexpr XII_ALWAYS_INLINE xiiUInt32 GetHeight() const { return Is1D() ? 1U : m_Size.height; }
  constexpr XII_ALWAYS_INLINE xiiUInt32 GetDepth() const { return Is3D() ? m_uiArraySizeOrDepth : 1U; }
};

/// \brief This describes the data for one texture sub-resource.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureSubResourceData : public xiiHashableStruct<xiiGALTextureSubResourceData>
{
  XII_DECLARE_POD_TYPE();

  xiiConstByteBlobPtr m_pData;              ///< Pointer to the sub-resource data in GPU memory.
  xiiUInt64           m_uiStride      = 0U; ///< For 2D and 3D textures, the row stride in bytes.
  xiiUInt64           m_uiDepthStride = 0U; ///< For 3D textures, the depth slice stride in bytes.
};

/// \brief This describes the initial data to store in the texture.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureData : public xiiHashableStruct<xiiGALTextureData>
{
  xiiArrayPtr<xiiGALTextureSubResourceData> m_SubResources; ///< Pointer to the array of the texture sub-resource elements containing the information about each sub-resource.

  // \todo GraphicsFoundation: Add command encoder that should be used to initialize the texture?
};

/// \brief This describes the mapped texture sub-resource data.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMappedTextureSubresource : public xiiHashableStruct<xiiGALMappedTextureSubresource>
{
  XII_DECLARE_POD_TYPE();

  void*     m_pData         = nullptr; ///< The pointer to the mapped texture sub-resource data.
  xiiUInt64 m_uiStride      = 0U;      ///< For 2D and 3D textures, the row stride in bytes.
  xiiUInt64 m_uiDepthStride = 0U;      ///< For 3D textures, the depth stride in bytes.
};

/// \brief This describes the mapped texture sub-resource mip-level data.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureMipLevelData : public xiiHashableStruct<xiiGALTextureMipLevelData>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiMipLevel   = 0U; ///< The texture mip level. Default is 0.
  xiiUInt32 m_uiArraySlice = 0U; ///< The texture array slice. Default is 0.
};

/// \brief This describes the sparse texture properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSparseTextureProperties : public xiiHashableStruct<xiiGALSparseTextureProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64 m_uiAddressSpaceSize = 0U; ///< The size of the texture's virtual address space. The default is 0.
  xiiUInt64 m_uiMipTailOffset    = 0U; ///< Specifies where to bind the mip tail memory. Reserved for internal use.
  xiiUInt64 m_uiMipTailStride    = 0U; ///< Specifies how to calculate the mip tail offset for 2D array texture. Reserved for internal use.
  xiiUInt64 m_uiMipTailSize      = 0U; ///< Specifies the mip tail size in bytes. The default is 0.
                                       ///<
                                       ///< \note A single mip tail for a 2D array may exceed the 32-bit limit.

  xiiUInt32  m_uiFirstMipInTail = 0U; ///< The first mip level in the mip tail that is packed as a whole into one or multiple memory blocks. The default is 0.
  xiiVec3U32 m_vTileSize;             ///< Specifies the dimension of a tile packed into a single memory block.
  xiiUInt32  m_uiBlockSize = 0U;      ///< Size of the sparse memory block, in bytes. The default is 0.
                                      ///<
                                      ///< \remarks The offset in the packed mip tail, memory offset and memory size that are used in sparse memory binding command must be multiples of the block size.
                                      ///<          If the xiiGALSparseTextureFlags::NonStandardBlockSize flag is not set in the Flags member, the block size is equal to xiiGALSparseResourceProperties::m_uiStandardBlockSize.

  xiiBitflags<xiiGALSparseTextureFlags> m_Flags = xiiGALSparseTextureFlags::None; ///< Flags that describe additional packing modes. The default is None.
};

/// \brief Interface that defines methods to manipulate a texture object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALTexture : public xiiGALResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTexture, xiiGALResource);

public:
  /// \brief This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALTextureCreationDescription& GetDescription() const { return m_Description; }

  /// \brief This returns the reference-counted pointer of the default view.
  ///
  /// \param viewType - The type of the requested view. See xiiGALTextureViewType.
  ///
  /// \return The reference-counted pointer to the buffer view.
  ///
  /// \note The function **increases** the reference counter for the returned interface.
  [[nodiscard]] xiiSharedPtr<xiiGALTextureView> GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType);

  /// \brief This returns the sparse texture properties.
  [[nodiscard]] virtual const xiiGALSparseTextureProperties& GetSparseProperties() const = 0;

protected:
  friend class xiiGALDevice;

  xiiGALTexture(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTexture();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData) = 0;

  virtual xiiResult DeInitPlatform() = 0;

protected:
  xiiGALTextureCreationDescription m_Description;

  xiiSharedPtr<xiiGALTextureView> m_DefaultTextureViews[xiiGALTextureViewType::ENUM_COUNT];

private:
  void CreateDefaultResourceViews();
};
