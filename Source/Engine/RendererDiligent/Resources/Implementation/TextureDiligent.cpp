#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>

xiiVec3U32 xiiGALTextureDiligent::GetMipLevelSize(xiiUInt32 uiMipLevelSize) const
{
  xiiVec3U32 size = {m_Description.m_uiWidth, m_Description.m_uiHeight, m_Description.m_uiDepth};
  size.x          = xiiMath::Max(1u, size.x >> uiMipLevelSize);
  size.y          = xiiMath::Max(1u, size.y >> uiMipLevelSize);
  size.z          = xiiMath::Max(1u, size.z >> uiMipLevelSize);
  return size;
}

xiiGALTextureDiligent::xiiGALTextureDiligent(const xiiGALTextureCreationDescription& Description) :
  xiiGALTexture(Description), m_pTexture(nullptr), m_pStagingTexture(nullptr), m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
{
}

xiiGALTextureDiligent::~xiiGALTextureDiligent() {}

xiiResult xiiGALTextureDiligent::InitPlatform(xiiGALDevice* pDevice, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  if (m_pExisitingNativeObject != nullptr)
  {
    /// \todo Validation if interface of corresponding texture object exists
    if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    {
      xiiResult result = CreateStagingTexture(pDeviceDiligent);
      if (result == XII_FAILURE)
      {
        m_pTexture = nullptr;
        return result;
      }
    }
    return XII_SUCCESS;
  }

  switch (m_Description.m_Type)
  {
    case xiiGALTextureType::Texture2D:
    case xiiGALTextureType::TextureCube:
    {
      Diligent::TextureDesc Tex2DDesc;
      Tex2DDesc.Name      = m_Description.m_szName;
      Tex2DDesc.ArraySize = (m_Description.m_Type == xiiGALTextureType::Texture2D ? m_Description.m_uiArraySize : (m_Description.m_uiArraySize * 6));
      Tex2DDesc.BindFlags = Diligent::BIND_NONE;

      if (m_Description.m_bAllowShaderResourceView || m_Description.m_bAllowDynamicMipGeneration)
        Tex2DDesc.BindFlags |= Diligent::BIND_SHADER_RESOURCE;
      if (m_Description.m_bAllowUAV)
        Tex2DDesc.BindFlags |= Diligent::BIND_UNORDERED_ACCESS;

      if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowDynamicMipGeneration)
        Tex2DDesc.BindFlags |= xiiGALResourceFormat::IsDepthFormat(m_Description.m_Format) ? Diligent::BIND_DEPTH_STENCIL : Diligent::BIND_RENDER_TARGET;

      Tex2DDesc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE; // We always use staging textures to update the data
      Tex2DDesc.Usage          = m_Description.m_ResourceAccess.IsImmutable() ? Diligent::USAGE_IMMUTABLE : Diligent::USAGE_DEFAULT;

      if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowUAV)
        Tex2DDesc.Usage = Diligent::USAGE_DEFAULT;

      Tex2DDesc.Format = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

      if (Tex2DDesc.Format == Diligent::TEX_FORMAT_UNKNOWN)
      {
        xiiLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
        return XII_FAILURE;
      }

      Tex2DDesc.Width     = m_Description.m_uiWidth;
      Tex2DDesc.Height    = m_Description.m_uiHeight;
      Tex2DDesc.MipLevels = m_Description.m_uiMipLevelCount;

      Tex2DDesc.MiscFlags = Diligent::MISC_TEXTURE_FLAG_NONE;

      if (m_Description.m_bAllowDynamicMipGeneration)
        Tex2DDesc.MiscFlags |= Diligent::MISC_TEXTURE_FLAG_GENERATE_MIPS;

      if (m_Description.m_Type == xiiGALTextureType::TextureCube)
        Tex2DDesc.Type = Diligent::RESOURCE_DIM_TEX_CUBE;
      else
        Tex2DDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;

      Tex2DDesc.SampleCount = m_Description.m_SampleCount;

      if (!pInitialData.IsEmpty())
      {
        xiiUInt32 uiInitialDataCount = (m_Description.m_uiMipLevelCount * Tex2DDesc.ArraySize);
        XII_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

        m_InitialData.SetCount(uiInitialDataCount);

        for (xiiUInt32 i = 0; i < uiInitialDataCount; i++)
        {
          m_InitialData[i].pData       = pInitialData[i].m_pData;
          m_InitialData[i].Stride      = pInitialData[i].m_uiRowPitch;
          m_InitialData[i].DepthStride = pInitialData[i].m_uiSlicePitch;
        }
      }

      if (!pInitialData.IsEmpty())
      {
        m_TextureData.pSubResources   = m_InitialData.GetData();
        m_TextureData.NumSubresources = m_InitialData.GetCount();
        pDeviceDiligent->GetDevice()->CreateTexture(Tex2DDesc, &m_TextureData, &m_pTexture);
      }
      else
      {
        pDeviceDiligent->GetDevice()->CreateTexture(Tex2DDesc, nullptr, &m_pTexture);
      }

      if (m_pTexture == nullptr)
      {
        return XII_FAILURE;
      }
      else
      {
        if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
          return CreateStagingTexture(pDeviceDiligent);

        return XII_SUCCESS;
      }
    }
    break;

    case xiiGALTextureType::Texture3D:
    {
      Diligent::TextureDesc Tex3DDesc;
      Tex3DDesc.Name      = m_Description.m_szName;
      Tex3DDesc.BindFlags = Diligent::BIND_NONE;

      if (m_Description.m_bAllowShaderResourceView)
        Tex3DDesc.BindFlags |= Diligent::BIND_SHADER_RESOURCE;
      if (m_Description.m_bAllowUAV)
        Tex3DDesc.BindFlags |= Diligent::BIND_UNORDERED_ACCESS;

      if (m_Description.m_bCreateRenderTarget)
        Tex3DDesc.BindFlags |= xiiGALResourceFormat::IsDepthFormat(m_Description.m_Format) ? Diligent::BIND_DEPTH_STENCIL : Diligent::BIND_RENDER_TARGET;

      Tex3DDesc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE; // We always use staging textures to update the data
      Tex3DDesc.Usage          = m_Description.m_ResourceAccess.IsImmutable() ? Diligent::USAGE_IMMUTABLE : Diligent::USAGE_DEFAULT;

      if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowUAV)
        Tex3DDesc.Usage = Diligent::USAGE_DEFAULT;

      Tex3DDesc.Format = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

      if (Tex3DDesc.Format == Diligent::TEX_FORMAT_UNKNOWN)
      {
        xiiLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
        return XII_FAILURE;
      }

      Tex3DDesc.Width     = m_Description.m_uiWidth;
      Tex3DDesc.Height    = m_Description.m_uiHeight;
      Tex3DDesc.Depth     = m_Description.m_uiDepth;
      Tex3DDesc.MipLevels = m_Description.m_uiMipLevelCount;

      Tex3DDesc.MiscFlags = Diligent::MISC_TEXTURE_FLAG_NONE;

      if (m_Description.m_bAllowDynamicMipGeneration)
        Tex3DDesc.MiscFlags |= Diligent::MISC_TEXTURE_FLAG_GENERATE_MIPS;

      if (m_Description.m_Type == xiiGALTextureType::TextureCube)
        Tex3DDesc.Type = Diligent::RESOURCE_DIM_TEX_CUBE;
      else
        Tex3DDesc.Type = Diligent::RESOURCE_DIM_TEX_3D;

      if (!pInitialData.IsEmpty())
      {
        const xiiUInt32 uiInitialDataCount = m_Description.m_uiMipLevelCount;
        XII_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

        m_InitialData.SetCount(uiInitialDataCount);

        for (xiiUInt32 i = 0; i < uiInitialDataCount; i++)
        {
          m_InitialData[i].pData       = pInitialData[i].m_pData;
          m_InitialData[i].Stride      = pInitialData[i].m_uiRowPitch;
          m_InitialData[i].DepthStride = pInitialData[i].m_uiSlicePitch;
        }
      }

      m_TextureData.pSubResources   = m_InitialData.GetData();
      m_TextureData.NumSubresources = m_InitialData.GetCount();
      pDeviceDiligent->GetDevice()->CreateTexture(Tex3DDesc, pInitialData.IsEmpty() ? nullptr : &m_TextureData, &m_pTexture);

      if (m_pTexture == nullptr)
      {
        return XII_FAILURE;
      }
      else
      {
        if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
          return CreateStagingTexture(pDeviceDiligent);

        return XII_SUCCESS;
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;

      return XII_FAILURE;
  }

  return XII_FAILURE;
}

