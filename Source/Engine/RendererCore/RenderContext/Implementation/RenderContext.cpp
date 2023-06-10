#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

xiiRenderContext*                    xiiRenderContext::s_pDefaultInstance = nullptr;
xiiHybridArray<xiiRenderContext*, 4> xiiRenderContext::s_Instances;

xiiMap<xiiRenderContext::ShaderVertexDecl, xiiGALVertexDeclarationHandle> xiiRenderContext::s_GALVertexDeclarations;

xiiMutex                                                              xiiRenderContext::s_ConstantBufferStorageMutex;
xiiIdTable<xiiConstantBufferStorageId, xiiConstantBufferStorageBase*> xiiRenderContext::s_ConstantBufferStorageTable;
xiiMap<xiiUInt32, xiiDynamicArray<xiiConstantBufferStorageBase*>>     xiiRenderContext::s_FreeConstantBufferStorage;

xiiGALSamplerStateHandle xiiRenderContext::s_hDefaultSamplerStates[4];

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RendererContext)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiShaderUtils::g_RequestBuiltinShaderCallback = xiiMakeDelegate(xiiRenderContext::LoadBuiltinShader);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiShaderUtils::g_RequestBuiltinShaderCallback = {};
    xiiRenderContext::OnEngineShutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

xiiRenderContext::Statistics::Statistics()
{
  Reset();
}

void xiiRenderContext::Statistics::Reset()
{
  m_uiFailedDrawcalls = 0;
}

//////////////////////////////////////////////////////////////////////////

xiiRenderContext* xiiRenderContext::GetDefaultInstance()
{
  if (s_pDefaultInstance == nullptr)
    s_pDefaultInstance = CreateInstance();

  return s_pDefaultInstance;
}

xiiRenderContext* xiiRenderContext::CreateInstance()
{
  return XII_DEFAULT_NEW(xiiRenderContext);
}

void xiiRenderContext::DestroyInstance(xiiRenderContext* pRenderer)
{
  XII_DEFAULT_DELETE(pRenderer);
}

xiiRenderContext::xiiRenderContext()
{
  if (s_pDefaultInstance == nullptr)
  {
    s_pDefaultInstance = this;
  }

  s_Instances.PushBack(this);

  m_StateFlags                 = xiiRenderContextFlags::AllStatesInvalid;
  m_Topology                   = xiiGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;
  m_DefaultTextureFilter       = xiiTextureFilterSetting::FixedAnisotropic4x;
  m_bAllowAsyncShaderLoading   = false;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<xiiGlobalConstants>(XII_STRINGIZE(xiiGlobalConstants));

  ResetContextState();
}

xiiRenderContext::~xiiRenderContext()
{
  DeleteConstantBufferStorage(m_hGlobalConstantBufferStorage);

  if (s_pDefaultInstance == this)
    s_pDefaultInstance = nullptr;

  s_Instances.RemoveAndSwap(this);
}

xiiRenderContext::Statistics xiiRenderContext::GetAndResetStatistics()
{
  xiiRenderContext::Statistics ret = m_Statistics;
  ret.Reset();

  return ret;
}

xiiGALRenderCommandEncoder* xiiRenderContext::BeginRendering(xiiGALPass* pGALPass, const xiiGALRenderingSetup& renderingSetup, const xiiRectFloat& viewport, const char* szName, bool bStereoSupport)
{
  xiiGALMSAASampleCount::Enum msaaSampleCount = xiiGALMSAASampleCount::None;

  xiiGALRenderTargetViewHandle hRTV;
  if (renderingSetup.m_RenderTargetSetup.GetRenderTargetCount() > 0)
  {
    hRTV = renderingSetup.m_RenderTargetSetup.GetRenderTarget(0);
  }
  if (hRTV.IsInvalidated())
  {
    hRTV = renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();
  }

  if (const xiiGALRenderTargetView* pRTV = xiiGALDevice::GetDefaultDevice()->GetRenderTargetView(hRTV))
  {
    msaaSampleCount = pRTV->GetTexture()->GetDescription().m_SampleCount;
  }

  if (msaaSampleCount != xiiGALMSAASampleCount::None)
  {
    SetShaderPermutationVariable("MSAA", "TRUE");
  }
  else
  {
    SetShaderPermutationVariable("MSAA", "FALSE");
  }

  auto& gc          = WriteGlobalConstants();
  gc.ViewportSize   = xiiVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  gc.NumMsaaSamples = msaaSampleCount;

  auto pGALCommandEncoder = pGALPass->BeginRendering(renderingSetup, szName);

  pGALCommandEncoder->SetViewport(viewport);

  m_pGALPass           = pGALPass;
  m_pGALCommandEncoder = pGALCommandEncoder;
  m_bCompute           = false;
  m_bStereoRendering   = bStereoSupport;

  return pGALCommandEncoder;
}

void xiiRenderContext::EndRendering()
{
  m_pGALPass->EndRendering(GetRenderCommandEncoder());

  m_pGALPass           = nullptr;
  m_pGALCommandEncoder = nullptr;
  m_bStereoRendering   = false;

  // TODO: The render context needs to reset its state after every encoding block if we want to record to separate command buffers.
  // Although this is currently not possible since a lot of high level code binds stuff only once per frame on the render context.
  // Resetting the state after every encoding block breaks those assumptions.
  //ResetContextState();
}

xiiGALComputeCommandEncoder* xiiRenderContext::BeginCompute(xiiGALPass* pGALPass, const char* szName /*= ""*/)
{
  auto pGALCommandEncoder = pGALPass->BeginCompute(szName);

  m_pGALPass           = pGALPass;
  m_pGALCommandEncoder = pGALCommandEncoder;
  m_bCompute           = true;

  return pGALCommandEncoder;
}

