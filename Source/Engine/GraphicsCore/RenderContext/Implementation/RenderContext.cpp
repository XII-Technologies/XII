#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/Blob.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

xiiRenderContext::xiiRenderContext(xiiSharedPtr<xiiGALCommandList> pCommandList) :
  m_pCommandList(pCommandList)
{
  XII_ASSERT_DEV(m_pCommandList != nullptr, "An invalid command list is given. A render context requires a valid command list reference.");

  m_pGlobalConstants = xiiMakeBlobPtr(reinterpret_cast<xiiGlobalConstants*>(xiiFoundation::GetAlignedAllocator()->Allocate(sizeof(xiiGlobalConstants), 16U)), 1U);

  xiiMemoryUtils::ZeroFill(m_pGlobalConstants.GetPtr(), 1U);
}

xiiRenderContext::~xiiRenderContext()
{
  xiiFoundation::GetAlignedAllocator()->Deallocate(m_pGlobalConstants.GetPtr());

  m_pGlobalConstants.Clear();
}

void xiiRenderContext::BeginRendering(const xiiRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName, bool bStereoRendering)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::None, "Already in a scope.");

  m_RenderContextScope = RenderContextScope::Graphics;
  m_bStereoRendering   = bStereoRendering;

  const xiiGALRenderPassCreationDescription& renderPassDescription = renderingSetup.GetRenderPassDescription();

  for (const auto& attachment : renderPassDescription.m_Attachments)
  {
    if (attachment.m_LoadOperation == xiiGALAttachmentLoadOperation::Clear)
    {
      m_bNeedsClear = true;
      break;
    }
  }

  xiiUInt8 uiSampleCount = xiiGALMSAASampleCount::OneSample;

  if (!renderPassDescription.m_Attachments.IsEmpty())
  {
    uiSampleCount = renderPassDescription.m_Attachments.PeekBack().m_uiSampleCount;
  }

  if (uiSampleCount > 1)
  {
    SetShaderPermutationVariable("MSAA", "TRUE");
  }
  else
  {
    SetShaderPermutationVariable("MSAA", "FALSE");
  }

  {
    xiiGlobalConstants* pGlobalConstants = GetGlobalConstants();
    pGlobalConstants->ViewportSize       = xiiVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
    pGlobalConstants->NumMsaaSamples     = uiSampleCount;
  }

  {
    m_bHasDebugGroup = !sName.IsEmpty();

    if (m_bHasDebugGroup)
    {
      m_pCommandList->BeginDebugGroup(sName);
    }
  }

  m_pCommandList->SetViewport({viewport.x, viewport.y, viewport.width, viewport.height, 0.0f, 0.1f});
}

void xiiRenderContext::EndRendering()
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "BeginRendering() has not been called.");

  if (m_bNeedsClear)
  {
    BeginInternalRenderPass();

    m_bNeedsClear = false;
  }

  EndInternalRenderPass();

  if (m_bHasDebugGroup)
  {
    m_pCommandList->EndDebugGroup();

    m_bHasDebugGroup = false;
  }

  m_bStereoRendering   = false;
  m_RenderContextScope = RenderContextScope::None;
}

void xiiRenderContext::BeginCompute(xiiStringView sName)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::None, "Already in a scope.");

  m_RenderContextScope = RenderContextScope::Compute;

  {
    m_bHasDebugGroup = !sName.IsEmpty();

    if (m_bHasDebugGroup)
    {
      m_pCommandList->BeginDebugGroup(sName);
    }
  }
}

void xiiRenderContext::EndCompute()
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "BeginCompute() has not been called.");

  if (m_bHasDebugGroup)
  {
    m_pCommandList->EndDebugGroup();

    m_bHasDebugGroup = false;
  }

  m_RenderContextScope = RenderContextScope::None;
}

void xiiRenderContext::SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sValue)
{
  xiiTempHashedString sHashedName(sName);

  xiiHashedString sNameHash, sValueHash;
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sHashedName, sValue, sNameHash, sValueHash))
  {
    SetShaderPermutationVariableInternal(sNameHash, sValueHash);
  }
}

void xiiRenderContext::SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void xiiRenderContext::BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pConstantBuffer)
{
  xiiSharedPtr<xiiGALBuffer>* pOldConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pOldConstantBuffer))
  {
    if (*pOldConstantBuffer == pConstantBuffer)
      return;

    *pOldConstantBuffer = pConstantBuffer;
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), pConstantBuffer);
  }

  m_StateFlags.Add(xiiRenderContextFlags::ConstantBufferBindingChanged);
}

