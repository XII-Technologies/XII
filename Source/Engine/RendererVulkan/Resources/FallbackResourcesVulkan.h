#pragma once
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

class xiiGALDeviceVulkan;
class xiiGALResourceViewVulkan;
class xiiGALUnorderedAccessViewVulkan;

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a descriptor slot.
class xiiFallbackResourcesVulkan
{
public:
  static void Initialize(xiiGALDeviceVulkan* pDevice);
  static void DeInitialize();

  static const xiiGALResourceViewVulkan*        GetFallbackResourceView(vk::DescriptorType descriptorType, xiiShaderResourceType::Enum xiiType, bool bDepth);
  static const xiiGALUnorderedAccessViewVulkan* GetFallbackUnorderedAccessView(vk::DescriptorType descriptorType, xiiShaderResourceType::Enum xiiType);

private:
  static void GALDeviceEventHandler(const xiiGALDeviceEvent& e);

  static xiiGALDeviceVulkan*    s_pDevice;
  static xiiEventSubscriptionID s_EventID;

  struct Key
  {
    XII_DECLARE_POD_TYPE();
    vk::DescriptorType          m_descriptorType;
    xiiShaderResourceType::Enum m_xiiType;
    bool                        m_bDepth = false;
  };

  struct KeyHash
  {
    static xiiUInt32 Hash(const Key& a);
    static bool      Equal(const Key& a, const Key& b);
  };

  static xiiHashTable<Key, xiiGALResourceViewHandle, KeyHash>        m_ResourceViews;
  static xiiHashTable<Key, xiiGALUnorderedAccessViewHandle, KeyHash> m_UAVs;

  static xiiDynamicArray<xiiGALBufferHandle>  m_Buffers;
  static xiiDynamicArray<xiiGALTextureHandle> m_Textures;
};
