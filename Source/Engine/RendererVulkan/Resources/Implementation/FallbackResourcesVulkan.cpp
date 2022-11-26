#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

xiiGALDeviceVulkan*    xiiFallbackResourcesVulkan::s_pDevice = nullptr;
xiiEventSubscriptionID xiiFallbackResourcesVulkan::s_EventID = 0;

xiiHashTable<xiiFallbackResourcesVulkan::Key, xiiGALResourceViewHandle, xiiFallbackResourcesVulkan::KeyHash>        xiiFallbackResourcesVulkan::m_ResourceViews;
xiiHashTable<xiiFallbackResourcesVulkan::Key, xiiGALUnorderedAccessViewHandle, xiiFallbackResourcesVulkan::KeyHash> xiiFallbackResourcesVulkan::m_UAVs;
xiiDynamicArray<xiiGALBufferHandle>                                                                                 xiiFallbackResourcesVulkan::m_Buffers;
xiiDynamicArray<xiiGALTextureHandle>                                                                                xiiFallbackResourcesVulkan::m_Textures;

void xiiFallbackResourcesVulkan::Initialize(xiiGALDeviceVulkan* pDevice)
{
  s_pDevice = pDevice;
  s_EventID = pDevice->m_Events.AddEventHandler(xiiMakeDelegate(&xiiFallbackResourcesVulkan::GALDeviceEventHandler));
}

void xiiFallbackResourcesVulkan::DeInitialize()
{
  s_pDevice->m_Events.RemoveEventHandler(s_EventID);
  s_pDevice = nullptr;
}
void xiiFallbackResourcesVulkan::GALDeviceEventHandler(const xiiGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGALDeviceEvent::AfterInit:
    {
      auto CreateTexture = [](xiiGALTextureType::Enum type, xiiGALMSAASampleCount::Enum samples, bool bDepth) -> xiiGALResourceViewHandle {
        xiiGALTextureCreationDescription desc;
        desc.m_uiWidth  = 4;
        desc.m_uiHeight = 4;
        if (type == xiiGALTextureType::Texture3D)
          desc.m_uiDepth = 4;
        desc.m_uiMipLevelCount             = 1;
        desc.m_Format                      = bDepth ? xiiGALResourceFormat::D16 : xiiGALResourceFormat::BGRAUByteNormalizedsRGB;
        desc.m_Type                        = type;
        desc.m_SampleCount                 = samples;
        desc.m_ResourceAccess.m_bImmutable = false;
        desc.m_bCreateRenderTarget         = bDepth;
        xiiGALTextureHandle hTexture       = s_pDevice->CreateTexture(desc);
        XII_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResourceVulkan");
        m_Textures.PushBack(hTexture);
        return s_pDevice->GetDefaultResourceView(hTexture);
      };
      {
        xiiGALResourceViewHandle hView                                                                     = CreateTexture(xiiGALTextureType::Texture2D, xiiGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::Texture2D, false}]      = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::Texture2DArray, false}] = hView;
      }
      {
        xiiGALResourceViewHandle hView                                                                    = CreateTexture(xiiGALTextureType::Texture2D, xiiGALMSAASampleCount::None, true);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::Texture2D, true}]      = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::Texture2DArray, true}] = hView;
      }

      // Swift shader can only do 4x MSAA. Add a check anyways.
      vk::ImageFormatProperties props;
      vk::Result                res = s_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(vk::Format::eB8G8R8A8Srgb, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled, {}, &props);
      if (res == vk::Result::eSuccess && props.sampleCounts & vk::SampleCountFlagBits::e4)
      {
        xiiGALResourceViewHandle hView                                                                       = CreateTexture(xiiGALTextureType::Texture2D, xiiGALMSAASampleCount::FourSamples, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::Texture2DMS, false}]      = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::Texture2DMSArray, false}] = hView;
      }
      {
        xiiGALResourceViewHandle hView                                                                       = CreateTexture(xiiGALTextureType::TextureCube, xiiGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::TextureCube, false}]      = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::TextureCubeArray, false}] = hView;
      }
      {
        xiiGALResourceViewHandle hView                                                                = CreateTexture(xiiGALTextureType::Texture3D, xiiGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, xiiShaderResourceType::Texture3D, false}] = hView;
      }
      {
        xiiGALBufferCreationDescription desc;
        desc.m_bUseForIndirectArguments    = false;
        desc.m_bUseAsStructuredBuffer      = true;
        desc.m_bAllowRawViews              = true;
        desc.m_bStreamOutputTarget         = false;
        desc.m_bAllowShaderResourceView    = true;
        desc.m_bAllowUAV                   = true;
        desc.m_uiStructSize                = 128;
        desc.m_uiTotalSize                 = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        xiiGALBufferHandle hBuffer         = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        xiiGALResourceViewHandle hView                                                                      = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{vk::DescriptorType::eUniformBuffer, xiiShaderResourceType::ConstantBuffer, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eUniformBuffer, xiiShaderResourceType::ConstantBuffer, true}]  = hView;
        m_ResourceViews[{vk::DescriptorType::eStorageBuffer, xiiShaderResourceType::GenericBuffer, false}]  = hView;
        m_ResourceViews[{vk::DescriptorType::eStorageBuffer, xiiShaderResourceType::GenericBuffer, true}]   = hView;
      }
      {
        xiiGALBufferCreationDescription desc;
        desc.m_uiStructSize                = sizeof(xiiUInt32);
        desc.m_uiTotalSize                 = 1024;
        desc.m_bAllowShaderResourceView    = true;
        desc.m_ResourceAccess.m_bImmutable = false;
        xiiGALBufferHandle hBuffer         = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        xiiGALResourceViewHandle hView                                                                          = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{vk::DescriptorType::eUniformTexelBuffer, xiiShaderResourceType::GenericBuffer, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eUniformTexelBuffer, xiiShaderResourceType::GenericBuffer, true}]  = hView;
      }
    }
    break;
    case xiiGALDeviceEvent::BeforeShutdown:
    {
      m_ResourceViews.Clear();
      m_ResourceViews.Compact();

      m_UAVs.Clear();
      m_UAVs.Compact();

      for (xiiGALBufferHandle hBuffer : m_Buffers)
      {
        s_pDevice->DestroyBuffer(hBuffer);
      }
      m_Buffers.Clear();
      m_Buffers.Compact();

      for (xiiGALTextureHandle hTexture : m_Textures)
      {
        s_pDevice->DestroyTexture(hTexture);
      }
      m_Textures.Clear();
      m_Textures.Compact();
    }
    break;
    default:
      break;
  }
}

