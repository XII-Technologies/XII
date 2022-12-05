#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererDiligent/Resources/UnorderedAccessViewDiligent.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALUnorderedAccessViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

xiiGALUnorderedAccessViewDiligent::xiiGALUnorderedAccessViewDiligent(
  xiiGALResourceBase*                                 pResource,
  const xiiGALUnorderedAccessViewCreationDescription& Description) :
  xiiGALUnorderedAccessView(pResource, Description), m_pUnorderedAccessView(nullptr)
{
}

xiiGALUnorderedAccessViewDiligent::~xiiGALUnorderedAccessViewDiligent() {}

static Diligent::VALUE_TYPE GALToDiligentFormat(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {

    // 32-bit float (may be normalized)
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_RG16_FLOAT:
      return Diligent::VT_FLOAT32;

    // 16-bit half precision float
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_R16_FLOAT:
      return Diligent::VT_FLOAT16;

    // 32-bit usigned integer (may be normalized)
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RGBA8_UINT:
    case Diligent::TEX_FORMAT_RG16_UNORM:
    case Diligent::TEX_FORMAT_RG16_UINT:
    case Diligent::TEX_FORMAT_R32_UINT:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
    case Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT:
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
      return Diligent::VT_UINT32;

    // 16-bit unsigned integer (may be normalized)
    case Diligent::TEX_FORMAT_RG8_UNORM:
    case Diligent::TEX_FORMAT_RG8_UINT:
    case Diligent::TEX_FORMAT_D16_UNORM:
    case Diligent::TEX_FORMAT_R16_UNORM:
    case Diligent::TEX_FORMAT_R16_UINT:
    case Diligent::TEX_FORMAT_B5G6R5_UNORM:
    case Diligent::TEX_FORMAT_B5G5R5A1_UNORM:
      return Diligent::VT_UINT16;

    // 8-bit unsigned integer (may be normalized)
    case Diligent::TEX_FORMAT_R8_UNORM:
    case Diligent::TEX_FORMAT_R8_UINT:
    case Diligent::TEX_FORMAT_A8_UNORM:
      return Diligent::VT_UINT8;

    // 32-bit signed integer (may be normalized)
    case Diligent::TEX_FORMAT_RGBA8_SNORM:
    case Diligent::TEX_FORMAT_RGBA8_SINT:
    case Diligent::TEX_FORMAT_RG16_SNORM:
    case Diligent::TEX_FORMAT_RG16_SINT:
    case Diligent::TEX_FORMAT_R32_SINT:
      return Diligent::VT_INT32;

    // 16-bit signed integer (may be normalized)
    case Diligent::TEX_FORMAT_RG8_SNORM:
    case Diligent::TEX_FORMAT_RG8_SINT:
    case Diligent::TEX_FORMAT_R16_SNORM:
    case Diligent::TEX_FORMAT_R16_SINT:
      return Diligent::VT_INT16;

    // 8-bit signed integer (may be normalized)
    case Diligent::TEX_FORMAT_R8_SNORM:
    case Diligent::TEX_FORMAT_R8_SINT:
      return Diligent::VT_INT8;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  return Diligent::VT_UNDEFINED;
}

static xiiUInt8 GALToDiligentNumComponent(Diligent::TEXTURE_FORMAT format)
{
  switch (format)
  {
    // 1 Component
    case Diligent::TEX_FORMAT_D32_FLOAT:
    case Diligent::TEX_FORMAT_R32_FLOAT:
    case Diligent::TEX_FORMAT_R16_FLOAT:
    case Diligent::TEX_FORMAT_R32_UINT:
    case Diligent::TEX_FORMAT_D16_UNORM:
    case Diligent::TEX_FORMAT_R16_UNORM:
    case Diligent::TEX_FORMAT_R16_UINT:
    case Diligent::TEX_FORMAT_R8_UNORM:
    case Diligent::TEX_FORMAT_R8_UINT:
    case Diligent::TEX_FORMAT_A8_UNORM:
    case Diligent::TEX_FORMAT_R32_SINT:
    case Diligent::TEX_FORMAT_R16_SNORM:
    case Diligent::TEX_FORMAT_R16_SINT:
    case Diligent::TEX_FORMAT_R8_SNORM:
    case Diligent::TEX_FORMAT_R8_SINT:
      return 1;

    // 2 Component
    case Diligent::TEX_FORMAT_RG16_FLOAT:
    case Diligent::TEX_FORMAT_RG16_UNORM:
    case Diligent::TEX_FORMAT_RG16_UINT:
    case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
    case Diligent::TEX_FORMAT_X24_TYPELESS_G8_UINT:
    case Diligent::TEX_FORMAT_RG8_UNORM:
    case Diligent::TEX_FORMAT_RG8_UINT:
    case Diligent::TEX_FORMAT_RG16_SNORM:
    case Diligent::TEX_FORMAT_RG16_SINT:
    case Diligent::TEX_FORMAT_RG8_SNORM:
    case Diligent::TEX_FORMAT_RG8_SINT:
      return 2;

    // 3 Component
    case Diligent::TEX_FORMAT_RGB32_FLOAT:
    case Diligent::TEX_FORMAT_R11G11B10_FLOAT:
    case Diligent::TEX_FORMAT_B5G6R5_UNORM:
      return 3;

    // 4 Component
    case Diligent::TEX_FORMAT_RGBA16_FLOAT:
    case Diligent::TEX_FORMAT_RGB10A2_UNORM:
    case Diligent::TEX_FORMAT_RGB10A2_UINT:
    case Diligent::TEX_FORMAT_RGBA8_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
    case Diligent::TEX_FORMAT_RGBA8_UINT:
    case Diligent::TEX_FORMAT_BGRA8_UNORM:
    case Diligent::TEX_FORMAT_BGRX8_UNORM:
    case Diligent::TEX_FORMAT_B5G5R5A1_UNORM:
    case Diligent::TEX_FORMAT_RGBA8_SNORM:
    case Diligent::TEX_FORMAT_RGBA8_SINT:
      return 4;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  return 0;
}

xiiResult xiiGALUnorderedAccessViewDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  const xiiGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const xiiGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    xiiLog::Error("No valid texture handle or buffer handle given for unordered access view creation!");
    return XII_FAILURE;
  }


  xiiGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    const xiiGALTextureCreationDescription& TexDesc = pTexture->GetDescription();

    if (ViewFormat == xiiGALResourceFormat::Invalid)
      ViewFormat = TexDesc.m_Format;
  }

  xiiGALDeviceDiligent* pDXDevice = static_cast<xiiGALDeviceDiligent*>(pDevice);


  Diligent::TEXTURE_FORMAT ViewFormatDiligent = Diligent::TEX_FORMAT_UNKNOWN;
  if (xiiGALResourceFormat::IsDepthFormat(ViewFormat))
  {
    ViewFormatDiligent = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eDepthOnlyType;
  }
  else
  {
    ViewFormatDiligent = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eResourceViewType;
  }

  if (ViewFormatDiligent == Diligent::TEX_FORMAT_UNKNOWN)
  {
    xiiLog::Error("Couldn't get valid DXGI format for resource view! ({0})", ViewFormat);
    return XII_FAILURE;
  }

  if (pTexture)
  {
    xiiGALResourceBase*                          pRes             = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
    Diligent::RefCntAutoPtr<Diligent::ITexture>& pTextureDiligent = static_cast<xiiGALTextureDiligent*>(pRes)->GetTexture();
    const xiiGALTextureCreationDescription&      texDesc          = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    Diligent::RefCntAutoPtr<Diligent::ITextureView> pTexView;
    Diligent::TextureViewDesc                       UAVDesc;
    UAVDesc.ViewType = Diligent::TEXTURE_VIEW_UNORDERED_ACCESS;

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DProxy:
      {
        if (!bIsArrayView)
        {
          UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D;
          UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
        }
        else
        {
          UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
          UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
          UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
          UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        }
      }
      break;

      case xiiGALTextureType::TextureCube:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
        UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
        UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
      }
      break;

      case xiiGALTextureType::Texture3D:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_3D;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
        UAVDesc.FirstDepthSlice = m_Description.m_uiFirstArraySlice;
        UAVDesc.NumDepthSlices  = m_Description.m_uiArraySize;
      }
      break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;
    }

    pTextureDiligent->CreateView(UAVDesc, &pTexView);

    if (!pTexView)
    {
      xiiLog::Error("Failed to create texture view.");
    }

    m_pUnorderedAccessView = pTexView;
  }
  else if (pBuffer)
  {
    xiiGALResourceBase*                         pRes            = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
    Diligent::RefCntAutoPtr<Diligent::IBuffer>& pBufferDiligent = static_cast<xiiGALBufferDiligent*>(pRes)->GetBuffer();

    Diligent::RefCntAutoPtr<Diligent::IBufferView> pBufferView;
    Diligent::BufferViewDesc                       UAVDesc;
    UAVDesc.Format.ValueType     = GALToDiligentFormat(ViewFormatDiligent);
    UAVDesc.Format.NumComponents = GALToDiligentNumComponent(ViewFormatDiligent);
    UAVDesc.ViewType             = Diligent::BUFFER_VIEW_UNORDERED_ACCESS;

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
      UAVDesc.Format.ValueType = Diligent::VT_UNDEFINED;

    UAVDesc.ByteOffset = m_Description.m_uiFirstElement;
    UAVDesc.ByteWidth  = m_Description.m_uiNumElements;

    pBufferDiligent->CreateView(UAVDesc, &pBufferView);

    if (!pBufferView)
    {
      xiiLog::Error("Failed to create buffer view.");
      return XII_FAILURE;
    }

    m_pUnorderedAccessView = pBufferView;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALUnorderedAccessViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_RELEASE(m_pUnorderedAccessView);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_UnorderedAccessViewDiligent);
