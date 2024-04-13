#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>

#include <d3d11_2.h>

xiiGALTextureD3D11::xiiGALTextureD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(pDeviceD3D11, creationDescription)
{
}

xiiGALTextureD3D11::~xiiGALTextureD3D11() = default;

xiiResult xiiGALTextureD3D11::InitPlatform(const xiiGALTextureData* pInitialData)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

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
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

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
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

  DXGI_SAMPLE_DESC sampleDescription = {.Count = m_Description.m_uiSampleCount, .Quality = 0U};

  D3D11_TEXTURE2D_DESC textureDescription = {};
  textureDescription.Width                = m_Description.m_Size.width;
  textureDescription.Height               = m_Description.m_Size.height;
  textureDescription.MipLevels            = m_Description.m_uiMipLevels;
  textureDescription.ArraySize            = m_Description.GetArraySize();
  textureDescription.Format               = xiiD3D11TypeConversions::GetFormat(m_Description.m_Format);
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
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

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
      out_InitialData[i].pSysMem          = pInitialData->m_SubResources[i].m_pData;
      out_InitialData[i].SysMemPitch      = static_cast<xiiUInt32>(pInitialData->m_SubResources[i].m_uiStride);
      out_InitialData[i].SysMemSlicePitch = static_cast<xiiUInt32>(pInitialData->m_SubResources[i].m_uiDepthStride);
    }
  }
}

void xiiGALTextureD3D11::InitializeSparseTextureProperties()
{
  if (m_Description.m_Usage != xiiGALResourceUsage::Sparse)
    return;

  xiiGALDeviceD3D11* pDeviceD3D11   = static_cast<xiiGALDeviceD3D11*>(m_pDevice);
  ID3D11Device2*     pDeviceD3D11_2 = static_cast<ID3D11Device2*>(pDeviceD3D11->GetD3D11Device());

  xiiUInt32             uiTileCountForEntireResource      = 0;
  xiiUInt32             uiSubresourceTilingCount          = 0U;
  D3D11_TILE_SHAPE      standardTileShapeForNonPackedMips = {};
  D3D11_PACKED_MIP_DESC packedMipDescription              = {};
  pDeviceD3D11_2->GetResourceTiling(m_pTexture, &uiTileCountForEntireResource, &packedMipDescription, &standardTileShapeForNonPackedMips, &uiSubresourceTilingCount, 0, nullptr);

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

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_TextureD3D11);
