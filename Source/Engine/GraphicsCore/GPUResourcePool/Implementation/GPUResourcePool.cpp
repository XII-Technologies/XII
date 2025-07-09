#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
#  include <Foundation/Utilities/Stats.h>
#endif

xiiGPUResourcePool* xiiGPUResourcePool::s_pDefaultInstance = nullptr;

xiiGPUResourcePool::xiiGPUResourcePool() :
  m_pDevice(xiiGALDevice::GetDefaultDevice()), m_GALDeviceEventSubscriptionID(xiiGALDevice::s_Events.AddEventHandler(xiiMakeDelegate(&xiiGPUResourcePool::GALDeviceEventHandler, this)))
{
}

xiiGPUResourcePool::~xiiGPUResourcePool()
{
  xiiGALDevice::s_Events.RemoveEventHandler(m_GALDeviceEventSubscriptionID);
  if (!m_TexturesInUse.IsEmpty())
  {
    xiiLog::SeriousWarning("Destructing a GPU resource pool of which textures are still in use!");
  }

  // Free remaining resources.
  ReleaseStaleResources(0);
}

xiiSharedPtr<xiiGALBuffer> xiiGPUResourcePool::GetBuffer(const xiiGALBufferCreationDescription& description)
{
  XII_LOCK(m_Lock);

  const xiiUInt32 uiBufferDescriptorHash = description.CalculateHash();

  // Check if there is a fitting buffer available.
  auto it = m_AvailableBuffers.Find(uiBufferDescriptorHash);

  if (it.IsValid())
  {
    xiiDynamicArray<BufferHandleWithAge>& buffers = it.Value();

    if (!buffers.IsEmpty())
    {
      xiiSharedPtr<xiiGALBuffer> pBuffer = buffers.PeekBack().m_pBuffer;
      buffers.PopBack();

      XII_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer in resource pool!");

      m_BuffersInUse.Insert(pBuffer);

      return pBuffer;
    }
  }

  // Since we found no matching buffer we need to create a new one, but we check if we should run the garbage collector first since we need to allocate memory now.
  CheckAndPotentiallyReleaseStaleResources();

  xiiSharedPtr<xiiGALBuffer> pNewBuffer = m_pDevice->CreateBuffer(description);

  if (pNewBuffer == nullptr)
  {
    xiiLog::Error("GPU resource pool could not create new buffer for the given descriptor (size: {0}).", description.m_uiSize);
    return nullptr;
  }

  // Track the newly created buffer.
  m_BuffersInUse.Insert(pNewBuffer);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += 0U;

  UpdateMemoryStats();

  return pNewBuffer;
}

void xiiGPUResourcePool::ReturnBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer)
{
  XII_LOCK(m_Lock);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  // Ensure this buffer was issued by the pool.
  if (!m_BuffersInUse.Contains(pBuffer))
  {
    xiiLog::Error("Returning a buffer to the GPU resource pool that was not issued by the pool is not valid!");
    return;
  }
#endif

  m_BuffersInUse.Remove(pBuffer);

  if (pBuffer != nullptr)
  {
    const xiiUInt32 uiBufferDescriptorHash = pBuffer->GetDescription().CalculateHash();

    auto it = m_AvailableBuffers.Find(uiBufferDescriptorHash);

    if (!it.IsValid())
    {
      it = m_AvailableBuffers.Insert(uiBufferDescriptorHash, xiiDynamicArray<BufferHandleWithAge>());
    }

    it.Value().PushBack({pBuffer, xiiRenderWorld::GetFrameCounter()});
  }
}

xiiSharedPtr<xiiGALTexture> xiiGPUResourcePool::GetRenderTarget(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALMSAASampleCount> sampleCount, xiiUInt32 uiSliceColunt, bool bIsArray)
{
  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Format             = format;
  textureDescription.m_Size.width         = uiWidth;
  textureDescription.m_Size.height        = uiHeight;
  textureDescription.m_uiSampleCount      = sampleCount;
  textureDescription.m_uiArraySizeOrDepth = uiSliceColunt;
  textureDescription.m_Type               = bIsArray ? xiiGALResourceDimension::Texture2DArray : xiiGALResourceDimension::Texture2D;
  textureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource;
  textureDescription.m_Usage              = xiiGALResourceUsage::Immutable;

  if (xiiGALResourceFormat::IsDepthFormat(format))
    textureDescription.m_BindFlags.Add(xiiGALBindFlags::DepthStencil);
  else
    textureDescription.m_BindFlags.Add(xiiGALBindFlags::RenderTarget);

  if (textureDescription.m_BindFlags.IsAnySet(xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil))
    textureDescription.m_Usage = xiiGALResourceUsage::Default;

  return GetTexture(textureDescription);
}