void xiiRenderContext::BindBufferView(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  xiiSharedPtr<xiiGALBufferView>* pOldBufferView = nullptr;
  if (m_BoundBufferSRVs.TryGetValue(sSlotName.GetHash(), pOldBufferView))
  {
    if (*pOldBufferView == pBufferView)
      return;

    *pOldBufferView = pBufferView;
  }
  else
  {
    m_BoundBufferSRVs.Insert(sSlotName.GetHash(), pBufferView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::BufferBindingChanged);
}

void xiiRenderContext::BindTextureView(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  xiiSharedPtr<xiiGALTextureView>* pOldTextureView = nullptr;
  if (m_BoundTextureSRVs.TryGetValue(sSlotName.GetHash(), pOldTextureView))
  {
    if (*pOldTextureView == pTextureView)
      return;

    *pOldTextureView = pTextureView;
  }
  else
  {
    m_BoundTextureSRVs.Insert(sSlotName.GetHash(), pTextureView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged);
}

void xiiRenderContext::BindSampler(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALSampler> pSampler)
{
  xiiSharedPtr<xiiGALSampler>* pOldSampler = nullptr;
  if (m_BoundSamplers.TryGetValue(sSlotName.GetHash(), pOldSampler))
  {
    if (*pOldSampler == pSampler)
      return;

    *pOldSampler = pSampler;
  }
  else
  {
    m_BoundSamplers.Insert(sSlotName.GetHash(), pSampler);
  }

  m_StateFlags.Add(xiiRenderContextFlags::SamplerBindingChanged);
}

void xiiRenderContext::BindBufferViewUAV(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView)
{
  xiiSharedPtr<xiiGALBufferView>* pOldBufferView = nullptr;
  if (m_BoundBufferUAVs.TryGetValue(sSlotName.GetHash(), pOldBufferView))
  {
    if (*pOldBufferView == pBufferView)
      return;

    *pOldBufferView = pBufferView;
  }
  else
  {
    m_BoundBufferUAVs.Insert(sSlotName.GetHash(), pBufferView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::BufferUAVBindingChanged);
}

void xiiRenderContext::BindTextureViewUAV(const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView)
{
  xiiSharedPtr<xiiGALTextureView>* pOldTextureView = nullptr;
  if (m_BoundTextureUAVs.TryGetValue(sSlotName.GetHash(), pOldTextureView))
  {
    if (*pOldTextureView == pTextureView)
      return;

    *pOldTextureView = pTextureView;
  }
  else
  {
    m_BoundTextureUAVs.Insert(sSlotName.GetHash(), pTextureView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureUAVBindingChanged);
}

void xiiRenderContext::BindTexture2D(const xiiTempHashedString& sSlotName, const xiiTexture2DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
  if (hTexture.IsValid())
  {
    xiiResourceLock<xiiTexture2DResource> pTexture(hTexture, acquireMode);

    BindTexture(sSlotName, pTexture->GetGALTexture());
    BindSampler(sSlotName, pTexture->GetGALSampler());
  }
  else
  {
    BindTexture(sSlotName, nullptr);
  }
}

void xiiRenderContext::BindTexture3D(const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
  if (hTexture.IsValid())
  {
    xiiResourceLock<xiiTexture3DResource> pTexture(hTexture, acquireMode);

    BindTexture(sSlotName, pTexture->GetGALTexture());
    BindSampler(sSlotName, pTexture->GetGALSampler());
  }
  else
  {
    BindTexture(sSlotName, nullptr);
  }
}

void xiiRenderContext::BindTextureCube(const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode)
{
  if (hTexture.IsValid())
  {
    xiiResourceLock<xiiTextureCubeResource> pTexture(hTexture, acquireMode);

    BindTexture(sSlotName, pTexture->GetGALTexture());
    BindSampler(sSlotName, pTexture->GetGALSampler());
  }
  else
  {
    BindTexture(sSlotName, nullptr);
  }
}

void xiiRenderContext::BindMaterial(const xiiMaterialResourceHandle& hMaterial)
{
}

void xiiRenderContext::BindShader(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags)
{
}

void xiiRenderContext::BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer)
{
}

void xiiRenderContext::BindMeshBuffer(const xiiMeshBufferResourceHandle& hMeshBuffer)
{
}

void xiiRenderContext::BindMeshBuffer(xiiSharedPtr<xiiGALBuffer> pVertexBuffer, xiiSharedPtr<xiiGALBuffer> pIndexBuffer, const xiiInputLayoutInfo* pInputLayoutInfo, xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount, xiiSharedPtr<xiiGALBuffer> pVertexBuffer2, xiiSharedPtr<xiiGALBuffer> pVertexBuffer3, xiiSharedPtr<xiiGALBuffer> pVertexBuffer4)
{
}

xiiResult xiiRenderContext::DrawMeshBuffer(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Succeeded() || uiPrimitiveCount == 0U || uiInstanceCount == 0U)
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiResult xiiRenderContext::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  if (ApplyContextStates().Succeeded())
  {
    return m_pCommandList->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
  }
  return XII_FAILURE;
}

xiiResult xiiRenderContext::ApplyContextStates(bool bForce)
{
  return XII_SUCCESS;
}

void xiiRenderContext::ResetContextState()
{
}

void xiiRenderContext::SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  xiiHashedString* pOldValue = nullptr;
  m_PermutationVariables.TryGetValue(sName, pOldValue);

  if (pOldValue == nullptr || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);

    m_StateFlags.Add(xiiRenderContextFlags::ShaderStateChanged);
  }
}

