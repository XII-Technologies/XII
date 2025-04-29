#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>

#include <d3d11_2.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

DXGI_FORMAT ResourceFormatToDXGI_Format(xiiEnum<xiiGALResourceFormat> textureFormat, xiiBitflags<xiiGALBindFlags> bindFlags);

xiiGALTextureD3D11::xiiGALTextureD3D11(xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(pDeviceD3D11, creationDescription)
{
}

xiiGALTextureD3D11::~xiiGALTextureD3D11() = default;

xiiResult xiiGALTextureD3D11::InitPlatform(const xiiGALTextureData* pInitialData)
{
  if (m_Description.m_Usage == xiiGALResourceUsage::Immutable && (pInitialData == nullptr || pInitialData->m_SubResources.IsEmpty()))
  {
    xiiLog::Error("Immutable textures must be initialized with data at creation time: pInitialData cannot be null.");
    return XII_FAILURE;
  }

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
  {
    xiiBitflags<xiiGALBindFlags> allowedBindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil;

    if (!m_Description.m_BindFlags.IsStrictlyAnySet(allowedBindFlags))
    {
      xiiLog::Error("The texture description contains invalid bind flags for sparse textures in Direct3D11.");
      return XII_FAILURE;
    }
  }

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    XII_SUCCEED_OR_RETURN(CreateFromNativeObject(m_Description.m_pExisitingNativeObject));
  }
  else
  {
    switch (m_Description.m_Type)
    {
      case xiiGALResourceDimension::Texture1D:
      case xiiGALResourceDimension::Texture1DArray:
      {
        ID3D11Texture1D* pTexture1D = nullptr;
        XII_SUCCEED_OR_RETURN(CreateTexture1D(&pTexture1D, pInitialData));

        m_pTexture = pTexture1D;
      }
      break;
      case xiiGALResourceDimension::Texture2D:
      case xiiGALResourceDimension::TextureCube:
      case xiiGALResourceDimension::Texture2DArray:
      case xiiGALResourceDimension::TextureCubeArray:
      {
        ID3D11Texture2D* pTexture2D = nullptr;
        XII_SUCCEED_OR_RETURN(CreateTexture2D(&pTexture2D, pInitialData));

        m_pTexture = pTexture2D;
      }
      break;
      case xiiGALResourceDimension::Texture3D:
      {
        ID3D11Texture3D* pTexture3D = nullptr;
        XII_SUCCEED_OR_RETURN(CreateTexture3D(&pTexture3D, pInitialData));

        m_pTexture = pTexture3D;
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  InitializeSparseTextureProperties();

  return XII_SUCCESS;
}

xiiResult xiiGALTextureD3D11::DeInitPlatform()
{
  XII_GAL_D3D11_RELEASE(m_pTexture);

  return XII_SUCCESS;
}

void xiiGALTextureD3D11::SetDebugNamePlatform(xiiStringView sName)
{
  if (m_pTexture != nullptr)
  {
    xiiStringBuilder sb;
    if (FAILED(m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, sName.GetElementCount(), sName.GetData(sb))))
    {
      xiiLog::Error("Failed to set the Direct3D11 texture debug name.");
    }
  }
}

xiiResult xiiGALTextureD3D11::CreateFromNativeObject(void* pNativeObject)
{
  ID3D11Resource* pTextureObject = static_cast<ID3D11Resource*>(pNativeObject);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  ID3D11Resource* pD3D11TextureResource = nullptr;
  if (FAILED(pTextureObject->QueryInterface(__uuidof(ID3D11Resource), (void**)&pD3D11TextureResource)))
  {
    xiiLog::Error("The interface interface of the corresponding object is not a texture object.");
    return XII_FAILURE;
  }
  XII_GAL_D3D11_RELEASE(pD3D11TextureResource);
#endif

  m_pTexture = pTextureObject;

  return XII_SUCCESS;
}

xiiResult xiiGALTextureD3D11::CreateTexture1D(ID3D11Texture1D** ppTexture1D, const xiiGALTextureData* pInitialData)
{
  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11 = m_pDevice.Downcast<xiiGALDeviceD3D11>();

  D3D11_TEXTURE1D_DESC textureDescription = {};
  textureDescription.Width                = m_Description.m_Size.width;
  textureDescription.MipLevels            = m_Description.m_uiMipLevels;
  textureDescription.ArraySize            = m_Description.GetArraySize();
  textureDescription.Format               = xiiD3D11TypeConversions::GetFormat(m_Description.m_Format);
  textureDescription.Usage                = xiiD3D11TypeConversions::GetUsage(m_Description.m_Usage);
  textureDescription.BindFlags            = xiiD3D11TypeConversions::GetBindFlags(m_Description.m_BindFlags);
  textureDescription.CPUAccessFlags       = xiiD3D11TypeConversions::GetCPUAccessFlags(m_Description.m_CPUAccessFlags);
  textureDescription.MiscFlags            = xiiD3D11TypeConversions::GetMiscTextureFlags(m_Description.m_MiscFlags);

  xiiHybridArray<D3D11_SUBRESOURCE_DATA, 16U> initialData;
  PrepareInitialData(m_Description, pInitialData, initialData);

  if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateTexture1D(&textureDescription, !initialData.IsEmpty() ? initialData.GetData() : nullptr, ppTexture1D)))
  {
    xiiLog::Error("Failed to create the Direct3D11 Texture1D.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALTextureD3D11::CreateTexture2D(ID3D11Texture2D** ppTexture2D, const xiiGALTextureData* pInitialData)
{
  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11 = m_pDevice.Downcast<xiiGALDeviceD3D11>();

  DXGI_SAMPLE_DESC sampleDescription = {.Count = m_Description.m_uiSampleCount, .Quality = 0U};

  D3D11_TEXTURE2D_DESC textureDescription = {};
  textureDescription.Width                = m_Description.m_Size.width;
  textureDescription.Height               = m_Description.m_Size.height;
  textureDescription.MipLevels            = m_Description.m_uiMipLevels;
  textureDescription.ArraySize            = m_Description.GetArraySize();
  textureDescription.Format               = ResourceFormatToDXGI_Format(m_Description.m_Format, m_Description.m_BindFlags);
  textureDescription.SampleDesc           = sampleDescription;
  textureDescription.Usage                = xiiD3D11TypeConversions::GetUsage(m_Description.m_Usage);
  textureDescription.BindFlags            = xiiD3D11TypeConversions::GetBindFlags(m_Description.m_BindFlags);
  textureDescription.CPUAccessFlags       = xiiD3D11TypeConversions::GetCPUAccessFlags(m_Description.m_CPUAccessFlags);
  textureDescription.MiscFlags            = xiiD3D11TypeConversions::GetMiscTextureFlags(m_Description.m_MiscFlags);

  if (textureDescription.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS)
    textureDescription.BindFlags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

  if (m_Description.m_Type == xiiGALResourceDimension::TextureCube || m_Description.m_Type == xiiGALResourceDimension::TextureCubeArray)
    textureDescription.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
    textureDescription.MiscFlags |= D3D11_RESOURCE_MISC_TILED;

  xiiHybridArray<D3D11_SUBRESOURCE_DATA, 16U> initialData;
  PrepareInitialData(m_Description, pInitialData, initialData);

  if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateTexture2D(&textureDescription, !initialData.IsEmpty() ? initialData.GetData() : nullptr, ppTexture2D)))
  {
    xiiLog::Error("Failed to create the Direct3D11 Texture2D.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALTextureD3D11::CreateTexture3D(ID3D11Texture3D** ppTexture3D, const xiiGALTextureData* pInitialData)
{
  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11 = m_pDevice.Downcast<xiiGALDeviceD3D11>();

  D3D11_TEXTURE3D_DESC textureDescription = {};
  textureDescription.Width                = m_Description.m_Size.width;
  textureDescription.Height               = m_Description.m_Size.height;
  textureDescription.Depth                = m_Description.GetArraySize();
  textureDescription.MipLevels            = m_Description.m_uiMipLevels;
  textureDescription.Format               = xiiD3D11TypeConversions::GetFormat(m_Description.m_Format);
  textureDescription.Usage                = xiiD3D11TypeConversions::GetUsage(m_Description.m_Usage);
  textureDescription.BindFlags            = xiiD3D11TypeConversions::GetBindFlags(m_Description.m_BindFlags);
  textureDescription.CPUAccessFlags       = xiiD3D11TypeConversions::GetCPUAccessFlags(m_Description.m_CPUAccessFlags);
  textureDescription.MiscFlags            = xiiD3D11TypeConversions::GetMiscTextureFlags(m_Description.m_MiscFlags);

  if (m_Description.m_Usage == xiiGALResourceUsage::Sparse)
    textureDescription.MiscFlags |= D3D11_RESOURCE_MISC_TILED;

  xiiHybridArray<D3D11_SUBRESOURCE_DATA, 16U> initialData;
  PrepareInitialData(m_Description, pInitialData, initialData);

  if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateTexture3D(&textureDescription, !initialData.IsEmpty() ? initialData.GetData() : nullptr, ppTexture3D)))
  {
    xiiLog::Error("Failed to create the Direct3D11 Texture3D.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiGALTextureD3D11::PrepareInitialData(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData, xiiHybridArray<D3D11_SUBRESOURCE_DATA, 16>& out_InitialData)
{
  if (pInitialData != nullptr && !pInitialData->m_SubResources.IsEmpty())
  {
    xiiUInt32 uiInitialDataCount = description.GetArraySize() * description.m_uiMipLevels;

    XII_ASSERT_DEV(uiInitialDataCount == pInitialData->m_SubResources.GetCount(), "The array of initial data values is not equal to the number of mip levels.");

    out_InitialData.SetCountUninitialized(pInitialData->m_SubResources.GetCount());

    for (xiiUInt32 i = 0; i < uiInitialDataCount; ++i)
    {
      out_InitialData[i].pSysMem          = pInitialData->m_SubResources[i].m_pData.GetPtr();
      out_InitialData[i].SysMemPitch      = static_cast<xiiUInt32>(pInitialData->m_SubResources[i].m_uiStride);
      out_InitialData[i].SysMemSlicePitch = static_cast<xiiUInt32>(pInitialData->m_SubResources[i].m_uiDepthStride);
    }
  }
}

void xiiGALTextureD3D11::InitializeSparseTextureProperties()
{
  if (m_Description.m_Usage != xiiGALResourceUsage::Sparse)
    return;

  xiiSharedPtr<xiiGALDeviceD3D11> pDeviceD3D11  = m_pDevice.Downcast<xiiGALDeviceD3D11>();
  ID3D11Device2*     pD3D11Device2 = nullptr;

  if (FAILED(pDeviceD3D11->GetD3D11Device()->QueryInterface(__uuidof(ID3D11Device2), (void**)&pD3D11Device2)))
  {
    xiiLog::Error("Failed to query ID3D11Device2 for resource tiling in sparse texture properties.");
    return;
  }
  XII_SCOPE_EXIT(XII_GAL_D3D11_RELEASE(pD3D11Device2));

  xiiUInt32             uiTileCountForEntireResource      = 0U;
  xiiUInt32             uiSubresourceTilingCount          = 0U;
  D3D11_TILE_SHAPE      standardTileShapeForNonPackedMips = {};
  D3D11_PACKED_MIP_DESC packedMipDescription              = {};
  pD3D11Device2->GetResourceTiling(m_pTexture, &uiTileCountForEntireResource, &packedMipDescription, &standardTileShapeForNonPackedMips, &uiSubresourceTilingCount, 0, nullptr);

  XII_ASSERT_DEV(uiTileCountForEntireResource % m_Description.GetArraySize() == 0, "");

  m_SparseTextureProperties = xiiGALSparseTextureProperties{
    .m_uiAddressSpaceSize = xiiUInt64{uiTileCountForEntireResource} * D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES,
    .m_uiMipTailOffset    = xiiUInt64{packedMipDescription.StartTileIndexInOverallResource} * D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES,
    .m_uiMipTailStride    = m_Description.IsArray() ? (uiTileCountForEntireResource / m_Description.m_uiArraySizeOrDepth) * D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES : 0,
    .m_uiMipTailSize      = xiiUInt64{packedMipDescription.NumTilesForPackedMips} * D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES,
    .m_uiFirstMipInTail   = packedMipDescription.NumStandardMips,
    .m_vTileSize          = xiiVec3U32{standardTileShapeForNonPackedMips.WidthInTexels, standardTileShapeForNonPackedMips.HeightInTexels, standardTileShapeForNonPackedMips.DepthInTexels},
    .m_uiBlockSize        = D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES,
    .m_Flags              = xiiGALSparseTextureFlags::None,
  };
}

DXGI_FORMAT CorrectDXGIFormat(DXGI_FORMAT DXGIFormat, xiiBitflags<xiiGALBindFlags> bindFlags)
{
  if (bindFlags.IsSet(xiiGALBindFlags::DepthStencil) && (bindFlags != xiiGALBindFlags::DepthStencil))
  {
    switch (DXGIFormat)
    {
      case DXGI_FORMAT_R32_TYPELESS:
      case DXGI_FORMAT_R32_FLOAT:
      case DXGI_FORMAT_D32_FLOAT:
        DXGIFormat = DXGI_FORMAT_R32_TYPELESS;
        break;

      case DXGI_FORMAT_R24G8_TYPELESS:
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
      case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        DXGIFormat = DXGI_FORMAT_R24G8_TYPELESS;
        break;

      case DXGI_FORMAT_R16_TYPELESS:
      case DXGI_FORMAT_R16_UNORM:
      case DXGI_FORMAT_D16_UNORM:
        DXGIFormat = DXGI_FORMAT_R16_TYPELESS;
        break;

      case DXGI_FORMAT_R32G8X24_TYPELESS:
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        DXGIFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
        break;

      default:
        XII_REPORT_FAILURE("Unsupported depth-stencil format");
        break;
    }
  }

  if (bindFlags == xiiGALBindFlags::DepthStencil)
  {
    switch (DXGIFormat)
    {
      case DXGI_FORMAT_R32_TYPELESS:
      case DXGI_FORMAT_R32_FLOAT:
        DXGIFormat = DXGI_FORMAT_D32_FLOAT;
        break;

      case DXGI_FORMAT_R24G8_TYPELESS:
      case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        DXGIFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;

      case DXGI_FORMAT_R16_TYPELESS:
      case DXGI_FORMAT_R16_UNORM:
        DXGIFormat = DXGI_FORMAT_D16_UNORM;
        break;
    }
  }

  if (bindFlags == xiiGALBindFlags::ShaderResource || bindFlags == xiiGALBindFlags::UnorderedAccess)
  {
    switch (DXGIFormat)
    {
      case DXGI_FORMAT_R32_TYPELESS:
      case DXGI_FORMAT_D32_FLOAT:
        DXGIFormat = DXGI_FORMAT_R32_FLOAT;
        break;

      case DXGI_FORMAT_R24G8_TYPELESS:
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
        DXGIFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        break;

      case DXGI_FORMAT_R16_TYPELESS:
      case DXGI_FORMAT_D16_UNORM:
        DXGIFormat = DXGI_FORMAT_R16_UNORM;
        break;
    }
  }

  return DXGIFormat;
}

DXGI_FORMAT ResourceFormatToDXGI_Format(xiiEnum<xiiGALResourceFormat> textureFormat, xiiBitflags<xiiGALBindFlags> bindFlags)
{
  static bool        bFormatMapInitialized                             = false;
  static DXGI_FORMAT FmtToDXGIFmtMap[xiiGALResourceFormat::ENUM_COUNT] = {DXGI_FORMAT_UNKNOWN};
  if (!bFormatMapInitialized)
  {
    FmtToDXGIFmtMap[xiiGALResourceFormat::Unknown] = DXGI_FORMAT_UNKNOWN;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA32Typeless] = DXGI_FORMAT_R32G32B32A32_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA32Float]    = DXGI_FORMAT_R32G32B32A32_FLOAT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA32UInt]     = DXGI_FORMAT_R32G32B32A32_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA32SInt]     = DXGI_FORMAT_R32G32B32A32_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RGB32Typeless] = DXGI_FORMAT_R32G32B32_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGB32Float]    = DXGI_FORMAT_R32G32B32_FLOAT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGB32UInt]     = DXGI_FORMAT_R32G32B32_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGB32SInt]     = DXGI_FORMAT_R32G32B32_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA16Typeless]    = DXGI_FORMAT_R16G16B16A16_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA16Float]       = DXGI_FORMAT_R16G16B16A16_FLOAT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA16UNormalized] = DXGI_FORMAT_R16G16B16A16_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA16UInt]        = DXGI_FORMAT_R16G16B16A16_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA16SNormalized] = DXGI_FORMAT_R16G16B16A16_SNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA16SInt]        = DXGI_FORMAT_R16G16B16A16_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RG32Typeless] = DXGI_FORMAT_R32G32_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG32Float]    = DXGI_FORMAT_R32G32_FLOAT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG32UInt]     = DXGI_FORMAT_R32G32_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG32SInt]     = DXGI_FORMAT_R32G32_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::R32G8X24Typeless]      = DXGI_FORMAT_R32G8X24_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::D32FloatS8X24UInt]     = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R32FloatX8X24Typeless] = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::X32TypelessG8X24UInt]  = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RGB10A2Typeless]    = DXGI_FORMAT_R10G10B10A2_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGB10A2UNormalized] = DXGI_FORMAT_R10G10B10A2_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGB10A2UInt]        = DXGI_FORMAT_R10G10B10A2_UINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RG11B10Float] = DXGI_FORMAT_R11G11B10_FLOAT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA8Typeless]        = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA8UNormalized]     = DXGI_FORMAT_R8G8B8A8_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA8UNormalizedSRGB] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA8UInt]            = DXGI_FORMAT_R8G8B8A8_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA8SNormalized]     = DXGI_FORMAT_R8G8B8A8_SNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGBA8SInt]            = DXGI_FORMAT_R8G8B8A8_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RG16Typeless]    = DXGI_FORMAT_R16G16_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG16Float]       = DXGI_FORMAT_R16G16_FLOAT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG16UNormalized] = DXGI_FORMAT_R16G16_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG16UInt]        = DXGI_FORMAT_R16G16_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG16SNormalized] = DXGI_FORMAT_R16G16_SNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG16SInt]        = DXGI_FORMAT_R16G16_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::R32Typeless] = DXGI_FORMAT_R32_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::D32Float]    = DXGI_FORMAT_D32_FLOAT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R32Float]    = DXGI_FORMAT_R32_FLOAT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R32UInt]     = DXGI_FORMAT_R32_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R32SInt]     = DXGI_FORMAT_R32_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::R24G8Typeless]            = DXGI_FORMAT_R24G8_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::D24UNormalizedS8UInt]     = DXGI_FORMAT_D24_UNORM_S8_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R24UNormalizedX8Typeless] = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::X24TypelessG8UInt]        = DXGI_FORMAT_X24_TYPELESS_G8_UINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::RG8Typeless]    = DXGI_FORMAT_R8G8_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG8UNormalized] = DXGI_FORMAT_R8G8_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG8UInt]        = DXGI_FORMAT_R8G8_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG8SNormalized] = DXGI_FORMAT_R8G8_SNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG8SInt]        = DXGI_FORMAT_R8G8_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::R16Typeless]    = DXGI_FORMAT_R16_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R16Float]       = DXGI_FORMAT_R16_FLOAT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::D16UNormalized] = DXGI_FORMAT_D16_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R16UNormalized] = DXGI_FORMAT_R16_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R16UInt]        = DXGI_FORMAT_R16_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R16SNormalized] = DXGI_FORMAT_R16_SNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R16SInt]        = DXGI_FORMAT_R16_SINT;

    FmtToDXGIFmtMap[xiiGALResourceFormat::R8Typeless]    = DXGI_FORMAT_R8_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R8UNormalized] = DXGI_FORMAT_R8_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R8UInt]        = DXGI_FORMAT_R8_UINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R8SNormalized] = DXGI_FORMAT_R8_SNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::R8SInt]        = DXGI_FORMAT_R8_SINT;
    FmtToDXGIFmtMap[xiiGALResourceFormat::A8UNormalized] = DXGI_FORMAT_A8_UNORM;

    FmtToDXGIFmtMap[xiiGALResourceFormat::R1UNormalized]        = DXGI_FORMAT_R1_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RGB9E5SharedExponent] = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    FmtToDXGIFmtMap[xiiGALResourceFormat::RG8BG8UNormalized]    = DXGI_FORMAT_R8G8_B8G8_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::GR8GB8UNormalized]    = DXGI_FORMAT_G8R8_G8B8_UNORM;

    FmtToDXGIFmtMap[xiiGALResourceFormat::BC1Typeless]        = DXGI_FORMAT_BC1_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC1UNormalized]     = DXGI_FORMAT_BC1_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC1UNormalizedSRGB] = DXGI_FORMAT_BC1_UNORM_SRGB;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC2Typeless]        = DXGI_FORMAT_BC2_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC2UNormalized]     = DXGI_FORMAT_BC2_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC2UNormalizedSRGB] = DXGI_FORMAT_BC2_UNORM_SRGB;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC3Typeless]        = DXGI_FORMAT_BC3_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC3UNormalized]     = DXGI_FORMAT_BC3_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC3UNormalizedSRGB] = DXGI_FORMAT_BC3_UNORM_SRGB;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC4Typeless]        = DXGI_FORMAT_BC4_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC4UNormalized]     = DXGI_FORMAT_BC4_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC4SNormalized]     = DXGI_FORMAT_BC4_SNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC5Typeless]        = DXGI_FORMAT_BC5_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC5UNormalized]     = DXGI_FORMAT_BC5_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC5SNormalized]     = DXGI_FORMAT_BC5_SNORM;

    FmtToDXGIFmtMap[xiiGALResourceFormat::B5G6R5UNormalized]   = DXGI_FORMAT_B5G6R5_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::B5G5R5A1UNormalized] = DXGI_FORMAT_B5G5R5A1_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BGRA8UNormalized]    = DXGI_FORMAT_B8G8R8A8_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BGRX8UNormalized]    = DXGI_FORMAT_B8G8R8X8_UNORM;

    FmtToDXGIFmtMap[xiiGALResourceFormat::R10G10B10XRBiasA2UNormalized] = DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

    FmtToDXGIFmtMap[xiiGALResourceFormat::BGRA8Typeless]        = DXGI_FORMAT_B8G8R8A8_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BGRA8UNormalizedSRGB] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BGRX8Typeless]        = DXGI_FORMAT_B8G8R8X8_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BGRX8UNormalizedSRGB] = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

    FmtToDXGIFmtMap[xiiGALResourceFormat::BC6HTypeless]       = DXGI_FORMAT_BC6H_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC6HUF16]           = DXGI_FORMAT_BC6H_UF16;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC6HSF16]           = DXGI_FORMAT_BC6H_SF16;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC7Typeless]        = DXGI_FORMAT_BC7_TYPELESS;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC7UNormalized]     = DXGI_FORMAT_BC7_UNORM;
    FmtToDXGIFmtMap[xiiGALResourceFormat::BC7UNormalizedSRGB] = DXGI_FORMAT_BC7_UNORM_SRGB;

    bFormatMapInitialized = true;
  }

  if (textureFormat >= xiiGALResourceFormat::Unknown && textureFormat < xiiGALResourceFormat::ENUM_COUNT)
  {
    auto DXGIFormat = FmtToDXGIFmtMap[textureFormat];
    XII_ASSERT_DEV(textureFormat == xiiGALResourceFormat::Unknown || DXGIFormat != DXGI_FORMAT_UNKNOWN, "Unsupported texture format");
    if (bindFlags.IsAnyFlagSet())
      DXGIFormat = CorrectDXGIFormat(DXGIFormat, bindFlags);
    return DXGIFormat;
  }
  else
  {
    // UNEXPECTED("Texture format (", TexFormat, ") is out of allowed range [0, ", xiiGALResourceFormat::NUM_FORMATS - 1, "]");
    return DXGI_FORMAT_UNKNOWN;
  }
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_TextureD3D11);