void xiiRenderContext::EndCompute()
{
  m_pGALPass->EndCompute(GetComputeCommandEncoder());

  m_pGALPass           = nullptr;
  m_pGALCommandEncoder = nullptr;

  // TODO: See EndRendering
  //ResetContextState();
}

void xiiRenderContext::SetShaderPermutationVariable(const char* szName, const xiiTempHashedString& sTempValue)
{
  xiiTempHashedString sHashedName(szName);

  xiiHashedString sName;
  xiiHashedString sValue;
  if (xiiShaderManager::IsPermutationValueAllowed(szName, sHashedName, sTempValue, sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void xiiRenderContext::SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  if (xiiShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}


void xiiRenderContext::BindMaterial(const xiiMaterialResourceHandle& hMaterial)
{
  // Don't set m_hMaterial directly since we first need to check whether the material has been modified in the mean time.
  m_hNewMaterial = hMaterial;
  m_StateFlags.Add(xiiRenderContextFlags::MaterialBindingChanged);
}

void xiiRenderContext::BindTexture2D(const xiiTempHashedString& sSlotName, const xiiTexture2DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode /*= xiiResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    xiiResourceLock<xiiTexture2DResource> pTexture(hTexture, acquireMode);
    BindTexture2D(sSlotName, xiiGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture2D(sSlotName, xiiGALResourceViewHandle());
  }
}

void xiiRenderContext::BindTexture3D(const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode /*= xiiResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    xiiResourceLock<xiiTexture3DResource> pTexture(hTexture, acquireMode);
    BindTexture3D(sSlotName, xiiGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture3D(sSlotName, xiiGALResourceViewHandle());
  }
}

void xiiRenderContext::BindTextureCube(const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode /*= xiiResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    xiiResourceLock<xiiTextureCubeResource> pTexture(hTexture, acquireMode);
    BindTextureCube(sSlotName, xiiGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTextureCube(sSlotName, xiiGALResourceViewHandle());
  }
}

void xiiRenderContext::BindTexture2D(const xiiTempHashedString& sSlotName, xiiGALResourceViewHandle hResourceView)
{
  xiiGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextures2D.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTextures2D.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged);
}

void xiiRenderContext::BindTexture3D(const xiiTempHashedString& sSlotName, xiiGALResourceViewHandle hResourceView)
{
  xiiGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextures3D.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTextures3D.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged);
}

void xiiRenderContext::BindTextureCube(const xiiTempHashedString& sSlotName, xiiGALResourceViewHandle hResourceView)
{
  xiiGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTexturesCube.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTexturesCube.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged);
}

void xiiRenderContext::BindUAV(const xiiTempHashedString& sSlotName, xiiGALUnorderedAccessViewHandle hUnorderedAccessView)
{
  xiiGALUnorderedAccessViewHandle* pOldResourceView = nullptr;
  if (m_BoundUAVs.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hUnorderedAccessView)
      return;

    *pOldResourceView = hUnorderedAccessView;
  }
  else
  {
    m_BoundUAVs.Insert(sSlotName.GetHash(), hUnorderedAccessView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::UAVBindingChanged);
}


void xiiRenderContext::BindSamplerState(const xiiTempHashedString& sSlotName, xiiGALSamplerStateHandle hSamplerSate)
{
  XII_ASSERT_DEBUG(sSlotName != "LinearSampler", "'LinearSampler' is a resevered sampler name and must not be set manually.");
  XII_ASSERT_DEBUG(sSlotName != "LinearClampSampler", "'LinearClampSampler' is a resevered sampler name and must not be set manually.");
  XII_ASSERT_DEBUG(sSlotName != "PointSampler", "'PointSampler' is a resevered sampler name and must not be set manually.");
  XII_ASSERT_DEBUG(sSlotName != "PointClampSampler", "'PointClampSampler' is a resevered sampler name and must not be set manually.");

  xiiGALSamplerStateHandle* pOldSamplerState = nullptr;
  if (m_BoundSamplers.TryGetValue(sSlotName.GetHash(), pOldSamplerState))
  {
    if (*pOldSamplerState == hSamplerSate)
      return;

    *pOldSamplerState = hSamplerSate;
  }
  else
  {
    m_BoundSamplers.Insert(sSlotName.GetHash(), hSamplerSate);
  }

  m_StateFlags.Add(xiiRenderContextFlags::SamplerBindingChanged);
}

void xiiRenderContext::BindBuffer(const xiiTempHashedString& sSlotName, xiiGALResourceViewHandle hResourceView)
{
  xiiGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundBuffer.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundBuffer.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(xiiRenderContextFlags::BufferBindingChanged);
}

void xiiRenderContext::BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiGALBufferHandle hConstantBuffer)
{
  BoundConstantBuffer* pBoundConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pBoundConstantBuffer))
  {
    if (pBoundConstantBuffer->m_hConstantBuffer == hConstantBuffer)
      return;

    pBoundConstantBuffer->m_hConstantBuffer = hConstantBuffer;
    pBoundConstantBuffer->m_hConstantBufferStorage.Invalidate();
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), BoundConstantBuffer(hConstantBuffer));
  }

  m_StateFlags.Add(xiiRenderContextFlags::ConstantBufferBindingChanged);
}

void xiiRenderContext::BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiConstantBufferStorageHandle hConstantBufferStorage)
{
  BoundConstantBuffer* pBoundConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pBoundConstantBuffer))
  {
    if (pBoundConstantBuffer->m_hConstantBufferStorage == hConstantBufferStorage)
      return;

    pBoundConstantBuffer->m_hConstantBuffer.Invalidate();
    pBoundConstantBuffer->m_hConstantBufferStorage = hConstantBufferStorage;
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), BoundConstantBuffer(hConstantBufferStorage));
  }

  m_StateFlags.Add(xiiRenderContextFlags::ConstantBufferBindingChanged);
}

void xiiRenderContext::BindShader(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags)
{
  m_hMaterial.Invalidate();
  m_StateFlags.Remove(xiiRenderContextFlags::MaterialBindingChanged);

  BindShaderInternal(hShader, flags);
}

void xiiRenderContext::BindMeshBuffer(const xiiMeshBufferResourceHandle& hMeshBuffer)
{
  xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetTopology(), pMeshBuffer->GetPrimitiveCount());
}

void xiiRenderContext::BindMeshBuffer(xiiGALBufferHandle hVertexBuffer, xiiGALBufferHandle hIndexBuffer, const xiiVertexDeclarationInfo* pVertexDeclarationInfo, xiiGALPrimitiveTopology::Enum topology, xiiUInt32 uiPrimitiveCount, xiiGALBufferHandle hVertexBuffer2, xiiGALBufferHandle hVertexBuffer3, xiiGALBufferHandle hVertexBuffer4)
{
  if (m_hVertexBuffers[0] == hVertexBuffer && m_hVertexBuffers[1] == hVertexBuffer2 && m_hVertexBuffers[2] == hVertexBuffer3 && m_hVertexBuffers[3] == hVertexBuffer4 && m_hIndexBuffer == hIndexBuffer && m_pVertexDeclarationInfo == pVertexDeclarationInfo && m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
  {
    return;
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (pVertexDeclarationInfo)
  {
    for (xiiUInt32 i1 = 0; i1 < pVertexDeclarationInfo->m_VertexStreams.GetCount(); ++i1)
    {
      for (xiiUInt32 i2 = 0; i2 < pVertexDeclarationInfo->m_VertexStreams.GetCount(); ++i2)
      {
        if (i1 != i2)
        {
          XII_ASSERT_DEBUG(pVertexDeclarationInfo->m_VertexStreams[i1].m_Semantic != pVertexDeclarationInfo->m_VertexStreams[i2].m_Semantic,
                           "Same semantic cannot be used twice in the same vertex declaration");
        }
      }
    }
  }
#endif

  if (m_Topology != topology)
  {
    m_Topology = topology;

    xiiTempHashedString sTopologies[xiiGALPrimitiveTopology::ENUM_COUNT] = {xiiTempHashedString("TOPOLOGY_POINTS"), xiiTempHashedString("TOPOLOGY_LINES"), xiiTempHashedString("TOPOLOGY_TRIANGLES")};

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_Topology]);
  }

  m_hVertexBuffers[0]          = hVertexBuffer;
  m_hVertexBuffers[1]          = hVertexBuffer2;
  m_hVertexBuffers[2]          = hVertexBuffer3;
  m_hVertexBuffers[3]          = hVertexBuffer4;
  m_hIndexBuffer               = hIndexBuffer;
  m_pVertexDeclarationInfo     = pVertexDeclarationInfo;
  m_uiMeshBufferPrimitiveCount = uiPrimitiveCount;

  m_StateFlags.Add(xiiRenderContextFlags::MeshBufferBindingChanged);
}

void xiiRenderContext::BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer)
{
  xiiResourceLock<xiiDynamicMeshBufferResource> pMeshBuffer(hDynamicMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetDescriptor().m_Topology, pMeshBuffer->GetDescriptor().m_uiMaxPrimitives, pMeshBuffer->GetColorBuffer());
}

xiiResult xiiRenderContext::DrawMeshBuffer(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed() || uiPrimitiveCount == 0 || uiInstanceCount == 0)
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return XII_FAILURE;
  }

  XII_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount, "Invalid primitive range: first primitive ({0}) can't be larger than number of primitives ({1})", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = xiiMath::Min(uiPrimitiveCount, m_uiMeshBufferPrimitiveCount - uiFirstPrimitive);
  XII_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  auto pCommandEncoder = GetRenderCommandEncoder();

  const xiiUInt32 uiVertsPerPrimitive = xiiGALPrimitiveTopology::VerticesPerPrimitive(pCommandEncoder->GetPrimitiveTopology());

  uiPrimitiveCount *= uiVertsPerPrimitive;
  uiFirstPrimitive *= uiVertsPerPrimitive;
  if (m_bStereoRendering)
  {
    uiInstanceCount *= 2;
  }

  if (uiInstanceCount > 1)
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      pCommandEncoder->DrawIndexedInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
    else
    {
      pCommandEncoder->DrawInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
  }
  else
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      pCommandEncoder->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      pCommandEncoder->Draw(uiPrimitiveCount, uiFirstPrimitive);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiRenderContext::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  if (ApplyContextStates().Failed())
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return XII_FAILURE;
  }

  GetComputeCommandEncoder()->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  return XII_SUCCESS;
}

xiiResult xiiRenderContext::ApplyContextStates(bool bForce)
{
  // First apply material state since this can modify all other states.
  // Note ApplyMaterialState only returns a valid material pointer if the constant buffer of this material needs to be updated.
  // This needs to be done once we have determined the correct shader permutation.
  xiiMaterialResource* pMaterial = nullptr;
  XII_SCOPE_EXIT(if (pMaterial != nullptr) { xiiResourceManager::EndAcquireResource(pMaterial); });

  if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::MaterialBindingChanged))
  {
    pMaterial = ApplyMaterialState();

    m_StateFlags.Remove(xiiRenderContextFlags::MaterialBindingChanged);
  }

  xiiShaderPermutationResource* pShaderPermutation = nullptr;
  XII_SCOPE_EXIT(if (pShaderPermutation != nullptr) { xiiResourceManager::EndAcquireResource(pShaderPermutation); });

  bool bRebuildVertexDeclaration = m_StateFlags.IsAnySet(xiiRenderContextFlags::ShaderStateChanged | xiiRenderContextFlags::MeshBufferBindingChanged);

  if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::ShaderStateChanged))
  {
    pShaderPermutation = ApplyShaderState();

    if (pShaderPermutation == nullptr)
    {
      return XII_FAILURE;
    }

    m_StateFlags.Remove(xiiRenderContextFlags::ShaderStateChanged);
  }

  if (m_hActiveShaderPermutation.IsValid())
  {
    if ((bForce || m_StateFlags.IsAnySet(xiiRenderContextFlags::TextureBindingChanged | xiiRenderContextFlags::UAVBindingChanged | xiiRenderContextFlags::SamplerBindingChanged | xiiRenderContextFlags::BufferBindingChanged | xiiRenderContextFlags::ConstantBufferBindingChanged)))
    {
      if (pShaderPermutation == nullptr)
        pShaderPermutation = xiiResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    }

    xiiLogBlock applyBindingsBlock("Applying Shader Bindings", pShaderPermutation != nullptr ? pShaderPermutation->GetResourceDescription().GetData() : "");

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::UAVBindingChanged))
    {
      // RWTextures/UAV are usually only supported in compute and pixel shader.
      if (auto pBin = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::ComputeShader))
      {
        ApplyUAVBindings(pBin);
      }
      if (auto pBin = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::PixelShader))
      {
        ApplyUAVBindings(pBin);
      }

      m_StateFlags.Remove(xiiRenderContextFlags::UAVBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::TextureBindingChanged))
    {
      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((xiiGALShaderStage::Enum)stage))
        {
          ApplyTextureBindings((xiiGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::TextureBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::SamplerBindingChanged))
    {
      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((xiiGALShaderStage::Enum)stage))
        {
          ApplySamplerBindings((xiiGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::SamplerBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::BufferBindingChanged))
    {
      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((xiiGALShaderStage::Enum)stage))
        {
          ApplyBufferBindings((xiiGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::BufferBindingChanged);
    }

    if (pMaterial != nullptr)
    {
      pMaterial->UpdateConstantBuffer(pShaderPermutation);
      BindConstantBuffer(XII_STRINGIZE(xiiMaterialConstants), pMaterial->m_hConstantBufferStorage);
    }

    UploadConstants();

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::ConstantBufferBindingChanged))
    {
      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((xiiGALShaderStage::Enum)stage))
        {
          ApplyConstantBufferBindings(pBin);
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::ConstantBufferBindingChanged);
    }
  }

  if ((bForce || bRebuildVertexDeclaration) && !m_bCompute)
  {
    if (m_hActiveGALShader.IsInvalidated())
      return XII_FAILURE;

    auto pCommandEncoder = GetRenderCommandEncoder();

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::MeshBufferBindingChanged))
    {
      pCommandEncoder->SetPrimitiveTopology(m_Topology);

      for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hVertexBuffers); ++i)
      {
        pCommandEncoder->SetVertexBuffer(i, m_hVertexBuffers[i]);
      }

      if (!m_hIndexBuffer.IsInvalidated())
        pCommandEncoder->SetIndexBuffer(m_hIndexBuffer);
    }

    xiiGALVertexDeclarationHandle hVertexDeclaration;
    if (m_pVertexDeclarationInfo != nullptr && BuildVertexDeclaration(m_hActiveGALShader, *m_pVertexDeclarationInfo, hVertexDeclaration).Failed())
      return XII_FAILURE;

    // If there is a vertex buffer we need a valid vertex declaration as well.
    if ((!m_hVertexBuffers[0].IsInvalidated() || !m_hVertexBuffers[1].IsInvalidated() || !m_hVertexBuffers[2].IsInvalidated() || !m_hVertexBuffers[3].IsInvalidated()) && hVertexDeclaration.IsInvalidated())
      return XII_FAILURE;

    pCommandEncoder->SetVertexDeclaration(hVertexDeclaration);

    m_StateFlags.Remove(xiiRenderContextFlags::MeshBufferBindingChanged);
  }

  return XII_SUCCESS;
}

void xiiRenderContext::ResetContextState()
{
  m_StateFlags = xiiRenderContextFlags::AllStatesInvalid;

  m_hActiveShader.Invalidate();
  m_hActiveGALShader.Invalidate();

  m_PermutationVariables.Clear();
  m_hNewMaterial.Invalidate();
  m_hMaterial.Invalidate();

  m_hActiveShaderPermutation.Invalidate();

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }

  m_hIndexBuffer.Invalidate();
  m_pVertexDeclarationInfo     = nullptr;
  m_Topology                   = xiiGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;

  m_BoundTextures2D.Clear();
  m_BoundTextures3D.Clear();
  m_BoundTexturesCube.Clear();
  m_BoundBuffer.Clear();

  m_BoundSamplers.Clear();
  m_BoundSamplers.Insert(xiiHashingUtils::StringHash("LinearSampler"), GetDefaultSamplerState(xiiDefaultSamplerFlags::LinearFiltering));
  m_BoundSamplers.Insert(xiiHashingUtils::StringHash("LinearClampSampler"), GetDefaultSamplerState(xiiDefaultSamplerFlags::LinearFiltering | xiiDefaultSamplerFlags::Clamp));
  m_BoundSamplers.Insert(xiiHashingUtils::StringHash("PointSampler"), GetDefaultSamplerState(xiiDefaultSamplerFlags::PointFiltering));
  m_BoundSamplers.Insert(xiiHashingUtils::StringHash("PointClampSampler"), GetDefaultSamplerState(xiiDefaultSamplerFlags::PointFiltering | xiiDefaultSamplerFlags::Clamp));

  m_BoundUAVs.Clear();
  m_BoundConstantBuffers.Clear();
}

xiiGlobalConstants& xiiRenderContext::WriteGlobalConstants()
{
  xiiConstantBufferStorage<xiiGlobalConstants>* pStorage = nullptr;
  XII_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForWriting();
}

const xiiGlobalConstants& xiiRenderContext::ReadGlobalConstants() const
{
  xiiConstantBufferStorage<xiiGlobalConstants>* pStorage = nullptr;
  XII_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForReading();
}

// static
xiiConstantBufferStorageHandle xiiRenderContext::CreateConstantBufferStorage(xiiUInt32 uiSizeInBytes, xiiConstantBufferStorageBase*& out_pStorage, const char* szName)
{
  XII_ASSERT_DEV(xiiMemoryUtils::IsSizeAligned(uiSizeInBytes, 16u), "Storage struct for constant buffer is not aligned to 16 bytes");

  XII_LOCK(s_ConstantBufferStorageMutex);

  xiiConstantBufferStorageBase* pStorage = nullptr;

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (it.IsValid())
  {
    xiiDynamicArray<xiiConstantBufferStorageBase*>& storageForSize = it.Value();
    if (!storageForSize.IsEmpty())
    {
      pStorage = storageForSize[0];
      storageForSize.RemoveAtAndSwap(0);
    }
  }

  if (pStorage == nullptr)
  {
    pStorage = XII_DEFAULT_NEW(xiiConstantBufferStorageBase, uiSizeInBytes, szName);
  }

  out_pStorage = pStorage;
  return xiiConstantBufferStorageHandle(s_ConstantBufferStorageTable.Insert(pStorage));
}

// static
void xiiRenderContext::DeleteConstantBufferStorage(xiiConstantBufferStorageHandle hStorage)
{
  XII_LOCK(s_ConstantBufferStorageMutex);

  xiiConstantBufferStorageBase* pStorage = nullptr;
  if (!s_ConstantBufferStorageTable.Remove(hStorage.m_InternalId, &pStorage))
  {
    // already deleted
    return;
  }

  xiiUInt32 uiSizeInBytes = pStorage->m_Data.GetCount();

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (!it.IsValid())
  {
    it = s_FreeConstantBufferStorage.Insert(uiSizeInBytes, xiiDynamicArray<xiiConstantBufferStorageBase*>());
  }

  it.Value().PushBack(pStorage);
}

// static
bool xiiRenderContext::TryGetConstantBufferStorage(xiiConstantBufferStorageHandle hStorage, xiiConstantBufferStorageBase*& out_pStorage)
{
  XII_LOCK(s_ConstantBufferStorageMutex);
  return s_ConstantBufferStorageTable.TryGetValue(hStorage.m_InternalId, out_pStorage);
}

// static
xiiGALSamplerStateHandle xiiRenderContext::GetDefaultSamplerState(xiiBitflags<xiiDefaultSamplerFlags> flags)
{
  xiiUInt32 uiSamplerStateIndex = flags.GetValue();
  XII_ASSERT_DEV(uiSamplerStateIndex < XII_ARRAY_SIZE(s_hDefaultSamplerStates), "");

  if (s_hDefaultSamplerStates[uiSamplerStateIndex].IsInvalidated())
  {
    xiiGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALTextureFilterMode::Linear : xiiGALTextureFilterMode::Point;
    desc.m_MagFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALTextureFilterMode::Linear : xiiGALTextureFilterMode::Point;
    desc.m_MipFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALTextureFilterMode::Linear : xiiGALTextureFilterMode::Point;

    desc.m_AddressU = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiImageAddressMode::Clamp : xiiImageAddressMode::Repeat;
    desc.m_AddressV = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiImageAddressMode::Clamp : xiiImageAddressMode::Repeat;
    desc.m_AddressW = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiImageAddressMode::Clamp : xiiImageAddressMode::Repeat;

    s_hDefaultSamplerStates[uiSamplerStateIndex] = xiiGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }

  return s_hDefaultSamplerStates[uiSamplerStateIndex];
}

// private functions
//////////////////////////////////////////////////////////////////////////

// static
void xiiRenderContext::LoadBuiltinShader(xiiShaderUtils::xiiBuiltinShaderType type, xiiShaderUtils::xiiBuiltinShader& out_shader)
{
  xiiShaderResourceHandle hActiveShader;
  bool                    bStereo = false;
  switch (type)
  {
    case xiiShaderUtils::xiiBuiltinShaderType::CopyImageArray:
      bStereo = true;
      [[fallthrough]];
    case xiiShaderUtils::xiiBuiltinShaderType::CopyImage:
      hActiveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Copy.xiiShader");
      break;
    case xiiShaderUtils::xiiBuiltinShaderType::DownscaleImageArray:
      bStereo = true;
      [[fallthrough]];
    case xiiShaderUtils::xiiBuiltinShaderType::DownscaleImage:
      hActiveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Downscale.xiiShader");
      break;
  }

  XII_ASSERT_DEV(hActiveShader.IsValid(), "Could not load builtin shader!");

  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables;
  static xiiHashedString                         sVSRTAI      = xiiMakeHashedString("VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX");
  static xiiHashedString                         sTrue        = xiiMakeHashedString("TRUE");
  static xiiHashedString                         sFalse       = xiiMakeHashedString("FALSE");
  static xiiHashedString                         sCameraMode  = xiiMakeHashedString("CAMERA_MODE");
  static xiiHashedString                         sPerspective = xiiMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static xiiHashedString                         sStereo      = xiiMakeHashedString("CAMERA_MODE_STEREO");

  permutationVariables.Insert(sCameraMode, bStereo ? sStereo : sPerspective);
  if (xiiGALDevice::GetDefaultDevice()->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
    permutationVariables.Insert(sVSRTAI, sTrue);
  else
    permutationVariables.Insert(sVSRTAI, sFalse);


  xiiShaderPermutationResourceHandle hActiveShaderPermutation = xiiShaderManager::PreloadSinglePermutation(hActiveShader, permutationVariables, false);

  XII_ASSERT_DEV(hActiveShaderPermutation.IsValid(), "Could not load builtin shader permutation!");

  xiiResourceLock<xiiShaderPermutationResource> pShaderPermutation(hActiveShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);

  XII_ASSERT_DEV(pShaderPermutation->IsShaderValid(), "Builtin shader permutation shader is invalid!");

  out_shader.m_hActiveGALShader = pShaderPermutation->GetGALShader();
  XII_ASSERT_DEV(!out_shader.m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  out_shader.m_hBlendState        = pShaderPermutation->GetBlendState();
  out_shader.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
  out_shader.m_hRasterizerState   = pShaderPermutation->GetRasterizerState();
}

// static
void xiiRenderContext::OnEngineShutdown()
{
  xiiShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    XII_DEFAULT_DELETE(rc);

  s_Instances.Clear();

  // Cleanup sampler states
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_hDefaultSamplerStates); ++i)
  {
    if (!s_hDefaultSamplerStates[i].IsInvalidated())
    {
      xiiGALDevice::GetDefaultDevice()->DestroySamplerState(s_hDefaultSamplerStates[i]);
      s_hDefaultSamplerStates[i].Invalidate();
    }
  }

  // Cleanup vertex declarations
  {
    for (auto it = s_GALVertexDeclarations.GetIterator(); it.IsValid(); ++it)
    {
      xiiGALDevice::GetDefaultDevice()->DestroyVertexDeclaration(it.Value());
    }

    s_GALVertexDeclarations.Clear();
  }

  // Cleanup constant buffer storage
  {
    for (auto it = s_ConstantBufferStorageTable.GetIterator(); it.IsValid(); ++it)
    {
      xiiConstantBufferStorageBase* pStorage = it.Value();
      XII_DEFAULT_DELETE(pStorage);
    }

    s_ConstantBufferStorageTable.Clear();

    for (auto it = s_FreeConstantBufferStorage.GetIterator(); it.IsValid(); ++it)
    {
      xiiDynamicArray<xiiConstantBufferStorageBase*>& storageForSize = it.Value();
      for (auto& pStorage : storageForSize)
      {
        XII_DEFAULT_DELETE(pStorage);
      }
    }

    s_FreeConstantBufferStorage.Clear();
  }
}

// static
xiiResult xiiRenderContext::BuildVertexDeclaration(xiiGALShaderHandle hShader, const xiiVertexDeclarationInfo& decl, xiiGALVertexDeclarationHandle& out_Declaration)
{
  ShaderVertexDecl svd;
  svd.m_hShader                 = hShader;
  svd.m_uiVertexDeclarationHash = decl.m_uiHash;

  bool bExisted = false;
  auto it       = s_GALVertexDeclarations.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    const xiiGALShader* pShader = xiiGALDevice::GetDefaultDevice()->GetShader(hShader);

    auto pBytecode = pShader->GetDescription().m_ByteCodes[xiiGALShaderStage::VertexShader];

    xiiGALVertexDeclarationCreationDescription vd;
    vd.m_hShader = hShader;

    for (xiiUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
    {
      auto& stream = decl.m_VertexStreams[slot];

      // stream.m_Format
      xiiGALVertexAttribute gal;
      gal.m_bInstanceData      = false;
      gal.m_eFormat            = stream.m_Format;
      gal.m_eSemantic          = stream.m_Semantic;
      gal.m_uiOffset           = stream.m_uiOffset;
      gal.m_uiVertexBufferSlot = stream.m_uiVertexBufferSlot;
      vd.m_VertexAttributes.PushBack(gal);
    }

    out_Declaration = xiiGALDevice::GetDefaultDevice()->CreateVertexDeclaration(vd);

    if (out_Declaration.IsInvalidated())
    {
      /* This can happen when the resource system gives you a fallback resource, which then selects a shader that
      does not fit the mesh layout.
      E.g. when a material is not yet loaded and the fallback material is used, that fallback material may
      use another shader, that requires more data streams, than what the mesh provides.
      This problem will go away, once the proper material is loaded.

      This can be fixed by ensuring that the fallback material uses a shader that only requires data that is
      always there, e.g. only position and maybe a texcoord, and of course all meshes must provide at least those
      data streams.

      Otherwise, this is harmless, the renderer will ignore invalid drawcalls and once all the correct stuff is
      available, it will work.
      */

      xiiLog::Warning("Failed to create vertex declaration");
      return XII_FAILURE;
    }

    it.Value() = out_Declaration;
  }

  out_Declaration = it.Value();
  return XII_SUCCESS;
}

void xiiRenderContext::UploadConstants()
{
  BindConstantBuffer(XII_STRINGIZE(xiiGlobalConstants), m_hGlobalConstantBufferStorage);

  for (auto it = m_BoundConstantBuffers.GetIterator(); it.IsValid(); ++it)
  {
    xiiConstantBufferStorageHandle hConstantBufferStorage = it.Value().m_hConstantBufferStorage;
    xiiConstantBufferStorageBase*  pConstantBufferStorage = nullptr;
    if (TryGetConstantBufferStorage(hConstantBufferStorage, pConstantBufferStorage))
    {
      pConstantBufferStorage->UploadData(m_pGALCommandEncoder);
    }
  }
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

void xiiRenderContext::BindShaderInternal(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags)
{
  if (flags.IsAnySet(xiiShaderBindFlags::ForceRebind) || m_hActiveShader != hShader)
  {
    m_ShaderBindFlags = flags;
    m_hActiveShader   = hShader;

    m_StateFlags.Add(xiiRenderContextFlags::ShaderStateChanged);
  }
}

xiiShaderPermutationResource* xiiRenderContext::ApplyShaderState()
{
  m_hActiveGALShader.Invalidate();

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged | xiiRenderContextFlags::SamplerBindingChanged |
                   xiiRenderContextFlags::BufferBindingChanged | xiiRenderContextFlags::ConstantBufferBindingChanged);

  if (!m_hActiveShader.IsValid())
    return nullptr;

  m_hActiveShaderPermutation = xiiShaderManager::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, m_bAllowAsyncShaderLoading);

  if (!m_hActiveShaderPermutation.IsValid())
    return nullptr;

  xiiShaderPermutationResource* pShaderPermutation = xiiResourceManager::BeginAcquireResource(
    m_hActiveShaderPermutation, m_bAllowAsyncShaderLoading ? xiiResourceAcquireMode::AllowLoadingFallback : xiiResourceAcquireMode::BlockTillLoaded);

  if (!pShaderPermutation->IsShaderValid())
  {
    xiiResourceManager::EndAcquireResource(pShaderPermutation);
    return nullptr;
  }

  m_hActiveGALShader = pShaderPermutation->GetGALShader();
  XII_ASSERT_DEV(!m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  m_pGALCommandEncoder->SetShader(m_hActiveGALShader);

  // Set render state from shader
  if (!m_bCompute)
  {
    auto pCommandEncoder = GetRenderCommandEncoder();

    if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoBlendState))
      pCommandEncoder->SetBlendState(pShaderPermutation->GetBlendState());

    if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoRasterizerState))
      pCommandEncoder->SetRasterizerState(pShaderPermutation->GetRasterizerState());

    if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoDepthStencilState))
      pCommandEncoder->SetDepthStencilState(pShaderPermutation->GetDepthStencilState());
  }

  return pShaderPermutation;
}

