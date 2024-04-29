#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
#  include <Foundation/Utilities/Stats.h>
#endif

xiiGPUResourcePool* xiiGPUResourcePool::s_pDefaultInstance = nullptr;

xiiGPUResourcePool::xiiGPUResourcePool()
{
  m_pDevice = xiiGALDevice::GetDefaultDevice();

  m_GALDeviceEventSubscriptionID = m_pDevice->m_Events.AddEventHandler(xiiMakeDelegate(&xiiGPUResourcePool::GALDeviceEventHandler, this));
}

xiiGPUResourcePool::~xiiGPUResourcePool()
{
  m_pDevice->m_Events.RemoveEventHandler(m_GALDeviceEventSubscriptionID);
  if (!m_TexturesInUse.IsEmpty())
  {
    xiiLog::SeriousWarning("Destructing a GPU resource pool of which textures are still in use!");
  }

  // Free remaining resources
  RunGC(0);
}

xiiGALTextureHandle xiiGPUResourcePool::GetRenderTarget(const xiiGALTextureCreationDescription& textureDesc)
{
  XII_LOCK(m_Lock);

  if (!textureDesc.m_BindFlags.IsAnySet(xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil))
  {
    xiiLog::Error("Texture description for render target or depth stencil needs to be created with xiiGALBindFlags::RenderTarget or xiiGALBindFlags::DepthStencil!");
    return xiiGALTextureHandle();
  }

  const xiiUInt32 uiTextureDescHash = textureDesc.CalculateHash();

  // Check if there is a fitting texture available
  auto it = m_AvailableTextures.Find(uiTextureDescHash);
  if (it.IsValid())
  {
    xiiDynamicArray<TextureHandleWithAge>& textures = it.Value();
    if (!textures.IsEmpty())
    {
      xiiGALTextureHandle hTexture = textures.PeekBack().m_hTexture;
      textures.PopBack();

      XII_ASSERT_DEV(m_pDevice->GetTexture(hTexture) != nullptr, "Invalid texture in resource pool");

      m_TexturesInUse.Insert(hTexture);

      return hTexture;
    }
  }

  // Since we found no matching texture we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  xiiGALTextureHandle hNewTexture = m_pDevice->CreateTexture(textureDesc);

  if (hNewTexture.IsInvalidated())
  {
    xiiLog::Error("GPU resource pool couldn't create new texture for given desc (size: {0} x {1}, format: {2})", textureDesc.m_Size.width, textureDesc.m_Size.height, textureDesc.m_Format);
    return xiiGALTextureHandle();
  }

  // Also track the new created texture
  m_TexturesInUse.Insert(hNewTexture);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(textureDesc);

  UpdateMemoryStats();

  return hNewTexture;
}

xiiGALTextureHandle xiiGPUResourcePool::GetRenderTarget(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiEnum<xiiGALTextureFormat> format, xiiEnum<xiiGALMSAASampleCount> sampleCount, xiiUInt32 uiSliceColunt, bool bIsArray)
{
  xiiGALTextureCreationDescription TextureDesc;
  TextureDesc.m_Format             = format;
  TextureDesc.m_Size.width         = uiWidth;
  TextureDesc.m_Size.height        = uiHeight;
  TextureDesc.m_uiSampleCount      = sampleCount;
  TextureDesc.m_uiArraySizeOrDepth = uiSliceColunt;
  TextureDesc.m_Type               = (bIsArray || TextureDesc.m_uiSampleCount > 1) ? xiiGALResourceDimension::Texture2DArray : xiiGALResourceDimension::Texture2D;
  TextureDesc.m_BindFlags          = xiiGALBindFlags::ShaderResource;
  TextureDesc.m_Usage              = xiiGALResourceUsage::Immutable;

  if (xiiGALTextureFormat::IsDepthFormat(format))
    TextureDesc.m_BindFlags.Add(xiiGALBindFlags::DepthStencil);
  else
    TextureDesc.m_BindFlags.Add(xiiGALBindFlags::RenderTarget);

  if (TextureDesc.m_BindFlags.IsAnySet(xiiGALBindFlags::RenderTarget | xiiGALBindFlags::DepthStencil))
    TextureDesc.m_Usage = xiiGALResourceUsage::Default;

  return GetRenderTarget(TextureDesc);
}

