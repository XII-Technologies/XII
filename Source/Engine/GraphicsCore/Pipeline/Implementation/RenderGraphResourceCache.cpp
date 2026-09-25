/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

xiiRenderGraphResourceCache::xiiRenderGraphResourceCache() = default;

xiiRenderGraphResourceCache::~xiiRenderGraphResourceCache()
{
  Shutdown();
}

void xiiRenderGraphResourceCache::Initialize(xiiSharedPtr<xiiGALDevice> pDevice)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null.");

  m_pDevice = pDevice;
}

void xiiRenderGraphResourceCache::Shutdown()
{
  XII_LOCK(m_Mutex);

  m_ActiveTextures.Clear();
  m_ActiveBuffers.Clear();
  m_RetiredTextures.Clear();
  m_RetiredBuffers.Clear();
  m_TexturePool.Clear();
  m_BufferPool.Clear();
  m_pDevice = nullptr;
}

void xiiRenderGraphResourceCache::BeginFrame(xiiUInt64 uiFrameIndex)
{
  const xiiUInt64 uiConservativeCompletedFrame = uiFrameIndex > 3ULL ? uiFrameIndex - 3ULL : 0ULL;
  BeginFrame(uiFrameIndex, uiConservativeCompletedFrame);
}

void xiiRenderGraphResourceCache::BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrameIndex)
{
  XII_LOCK(m_Mutex);

  m_uiCurrentFrame   = uiFrameIndex;
  m_uiCompletedFrame = xiiMath::Min(uiCompletedFrameIndex, uiFrameIndex);

  for (xiiUInt32 i = m_RetiredTextures.GetCount(); i > 0U; --i)
  {
    PooledTexture& retired = m_RetiredTextures[i - 1U];
    if (retired.m_uiLastUsedFrame > m_uiCompletedFrame)
      continue;

    const xiiUInt32                 uiHash = retired.m_pTexture->GetDescription().CalculateHash();
    xiiDynamicArray<PooledTexture>* pPool = m_TexturePool.GetValue(uiHash);
    if (pPool == nullptr)
    {
      m_TexturePool.Insert(uiHash, xiiDynamicArray<PooledTexture>());
      pPool = m_TexturePool.GetValue(uiHash);
    }
    pPool->PushBack(retired);
    m_RetiredTextures.RemoveAtAndSwap(i - 1U);
  }

  for (xiiUInt32 i = m_RetiredBuffers.GetCount(); i > 0U; --i)
  {
    PooledBuffer& retired = m_RetiredBuffers[i - 1U];
    if (retired.m_uiLastUsedFrame > m_uiCompletedFrame)
      continue;

    const xiiUInt32                uiHash = retired.m_pBuffer->GetDescription().CalculateHash();
    xiiDynamicArray<PooledBuffer>* pPool = m_BufferPool.GetValue(uiHash);
    if (pPool == nullptr)
    {
      m_BufferPool.Insert(uiHash, xiiDynamicArray<PooledBuffer>());
      pPool = m_BufferPool.GetValue(uiHash);
    }
    pPool->PushBack(retired);
    m_RetiredBuffers.RemoveAtAndSwap(i - 1U);
  }
}

void xiiRenderGraphResourceCache::EndFrame()
{
  XII_LOCK(m_Mutex);

  for (const xiiSharedPtr<xiiGALTexture>& pTexture : m_ActiveTextures)
  {
    PooledTexture& entry    = m_RetiredTextures.ExpandAndGetRef();
    entry.m_pTexture        = pTexture;
    entry.m_uiLastUsedFrame = m_uiCurrentFrame;
  }
  m_ActiveTextures.Clear();

  for (const xiiSharedPtr<xiiGALBuffer>& pBuffer : m_ActiveBuffers)
  {
    PooledBuffer& entry     = m_RetiredBuffers.ExpandAndGetRef();
    entry.m_pBuffer         = pBuffer;
    entry.m_uiLastUsedFrame = m_uiCurrentFrame;
  }
  m_ActiveBuffers.Clear();
}

xiiSharedPtr<xiiGALTexture> xiiRenderGraphResourceCache::AcquireTexture(const xiiGALTextureCreationDescription& description)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "RenderGraphResourceCache has not been initialized.");
  XII_LOCK(m_Mutex);

  const xiiUInt32 uiHash = description.CalculateHash();

  xiiDynamicArray<PooledTexture>* pPool = m_TexturePool.GetValue(uiHash);
  if (pPool != nullptr && !pPool->IsEmpty())
  {
    PooledTexture pooled = pPool->PeekBack();
    pPool->PopBack();

    m_ActiveTextures.PushBack(pooled.m_pTexture);

    return pooled.m_pTexture;
  }

  xiiSharedPtr<xiiGALTexture> pTexture = m_pDevice->CreateTexture(description);
  XII_ASSERT_ALWAYS(pTexture != nullptr, "Failed to create transient texture.");
  m_ActiveTextures.PushBack(pTexture);

  return pTexture;
}

