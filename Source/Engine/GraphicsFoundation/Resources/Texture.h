/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Containers/Blob.h>
#include <GraphicsFoundation/Resources/TextureView.h>

class xiiStreamWriter;

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

  /// \brief Returns true if the texture is a 1D texture or a 1D texture array.
  XII_ALWAYS_INLINE bool Is1D() const { return m_Type == xiiGALResourceDimension::Texture1D || m_Type == xiiGALResourceDimension::Texture1DArray; }

  /// \brief Returns true if the texture is a 2D texture or a 2D texture array.
  XII_ALWAYS_INLINE bool Is2D() const { return m_Type == xiiGALResourceDimension::Texture2D || m_Type == xiiGALResourceDimension::Texture2DArray; }

  /// \brief Returns true if the texture is a 3D texture.
  XII_ALWAYS_INLINE bool Is3D() const { return m_Type == xiiGALResourceDimension::Texture3D; }

  /// \brief Returns true if the texture is a cube map or a cube map array.
  XII_ALWAYS_INLINE bool IsCube() const { return m_Type == xiiGALResourceDimension::TextureCube || m_Type == xiiGALResourceDimension::TextureCubeArray; }

  /// \brief Returns true if the texture is a 1D array, 2D array, cube map or cube map array.
  XII_ALWAYS_INLINE bool IsArray() const { return m_Type == xiiGALResourceDimension::Texture1DArray || m_Type == xiiGALResourceDimension::Texture2DArray || m_Type == xiiGALResourceDimension::TextureCube || m_Type == xiiGALResourceDimension::TextureCubeArray; }

  xiiEnum<xiiGALResourceDimension>    m_Type                  = xiiGALResourceDimension::Undefined; ///< Texture type. The default is Undefined.
  xiiSizeU32                          m_Size                  = xiiSizeU32(0, 0);                   ///< Texture width and height in pixels. The default is (0, 0).
  xiiUInt32                           m_uiArraySizeOrDepth    = 1U;                                 ///< For a 1D Array or 2D Array, the number of array slices. For cube maps and cube map arrays, this value must be a multiple of 6. For a 3D texture, the number of depth slices. The default is 1.
  xiiEnum<xiiGALResourceFormat>       m_Format                = xiiGALResourceFormat::Unknown;      ///< Texture format. The default is Unknown.
  xiiUInt32                           m_uiMipLevels           = 1U;                                 ///< Number of Mip levels in the texture. Multi-sampled textures can only have 1 Mip level. Specify 0 to create full mipmap chain. The default is 1.
  xiiUInt32                           m_uiSampleCount         = 1U;                                 ///< Number of samples. Only 2D textures or 2D texture arrays can be multi-sampled. The default is 1.
  xiiBitflags<xiiGALBindFlags>        m_BindFlags             = xiiGALBindFlags::None;              ///< Bind flags. The default is None.
  xiiEnum<xiiGALResourceUsage>        m_Usage                 = xiiGALResourceUsage::Mutable;       ///< Texture usage. The default is Default.
  xiiBitflags<xiiGALCPUAccessFlag>    m_CPUAccessFlags        = xiiGALCPUAccessFlag::None;          ///< CPU access flags. The default is None.
  xiiBitflags<xiiGALMiscTextureFlags> m_MiscFlags             = xiiGALMiscTextureFlags::None;       ///< Miscellaneous flags. The default is None.
  xiiGALOptimizedClearValue           m_ClearValue            = {};                                 ///< Optimized clear value.
  void*                               m_pExistingNativeObject = nullptr;                            ///< Used to encapsulate existing native textures in objects usable by the GAL.
};

/// \brief This describes the data for one texture sub-resource.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureSubResourceData
{
  XII_ALWAYS_INLINE xiiGALTextureSubResourceData() :
    m_uiStride(0ULL), m_uiDepthStride(0ULL)
  {
  }

  XII_ALWAYS_INLINE xiiGALTextureSubResourceData(xiiConstByteBlobPtr pData, xiiUInt64 uiStride) :
    m_pData(pData), m_uiStride(uiStride), m_uiDepthStride(0U)
  {
  }

  XII_ALWAYS_INLINE xiiGALTextureSubResourceData(xiiConstByteBlobPtr pData, xiiUInt64 uiStride, xiiUInt64 uiDepthStride) :
    m_pData(pData), m_uiStride(uiStride), m_uiDepthStride(uiDepthStride)
  {
  }

  xiiConstByteBlobPtr m_pData;         ///< Pointer to the sub-resource data in GPU memory.
  xiiUInt64           m_uiStride;      ///< For 2D and 3D textures, the row stride in bytes.
  xiiUInt64           m_uiDepthStride; ///< For 3D textures, the depth slice stride in bytes.
};