void xiiGPUResourcePool::ReturnRenderTarget(xiiGALTextureHandle hRenderTarget)
{
  XII_LOCK(m_Lock);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_TexturesInUse.Contains(hRenderTarget))
  {
    xiiLog::Error("Returning a texture to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_TexturesInUse.Remove(hRenderTarget);

  if (const xiiGALTexture* pTexture = m_pDevice->GetTexture(hRenderTarget))
  {
    const xiiUInt32 uiTextureDescHash = pTexture->GetDescription().CalculateHash();

    auto it = m_AvailableTextures.Find(uiTextureDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableTextures.Insert(uiTextureDescHash, xiiDynamicArray<TextureHandleWithAge>());
    }

    it.Value().PushBack({hRenderTarget, xiiRenderWorld::GetFrameCounter()});
  }
}

xiiGALBufferHandle xiiGPUResourcePool::GetBuffer(const xiiGALBufferCreationDescription& bufferDesc)
{
  XII_LOCK(m_Lock);

  const xiiUInt32 uiBufferDescHash = bufferDesc.CalculateHash();

  // Check if there is a fitting buffer available
  auto it = m_AvailableBuffers.Find(uiBufferDescHash);
  if (it.IsValid())
  {
    xiiDynamicArray<BufferHandleWithAge>& buffers = it.Value();
    if (!buffers.IsEmpty())
    {
      xiiGALBufferHandle hBuffer = buffers.PeekBack().m_hBuffer;
      buffers.PopBack();

      XII_ASSERT_DEV(m_pDevice->GetBuffer(hBuffer) != nullptr, "Invalid buffer in resource pool");

      m_BuffersInUse.Insert(hBuffer);

      return hBuffer;
    }
  }

  // Since we found no matching buffer we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  xiiGALBufferHandle hNewBuffer = m_pDevice->CreateBuffer(bufferDesc);

  if (hNewBuffer.IsInvalidated())
  {
    xiiLog::Error("GPU resource pool couldn't create new buffer for given desc (size: {0})", bufferDesc.m_uiSize);
    return xiiGALBufferHandle();
  }

  // Also track the new created buffer
  m_BuffersInUse.Insert(hNewBuffer);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForBuffer(bufferDesc);

  UpdateMemoryStats();

  return hNewBuffer;
}

void xiiGPUResourcePool::ReturnBuffer(xiiGALBufferHandle hBuffer)
{
  XII_LOCK(m_Lock);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_BuffersInUse.Contains(hBuffer))
  {
    xiiLog::Error("Returning a buffer to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_BuffersInUse.Remove(hBuffer);

  if (const xiiGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer))
  {
    const xiiUInt32 uiBufferDescHash = pBuffer->GetDescription().CalculateHash();

    auto it = m_AvailableBuffers.Find(uiBufferDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableBuffers.Insert(uiBufferDescHash, xiiDynamicArray<BufferHandleWithAge>());
    }

    it.Value().PushBack({hBuffer, xiiRenderWorld::GetFrameCounter()});
  }
}

void xiiGPUResourcePool::RunGC(xiiUInt32 uiMinimumAge)
{
  XII_LOCK(m_Lock);

  XII_PROFILE_SCOPE("RunGC");
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
          if (const xiiGALTexture* pTexture = m_pDevice->GetTexture(texture.m_hTexture))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTexture->GetDescription());
          }

          m_pDevice->DestroyTexture(texture.m_hTexture);
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
          if (const xiiGALBuffer* pBuffer = m_pDevice->GetBuffer(buffer.m_hBuffer))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForBuffer(pBuffer->GetDescription());
          }

          m_pDevice->DestroyBuffer(buffer.m_hBuffer);
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


void xiiGPUResourcePool::CheckAndPotentiallyRunGC()
{
  if ((m_uiNumAllocationsSinceLastGC >= m_uiNumAllocationsThresholdForGC) || (m_uiCurrentlyAllocatedMemory >= m_uiMemoryThresholdForGC))
  {
    // Only try to collect resources unused for 3 or more frames. Using a smaller number will result in constant memory thrashing.
    RunGC(3);
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
      RunGC(10);
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_GPUResourcePool_Implementation_GPUResourcePool);