void xiiRenderGraphResourceCache::ReturnTexture(xiiSharedPtr<xiiGALTexture> pTexture)
{
  XII_ASSERT_DEV(pTexture != nullptr, "Cannot return a null texture to the resource cache.");
  XII_LOCK(m_Mutex);

  const xiiUInt32 uiIndex = m_ActiveTextures.IndexOf(pTexture);
  XII_ASSERT_DEV(uiIndex != xiiInvalidIndex, "Returned texture was not acquired from this cache in the current frame.");
  m_ActiveTextures.RemoveAtAndSwap(uiIndex);

  PooledTexture& entry    = m_RetiredTextures.ExpandAndGetRef();
  entry.m_pTexture        = pTexture;
  entry.m_uiLastUsedFrame = m_uiCurrentFrame;
}

xiiSharedPtr<xiiGALBuffer> xiiRenderGraphResourceCache::AcquireBuffer(const xiiGALBufferCreationDescription& description)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "RenderGraphResourceCache has not been initialized.");
  XII_LOCK(m_Mutex);

  const xiiUInt32 uiHash = description.CalculateHash();

  xiiDynamicArray<PooledBuffer>* pPool = m_BufferPool.GetValue(uiHash);
  if (pPool != nullptr && !pPool->IsEmpty())
  {
    PooledBuffer pooled = pPool->PeekBack();
    pPool->PopBack();

    m_ActiveBuffers.PushBack(pooled.m_pBuffer);

    return pooled.m_pBuffer;
  }

  xiiSharedPtr<xiiGALBuffer> pBuffer = m_pDevice->CreateBuffer(description);
  XII_ASSERT_ALWAYS(pBuffer != nullptr, "Failed to create transient buffer.");
  m_ActiveBuffers.PushBack(pBuffer);

  return pBuffer;
}

void xiiRenderGraphResourceCache::ReturnBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer)
{
  XII_ASSERT_DEV(pBuffer != nullptr, "Cannot return a null buffer to the resource cache.");
  XII_LOCK(m_Mutex);

  const xiiUInt32 uiIndex = m_ActiveBuffers.IndexOf(pBuffer);
  XII_ASSERT_DEV(uiIndex != xiiInvalidIndex, "Returned buffer was not acquired from this cache in the current frame.");
  m_ActiveBuffers.RemoveAtAndSwap(uiIndex);

  PooledBuffer& entry     = m_RetiredBuffers.ExpandAndGetRef();
  entry.m_pBuffer         = pBuffer;
  entry.m_uiLastUsedFrame = m_uiCurrentFrame;
}

void xiiRenderGraphResourceCache::ReleaseStaleResources(xiiUInt32 uiMinAgeFrames)
{
  XII_LOCK(m_Mutex);

  if (m_uiCurrentFrame < static_cast<xiiUInt64>(uiMinAgeFrames))
    return;

  const xiiUInt64 uiStaleThreshold = m_uiCurrentFrame - static_cast<xiiUInt64>(uiMinAgeFrames);

  for (auto it = m_TexturePool.GetIterator(); it.IsValid(); ++it)
  {
    xiiDynamicArray<PooledTexture>& pool = it.Value();

    for (xiiUInt32 i = pool.GetCount(); i > 0U; --i)
    {
      if (pool[i - 1U].m_uiLastUsedFrame <= uiStaleThreshold)
      {
        pool.RemoveAtAndSwap(i - 1U);
      }
    }
  }

  for (auto it = m_BufferPool.GetIterator(); it.IsValid(); ++it)
  {
    xiiDynamicArray<PooledBuffer>& pool = it.Value();

    for (xiiUInt32 i = pool.GetCount(); i > 0U; --i)
    {
      if (pool[i - 1U].m_uiLastUsedFrame <= uiStaleThreshold)
      {
        pool.RemoveAtAndSwap(i - 1U);
      }
    }
  }
}

xiiUInt32 xiiRenderGraphResourceCache::GetActiveResourceCount() const
{
  XII_LOCK(m_Mutex);

  return m_ActiveTextures.GetCount() + m_ActiveBuffers.GetCount();
}

xiiUInt32 xiiRenderGraphResourceCache::GetPooledResourceCount() const
{
  XII_LOCK(m_Mutex);

  xiiUInt32 uiCount = 0U;
  for (auto it = m_TexturePool.GetIterator(); it.IsValid(); ++it)
  {
    uiCount += it.Value().GetCount();
  }
  for (auto it = m_BufferPool.GetIterator(); it.IsValid(); ++it)
  {
    uiCount += it.Value().GetCount();
  }
  return uiCount;
}