xiiSharedPtr<xiiGALRenderPass> xiiRenderContext::CreateInternalRenderPass(const xiiGALRenderPassCreationDescription& description)
{
  RenderPassCache* pRenderPassCache;
  if (m_RenderPassCache.TryGetValue(description, pRenderPassCache))
  {
    return pRenderPassCache->m_pRenderPass;
  }

  xiiSharedPtr<xiiGALDevice>     pDevice     = xiiGALDevice::GetDefaultDevice();
  xiiSharedPtr<xiiGALRenderPass> pRenderPass = pDevice->CreateRenderPass(description);

  XII_ASSERT_DEV(pRenderPass != nullptr, "Failed to create render pass.");

  m_RenderPassCache.Insert(description, RenderPassCache{pRenderPass});

  return pRenderPass;
}

xiiSharedPtr<xiiGALFramebuffer> xiiRenderContext::GetCurrentFramebuffer()
{
  XII_ASSERT_DEV(m_pActiveRenderPass != nullptr, "GetCurrentFramebuffer() may only be called once an active render pass has been created.");

  RenderPassCache* pRenderPassCache;
  if (m_RenderPassCache.TryGetValue(m_pActiveRenderPass->GetDescription(), pRenderPassCache))
  {
    for (xiiUInt32 i = 0; i < pRenderPassCache->m_FramebufferCache.GetCount(); ++i)
    {
      const auto& pFrameBuffer = pRenderPassCache->m_FramebufferCache[i];
      const auto& description  = pFrameBuffer->GetDescription();

      XII_ASSERT_DEBUG(description.m_pRenderPass == pRenderPassCache->m_pRenderPass, "Render pass mismatch for the same render pass description.");

      if (description.m_Attachments == m_RenderingSetup.GetFramebufferDescription().m_Attachments && description.m_FramebufferSize == m_RenderingSetup.GetFramebufferDescription().m_FramebufferSize && description.m_uiArraySliceCount == m_RenderingSetup.GetFramebufferDescription().m_uiArraySliceCount)
      {
        return pFrameBuffer;
      }
    }

    xiiGALFramebufferCreationDescription framebufferDescription = m_RenderingSetup.GetFramebufferDescription();
    framebufferDescription.m_pRenderPass                        = pRenderPassCache->m_pRenderPass;

    xiiSharedPtr<xiiGALDevice>      pDevice      = xiiGALDevice::GetDefaultDevice();
    xiiSharedPtr<xiiGALFramebuffer> pFramebuffer = pDevice->CreateFramebuffer(framebufferDescription);

    pRenderPassCache->m_FramebufferCache.PushBack(pFramebuffer);

    return pFramebuffer;
  }
  return nullptr;
}

void xiiRenderContext::BeginInternalRenderPass()
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "Render pass can only be begun in a graphics scope.");

  if (!m_pActiveRenderPass && !m_RenderingSetup.GetRenderPassDescription().m_Attachments.IsEmpty())
  {
    if (m_bNeedsClear)
    {
      m_pActiveRenderPass = CreateInternalRenderPass(m_RenderingSetup.GetRenderPassDescription());

      m_pCommandList->BeginRenderPass({m_pActiveRenderPass, GetCurrentFramebuffer(), m_ClearValues});

      m_bNeedsClear = false;
    }
    else
    {
      xiiGALRenderPassCreationDescription renderPassDescription = m_RenderingSetup.GetRenderPassDescription();

      for (auto& attachment : renderPassDescription.m_Attachments)
      {
        if (attachment.m_LoadOperation == xiiGALAttachmentLoadOperation::Clear)
        {
          attachment.m_LoadOperation = xiiGALAttachmentLoadOperation::Load;
        }
      }

      m_pActiveRenderPass = CreateInternalRenderPass(renderPassDescription);

      m_pCommandList->BeginRenderPass({m_pActiveRenderPass, GetCurrentFramebuffer()});
    }
  }
}

void xiiRenderContext::EndInternalRenderPass()
{
  if (m_pActiveRenderPass)
  {
    m_pCommandList->EndRenderPass();

    m_pActiveRenderPass = nullptr;
  }
}

// static
xiiGALSamplerCreationDescription xiiRenderContext::GetDefaultSamplerDescription(xiiBitflags<xiiDefaultSamplerFlags> flags)
{
  xiiGALSamplerCreationDescription samplerDescription;
  samplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Never;
  samplerDescription.m_BorderColor        = xiiColor::Black;
  samplerDescription.m_fMipLODBias        = 0.0f;
  samplerDescription.m_fMinLOD            = -1.0f;
  samplerDescription.m_fMaxLOD            = 42000.0f;
  samplerDescription.m_uiMaxAnisotropy    = 4U;

  samplerDescription.m_MinFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;
  samplerDescription.m_MagFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;
  samplerDescription.m_MipFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;

  samplerDescription.m_AddressU = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);
  samplerDescription.m_AddressV = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);
  samplerDescription.m_AddressW = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);

  return samplerDescription;
}