/// \brief This describes the initial data to store in the texture.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureData
{
  XII_ALWAYS_INLINE xiiGALTextureData() :
    m_pCommandList(nullptr)
  {
  }

  XII_ALWAYS_INLINE xiiGALTextureData(xiiArrayPtr<const xiiGALTextureSubResourceData> pSubResources) :
    m_pSubResources(pSubResources), m_pCommandList(nullptr)
  {
  }

  XII_ALWAYS_INLINE xiiGALTextureData(xiiArrayPtr<const xiiGALTextureSubResourceData> pSubResources, xiiGALCommandList* pCommandList) :
    m_pSubResources(pSubResources), m_pCommandList(pCommandList)
  {
  }

  xiiArrayPtr<const xiiGALTextureSubResourceData> m_pSubResources; ///< Pointer to the array of the texture sub-resource elements containing the information about each sub-resource.
  xiiGALCommandList*                              m_pCommandList;  ///< Optional command list used to upload data; if null, a new one is created; if reused elsewhere, synchronization (e.g., fence) is required.
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

  /// \brief Returns the external memory kind flags for this texture.
  [[nodiscard]] XII_ALWAYS_INLINE xiiBitflags<xiiGALExternalMemoryKind> GetExternalMemoryKind() const { return m_ExternalMemoryDescription.m_Type; }

  /// \brief Returns the external memory description for this texture.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALExternalMemoryDescription& GetExternalMemoryDescription() const { return m_ExternalMemoryDescription; }

  /// \brief Returns the calculated memory consumption for texture.
  [[nodiscard]] virtual xiiUInt64 GetMemoryConsumption() const;

  /// \brief This returns the reference-counted pointer of the default view.
  ///
  /// \param viewType - The type of the requested view. See xiiGALTextureViewType.
  ///
  /// \return The reference-counted pointer to the texture view.
  ///
  /// \note The function **increases** the reference counter for the returned interface.
  [[nodiscard]] xiiSharedPtr<xiiGALTextureView> GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType);

  /// \brief This returns the sparse texture properties.
  [[nodiscard]] virtual const xiiGALSparseTextureProperties& GetSparseProperties() const = 0;

  /// \brief This creates a new texture view.
  ///
  /// \param description - The texture view description. See xiiGALTextureViewCreationDescription.
  ///
  /// \return The reference-counted pointer to the texture view.
  ///
  /// \remarks To create a shader resource view addressing the entire texture, set only xiiGALTextureViewCreationDescription::m_ViewType member of the description parameter to xiiGALTextureViewType::ShaderResource and leave all other
  ///          members in their default values. Using the same method, you can create render target or depth stencil view addressing the largest mip level.\n
  ///          If texture view format is xiiGALResourceFormat::Unknown, the view format will match the texture format.\n
  ///          If texture view type is xiiGALTextureViewType::Undefined, the type will match the texture type.\n
  ///          If the number of mip levels is 0, and the view type is shader resource, the view will address all mip levels. For other view types it will address one mip level.\n
  ///          If the number of slices is 0, all slices from m_uiFirstArraySlice or m_uiFirstDepthSlice will be referenced by the view.
  ///          For non-array textures, the only allowed values for the number of slices are 0 and 1.\n
  ///          Texture view will contain strong reference to the texture, so the texture will not be destroyed until all views are released.\n
  [[nodiscard]] xiiSharedPtr<xiiGALTextureView> CreateView(xiiGALTextureViewCreationDescription& description);

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALTexture(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTexture();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) = 0;

  virtual xiiInternal::NewInstance<xiiGALTextureView> CreateViewPlatform(const xiiGALTextureViewCreationDescription& description) = 0;

protected:
  xiiGALTextureCreationDescription m_Description;

  xiiGALExternalMemoryDescription m_ExternalMemoryDescription;

  xiiSharedPtr<xiiGALTextureView> m_DefaultTextureViews[xiiGALTextureViewType::ENUM_COUNT];

private:
  void CreateDefaultResourceViews();
};

XII_GRAPHICSFOUNDATION_DLL xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiGALTextureCreationDescription& description);

XII_GRAPHICSFOUNDATION_DLL xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiGALTextureCreationDescription& out_description);
