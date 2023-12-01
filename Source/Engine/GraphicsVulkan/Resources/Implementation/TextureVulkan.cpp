#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

xiiGALTextureVulkan::xiiGALTextureVulkan(const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(creationDescription)
{
}

xiiGALTextureVulkan::~xiiGALTextureVulkan() = default;

xiiResult xiiGALTextureVulkan::InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    m_pTexture = static_cast<Diligent::ITexture*>(m_Description.m_pExisitingNativeObject);

    return XII_SUCCESS;
  }

  Diligent::TextureDesc textureDescription;
  textureDescription.Name                 = m_Description.m_sName.GetStartPointer();
  textureDescription.Type                 = xiiDiligentTypeConversions::GetResourceDimension(m_Description.m_Type);
  textureDescription.Width                = m_Description.m_Size.width;
  textureDescription.Height               = m_Description.m_Size.height;
  textureDescription.Format               = xiiDiligentTypeConversions::GetTextureFormat(m_Description.m_Format);
  textureDescription.MipLevels            = m_Description.m_uiMipLevels;
  textureDescription.SampleCount          = m_Description.m_uiSampleCount;
  textureDescription.BindFlags            = xiiDiligentTypeConversions::GetBindFlags(m_Description.m_BindFlags);
  textureDescription.Usage                = xiiDiligentTypeConversions::GetUsage(m_Description.m_Usage);
  textureDescription.CPUAccessFlags       = xiiDiligentTypeConversions::GetCPUAccessFlags(m_Description.m_CPUAccessFlags);
  textureDescription.MiscFlags            = xiiDiligentTypeConversions::GetMiscTextureFlags(m_Description.m_MiscFlags);
  textureDescription.ImmediateContextMask = m_Description.m_uiImmediateContextMask;

  if (m_Description.Is3D())
    textureDescription.Depth = m_Description.m_uiArraySizeOrDepth;
  else
    textureDescription.ArraySize = m_Description.m_uiArraySizeOrDepth;

  if (pInitialData != nullptr)
  {
    const xiiUInt32 uiSubresourceCount = pInitialData->m_SubResources.GetCount();

    Diligent::TextureData initialData;
    initialData.NumSubresources = uiSubresourceCount;

    xiiHybridArray<Diligent::TextureSubResData, 16> subResources;
    subResources.SetCount(uiSubresourceCount);

    for (xiiUInt32 i = 0; i < uiSubresourceCount; ++i)
    {
      auto& data        = pInitialData->m_SubResources[i];
      auto& subresource = subResources[i];

      subresource.pData       = data.m_pData;
      subresource.SrcOffset   = data.m_uiSourceOffset;
      subresource.Stride      = data.m_uiStride;
      subresource.DepthStride = data.m_uiDepthStride;
    }

    initialData.pSubResources = subResources.GetData();

    pDeviceVulkan->GetDevice()->CreateTexture(textureDescription, &initialData, &m_pTexture);
  }
  else
  {
    pDeviceVulkan->GetDevice()->CreateTexture(textureDescription, nullptr, &m_pTexture);
  }

  return (m_pTexture != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALTextureVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  // Prevent releasing native objects.
  if (m_Description.m_pExisitingNativeObject == nullptr)
  {
    XII_GAL_DILIGENT_PTR_RELEASE(m_pTexture);
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TextureVulkan);