xiiResult xiiGALTextureDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pTexture);
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pStagingTexture);

  return XII_SUCCESS;
}

xiiResult xiiGALTextureDiligent::CreateStagingTexture(xiiGALDeviceDiligent* pDevice)
{
  switch (m_Description.m_Type)
  {
    case xiiGALTextureType::Texture2D:
    case xiiGALTextureType::TextureCube:
    {
      Diligent::TextureDesc Desc = m_pTexture->GetDesc();
      Desc.BindFlags             = Diligent::BIND_NONE;
      Desc.CPUAccessFlags        = Diligent::CPU_ACCESS_NONE;
      Desc.Usage                 = Diligent::USAGE_STAGING;
      Desc.SampleCount           = 1; // Disable MSAA for the readback texture, the conversion needs to happen during readback!

      if (m_Description.m_ResourceAccess.m_bReadBack)
        Desc.CPUAccessFlags = Diligent::CPU_ACCESS_READ;

      if (!m_Description.m_ResourceAccess.IsImmutable())
        Desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

      pDevice->GetDevice()->CreateTexture(Desc, nullptr, &m_pStagingTexture);

      if (m_pStagingTexture == nullptr)
      {
        xiiLog::Error("Failed to create staging resource for data upload and/or read back!");
        return XII_FAILURE;
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_TextureDiligent);