const xiiGALResourceViewVulkan* xiiFallbackResourcesVulkan::GetFallbackResourceView(vk::DescriptorType descriptorType, xiiShaderResourceType::Enum xiiType, bool bDepth)
{
  if (xiiGALResourceViewHandle* pView = m_ResourceViews.GetValue(Key{descriptorType, xiiType, bDepth}))
  {
    return static_cast<const xiiGALResourceViewVulkan*>(s_pDevice->GetResourceView(*pView));
  }
  XII_REPORT_FAILURE("No fallback resource set, update xiiFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

const xiiGALUnorderedAccessViewVulkan* xiiFallbackResourcesVulkan::GetFallbackUnorderedAccessView(vk::DescriptorType descriptorType, xiiShaderResourceType::Enum xiiType)
{
  if (xiiGALUnorderedAccessViewHandle* pView = m_UAVs.GetValue(Key{descriptorType, xiiType, false}))
  {
    return static_cast<const xiiGALUnorderedAccessViewVulkan*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  XII_REPORT_FAILURE("No fallback resource set, update xiiFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

xiiUInt32 xiiFallbackResourcesVulkan::KeyHash::Hash(const Key& a)
{
  xiiHashStreamWriter32 writer;
  writer << xiiConversionUtilsVulkan::GetUnderlyingValue(a.m_descriptorType);
  writer << xiiConversionUtilsVulkan::GetUnderlyingValue(a.m_xiiType);
  writer << a.m_bDepth;
  return writer.GetHashValue();
}

bool xiiFallbackResourcesVulkan::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_descriptorType == b.m_descriptorType && a.m_xiiType == b.m_xiiType && a.m_bDepth == b.m_bDepth;
}