xiiMaterialResource* xiiRenderContext::ApplyMaterialState()
{
  if (!m_hNewMaterial.IsValid())
  {
    BindShaderInternal(xiiShaderResourceHandle(), xiiShaderBindFlags::Default);
    return nullptr;
  }

  // check whether material has been modified
  xiiMaterialResource* pMaterial = xiiResourceManager::BeginAcquireResource(m_hNewMaterial, xiiResourceAcquireMode::AllowLoadingFallback);

  if (m_hNewMaterial != m_hMaterial || pMaterial->IsModified())
  {
    auto pCachedValues = pMaterial->GetOrUpdateCachedValues();

    BindShaderInternal(pCachedValues->m_hShader, xiiShaderBindFlags::Default);

    if (!pMaterial->m_hConstantBufferStorage.IsInvalidated())
    {
      BindConstantBuffer(XII_STRINGIZE(xiiMaterialConstants), pMaterial->m_hConstantBufferStorage);
    }

    for (auto it = pCachedValues->m_PermutationVars.GetIterator(); it.IsValid(); ++it)
    {
      SetShaderPermutationVariableInternal(it.Key(), it.Value());
    }

    for (auto it = pCachedValues->m_Texture2DBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTexture2D(it.Key(), it.Value());
    }

    for (auto it = pCachedValues->m_TextureCubeBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTextureCube(it.Key(), it.Value());
    }

    m_hMaterial = m_hNewMaterial;
  }

  // The material needs its constant buffer updated.
  // Thus we keep it acquired until we have the correct shader permutation for the constant buffer layout.
  if (pMaterial->AreConstantsModified())
  {
    m_StateFlags.Add(xiiRenderContextFlags::ConstantBufferBindingChanged);

    return pMaterial;
  }

  xiiResourceManager::EndAcquireResource(pMaterial);
  return nullptr;
}