xiiSharedPtr<xiiGALTexture> xiiGPUResourcePool::GetTexture(const xiiGALTextureCreationDescription& description)
{
  XII_LOCK(m_Lock);

  const xiiUInt32 uiTextureDescriptorHash = description.CalculateHash();

  // Check if there is a fitting texture available.
  auto it = m_AvailableTextures.Find(uiTextureDescriptorHash);

  if (it.IsValid())
  {
    xiiDynamicArray<TextureHandleWithAge>& textures = it.Value();

    if (!textures.IsEmpty())
    {
      xiiSharedPtr<xiiGALTexture> pTexture = textures.PeekBack().m_pTexture;
      textures.PopBack();

      XII_ASSERT_DEV(pTexture != nullptr, "Invalid texture in resource pool!");

      m_TexturesInUse.Insert(pTexture);

      return pTexture;
    }
  }

  // Since we found no matching texture we need to create a new one, but we check if we should run the garbage collector first since we need to allocate memory now.
  CheckAndPotentiallyReleaseStaleResources();

  xiiSharedPtr<xiiGALTexture> pNewTexture = m_pDevice->CreateTexture(description);

  if (pNewTexture == nullptr)
  {
    xiiLog::Error("GPU resource pool could not create new texture for the given descriptor.");
    return nullptr;
  }

  // Track the newly created texture.
  m_TexturesInUse.Insert(pNewTexture);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += 0U;

  UpdateMemoryStats();

  return pNewTexture;
}

void xiiGPUResourcePool::ReturnTexture(xiiSharedPtr<xiiGALTexture> pTexture)
{
  XII_LOCK(m_Lock);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  // Ensure this texture was issued by the pool.
  if (!m_TexturesInUse.Contains(pTexture))
  {
    xiiLog::Error("Returning a texture to the GPU resource pool that was not issued by the pool is not valid!");
    return;
  }
#endif

  m_TexturesInUse.Remove(pTexture);

  if (pTexture != nullptr)
  {
    const xiiUInt32 uiTextureDescriptorHash = pTexture->GetDescription().CalculateHash();

    auto it = m_AvailableTextures.Find(uiTextureDescriptorHash);

    if (!it.IsValid())
    {
      it = m_AvailableTextures.Insert(uiTextureDescriptorHash, xiiDynamicArray<TextureHandleWithAge>());
    }

    it.Value().PushBack({pTexture, xiiRenderWorld::GetFrameCounter()});
  }
}

xiiSharedPtr<xiiGALSampler> xiiGPUResourcePool::GetSampler(const xiiGALSamplerCreationDescription& description)
{
  XII_LOCK(m_Lock);

  const xiiUInt32 uiSamplerDescriptorHash = description.CalculateHash();

  // Check if there is a fitting sampler available.
  auto it = m_AvailableSamplers.Find(uiSamplerDescriptorHash);

  if (it.IsValid())
  {
    xiiDynamicArray<SamplerHandleWithAge>& samplers = it.Value();

    if (!samplers.IsEmpty())
    {
      xiiSharedPtr<xiiGALSampler> pSampler = samplers.PeekBack().m_pSampler;
      samplers.PopBack();

      XII_ASSERT_DEV(pSampler != nullptr, "Invalid sampler in resource pool!");

      m_SamplersInUse.Insert(pSampler);

      return pSampler;
    }
  }

  // Since we found no matching sampler we need to create a new one, but we check if we should run the garbage collector first since we need to allocate memory now.
  CheckAndPotentiallyReleaseStaleResources();

  xiiSharedPtr<xiiGALSampler> pNewSampler = m_pDevice->CreateSampler(description);

  if (pNewSampler == nullptr)
  {
    xiiLog::Error("GPU resource pool could not create new sampler for the given descriptor.");
    return nullptr;
  }

  // Track the newly created sampler.
  m_SamplersInUse.Insert(pNewSampler);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += 0U;

  UpdateMemoryStats();

  return pNewSampler;
}