void xiiRenderContext::ApplyConstantBufferBindings(const xiiShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != xiiShaderResourceType::ConstantBuffer)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    BoundConstantBuffer boundConstantBuffer;
    if (!m_BoundConstantBuffers.TryGetValue(uiResourceHash, boundConstantBuffer))
    {
      // If the shader was compiled with debug info the shader compiler will not strip unused resources and
      // thus this error would trigger although the shader doesn't actually uses the resource.
      if (!pBinary->m_bWasCompiledWithDebug)
      {
        xiiLog::Error("No resource is bound for constant buffer slot '{0}'", binding.m_sName);
      }
      m_pGALCommandEncoder->SetConstantBuffer(binding.m_iSlot, xiiGALBufferHandle());
      continue;
    }

    if (!boundConstantBuffer.m_hConstantBuffer.IsInvalidated())
    {
      m_pGALCommandEncoder->SetConstantBuffer(binding.m_iSlot, boundConstantBuffer.m_hConstantBuffer);
    }
    else
    {
      xiiConstantBufferStorageBase* pConstantBufferStorage = nullptr;
      if (TryGetConstantBufferStorage(boundConstantBuffer.m_hConstantBufferStorage, pConstantBufferStorage))
      {
        m_pGALCommandEncoder->SetConstantBuffer(binding.m_iSlot, pConstantBufferStorage->GetGALBufferHandle());
      }
      else
      {
        xiiLog::Error("Invalid constant buffer storage is bound for slot '{0}'", binding.m_sName);
        m_pGALCommandEncoder->SetConstantBuffer(binding.m_iSlot, xiiGALBufferHandle());
      }
    }
  }
}

void xiiRenderContext::ApplyTextureBindings(xiiGALShaderStage::Enum stage, const xiiShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    // we currently only support 2D and cube textures

    const xiiUInt64          uiResourceHash = binding.m_sName.GetHash();
    xiiGALResourceViewHandle hResourceView;

    if (binding.m_Type >= xiiShaderResourceType::Texture2D && binding.m_Type <= xiiShaderResourceType::Texture2DMSArray)
    {
      m_BoundTextures2D.TryGetValue(uiResourceHash, hResourceView);
      m_pGALCommandEncoder->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }

    if (binding.m_Type == xiiShaderResourceType::Texture3D)
    {
      m_BoundTextures3D.TryGetValue(uiResourceHash, hResourceView);
      m_pGALCommandEncoder->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }

    if (binding.m_Type >= xiiShaderResourceType::TextureCube && binding.m_Type <= xiiShaderResourceType::TextureCubeArray)
    {
      m_BoundTexturesCube.TryGetValue(uiResourceHash, hResourceView);
      m_pGALCommandEncoder->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }
  }
}

void xiiRenderContext::ApplyUAVBindings(const xiiShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != xiiShaderResourceType::UAV)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiGALUnorderedAccessViewHandle hResourceView;
    m_BoundUAVs.TryGetValue(uiResourceHash, hResourceView);

    m_pGALCommandEncoder->SetUnorderedAccessView(binding.m_iSlot, hResourceView);
  }
}