void xiiGPUResourcePool::ReturnSampler(xiiSharedPtr<xiiGALSampler> pSampler)
{
  XII_LOCK(m_Lock);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  // Ensure this sampler was issued by the pool.
  if (!m_SamplersInUse.Contains(pSampler))
  {
    xiiLog::Error("Returning a sampler to the GPU resource pool that was not issued by the pool is not valid!");
    return;
  }
#endif

  m_SamplersInUse.Remove(pSampler);

  if (pSampler != nullptr)
  {
    const xiiUInt32 uiSamplerDescriptorHash = pSampler->GetDescription().CalculateHash();

    auto it = m_AvailableSamplers.Find(uiSamplerDescriptorHash);

    if (!it.IsValid())
    {
      it = m_AvailableSamplers.Insert(uiSamplerDescriptorHash, xiiDynamicArray<SamplerHandleWithAge>());
    }

    it.Value().PushBack({pSampler, xiiRenderWorld::GetFrameCounter()});
  }
}

void xiiGPUResourcePool::ReleaseStaleResources(xiiUInt32 uiMinimumAge)
{
  XII_LOCK(m_Lock);

  XII_PROFILE_SCOPE("ReleaseStaleResources");
  xiiUInt64 uiCurrentFrame = xiiRenderWorld::GetFrameCounter();
  // Destroy all available textures older than uiMinimumAge frames
  {
    for (auto it = m_AvailableTextures.GetIterator(); it.IsValid();)
    {
      auto& textures = it.Value();
      for (xiiInt32 i = (xiiInt32)textures.GetCount() - 1; i >= 0; i--)
      {
        TextureHandleWithAge& texture = textures[i];
        if (texture.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (texture.m_pTexture != nullptr)
          {
            m_uiCurrentlyAllocatedMemory -= texture.m_pTexture->GetMemoryConsumption();
          }
          texture.m_pTexture.Clear();

          textures.RemoveAtAndCopy(i);
        }
        else
        {
          // The available textures are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (textures.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableTextures.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  // Destroy all available buffers older than uiMinimumAge frames
  {
    for (auto it = m_AvailableBuffers.GetIterator(); it.IsValid();)
    {
      auto& buffers = it.Value();
      for (xiiInt32 i = (xiiInt32)buffers.GetCount() - 1; i >= 0; i--)
      {
        BufferHandleWithAge& buffer = buffers[i];
        if (buffer.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (buffer.m_pBuffer != nullptr)
          {
            m_uiCurrentlyAllocatedMemory -= buffer.m_pBuffer->GetMemoryConsumption();
          }
          buffer.m_pBuffer.Clear();

          buffers.RemoveAtAndCopy(i);
        }
        else
        {
          // The available buffers are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (buffers.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableBuffers.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  m_uiNumAllocationsSinceLastGC = 0;

  UpdateMemoryStats();
}

xiiGPUResourcePool* xiiGPUResourcePool::GetDefaultInstance()
{
  return s_pDefaultInstance;
}

void xiiGPUResourcePool::SetDefaultInstance(xiiGPUResourcePool* pDefaultInstance)
{
  XII_DEFAULT_DELETE(s_pDefaultInstance);
  s_pDefaultInstance = pDefaultInstance;
}

void xiiGPUResourcePool::CheckAndPotentiallyReleaseStaleResources()
{
  if ((m_uiNumAllocationsSinceLastGC >= m_uiNumAllocationsThresholdForGC) || (m_uiCurrentlyAllocatedMemory >= m_uiMemoryThresholdForGC))
  {
    // Only try to collect resources unused for 3 or more frames. Using a smaller number will result in constant memory thrashing.
    ReleaseStaleResources(3);
  }
}

void xiiGPUResourcePool::UpdateMemoryStats() const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  float fMegaBytes = float(m_uiCurrentlyAllocatedMemory) / (1024.0f * 1024.0f);
  xiiStats::SetStat("GPU Resource Pool/Memory Consumption (MB)", fMegaBytes);
#endif
}

void xiiGPUResourcePool::GALDeviceEventHandler(const xiiGALDeviceEvent& e)
{
  if (e.m_Type == xiiGALDeviceEventType::AfterEndFrame)
  {
    ++m_uiFramesSinceLastGC;
    if (m_uiFramesSinceLastGC >= m_uiFramesThresholdSinceLastGC)
    {
      m_uiFramesSinceLastGC = 0;

      ReleaseStaleResources(10);
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_GPUResourcePool_Implementation_GPUResourcePool);