void xiiRenderContext::ApplySamplerBindings(xiiGALShaderStage::Enum stage, const xiiShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != xiiShaderResourceType::Sampler)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiGALSamplerStateHandle hSamplerState;
    if (!m_BoundSamplers.TryGetValue(uiResourceHash, hSamplerState))
    {
      hSamplerState = GetDefaultSamplerState(xiiDefaultSamplerFlags::LinearFiltering); // Bind a default state to avoid D3D11 errors.
    }

    m_pGALCommandEncoder->SetSamplerState(stage, binding.m_iSlot, hSamplerState);
  }
}

void xiiRenderContext::ApplyBufferBindings(xiiGALShaderStage::Enum stage, const xiiShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != xiiShaderResourceType::GenericBuffer)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiGALResourceViewHandle hResourceView;
    m_BoundBuffer.TryGetValue(uiResourceHash, hResourceView);

    m_pGALCommandEncoder->SetResourceView(stage, binding.m_iSlot, hResourceView);
  }
}

void xiiRenderContext::SetDefaultTextureFilter(xiiTextureFilterSetting::Enum filter)
{
  XII_ASSERT_DEBUG(filter >= xiiTextureFilterSetting::FixedBilinear && filter <= xiiTextureFilterSetting::FixedAnisotropic16x, "Invalid default texture filter");

  filter = xiiMath::Clamp(filter, xiiTextureFilterSetting::FixedBilinear, xiiTextureFilterSetting::FixedAnisotropic16x);

  if (m_DefaultTextureFilter == filter)
    return;

  m_DefaultTextureFilter = filter;
}

xiiTextureFilterSetting::Enum xiiRenderContext::GetSpecificTextureFilter(xiiTextureFilterSetting::Enum configuration) const
{
  if (configuration >= xiiTextureFilterSetting::FixedNearest && configuration <= xiiTextureFilterSetting::FixedAnisotropic16x)
    return configuration;

  xiiInt32 iFilter = m_DefaultTextureFilter;

  switch (configuration)
  {
    case xiiTextureFilterSetting::LowestQuality:
      iFilter -= 2;
      break;
    case xiiTextureFilterSetting::LowQuality:
      iFilter -= 1;
      break;
    case xiiTextureFilterSetting::HighQuality:
      iFilter += 1;
      break;
    case xiiTextureFilterSetting::HighestQuality:
      iFilter += 2;
      break;
    default:
      break;
  }

  iFilter = xiiMath::Clamp<xiiInt32>(iFilter, xiiTextureFilterSetting::FixedBilinear, xiiTextureFilterSetting::FixedAnisotropic16x);

  return (xiiTextureFilterSetting::Enum)iFilter;
}

void xiiRenderContext::SetAllowAsyncShaderLoading(bool bAllow)
{
  m_bAllowAsyncShaderLoading = bAllow;
}

bool xiiRenderContext::GetAllowAsyncShaderLoading()
{
  return m_bAllowAsyncShaderLoading;
}

XII_STATICLINK_FILE(RendererCore, RendererCore_RenderContext_Implementation_RenderContext);
