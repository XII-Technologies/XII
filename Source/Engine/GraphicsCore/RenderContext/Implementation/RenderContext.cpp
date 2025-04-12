#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/BufferView.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/DynamicMeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderStageBinary.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

xiiRenderContext*                    xiiRenderContext::s_pDefaultInstance = nullptr;
xiiHybridArray<xiiRenderContext*, 4> xiiRenderContext::s_Instances;

xiiMap<xiiRenderContext::ShaderVertexDecl, xiiGALInputLayoutHandle> xiiRenderContext::s_GALInputLayouts;

xiiMutex                                                              xiiRenderContext::s_ConstantBufferStorageMutex;
xiiIdTable<xiiConstantBufferStorageId, xiiConstantBufferStorageBase*> xiiRenderContext::s_ConstantBufferStorageTable;
xiiMap<xiiUInt32, xiiDynamicArray<xiiConstantBufferStorageBase*>>     xiiRenderContext::s_FreeConstantBufferStorage;

xiiGALSamplerHandle xiiRenderContext::s_hDefaultSamplers[4];

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RendererContext)

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
    xiiShaderUtilities::g_RequestBuiltinShaderCallback = xiiMakeDelegate(xiiRenderContext::LoadBuiltinShader);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiShaderUtilities::g_RequestBuiltinShaderCallback = {};
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
  {
    s_pDefaultInstance = CreateInstance();
  }

  XII_ASSERT_DEBUG(s_pDefaultInstance != nullptr, "Default instance should have been created during device creation.");

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
  m_Topology                   = xiiGALPrimitiveTopology::Undefined;
  m_uiMeshBufferPrimitiveCount = 0;
  m_DefaultTextureFilter       = xiiTextureFilterSetting::FixedAnisotropic4x;
  m_bAllowAsyncShaderLoading   = false;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<xiiGlobalConstants>();

  ResetContextState();
}

xiiRenderContext::~xiiRenderContext()
{
  xiiGALDevice* pGALDevice = xiiGALDevice::GetDefaultDevice();

  for (auto& framebufferInfo : m_FramebufferCache)
  {
    auto& hFramebuffer = framebufferInfo.Value().hFrameBuffer;
    if (hFramebuffer.IsInvalidated())
      continue;

    pGALDevice->DestroyFramebuffer(hFramebuffer);
    hFramebuffer.Invalidate();
  }

  for (auto& renderPassInfo : m_RenderPassCache)
  {
    auto& hRenderPass = renderPassInfo.Value();

    if (hRenderPass.IsInvalidated())
      continue;

    pGALDevice->DestroyRenderPass(hRenderPass);
    hRenderPass.Invalidate();
  }

  FlushPipelineStateCache();

  DeleteConstantBufferStorage(m_hGlobalConstantBufferStorage);

  if (s_pDefaultInstance == this)
  {
    s_pDefaultInstance = nullptr;
  }

  s_Instances.RemoveAndSwap(this);
}

xiiRenderContext::Statistics xiiRenderContext::GetAndResetStatistics()
{
  xiiRenderContext::Statistics ret = m_Statistics;
  ret.Reset();

  return ret;
}

void xiiRenderContext::BeginRendering(const xiiGALRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName, bool bStereoSupport)
{
  XII_ASSERT_DEV(m_bIsRendering == false && m_bIsCompute == false, "Already in a scope.");
  XII_ASSERT_DEV(m_pCommandList != nullptr, "Command list has not been set.");

  m_CurrentRenderingSetup = renderingSetup;
  m_bIsRendering          = true;
  m_bNeedsClear           = (renderingSetup.m_bClearDepth || renderingSetup.m_bClearStencil || renderingSetup.m_uiRenderTargetClearMask);
  m_bStereoRendering      = bStereoSupport;

  xiiGALDevice*           pDevice = xiiGALDevice::GetDefaultDevice();
  xiiGALTextureViewHandle hRTV;
  {
    if (renderingSetup.m_RenderTargetSetup.GetRenderTargetCount() > 0)
    {
      hRTV = renderingSetup.m_RenderTargetSetup.GetRenderTarget(0);
    }
    if (hRTV.IsInvalidated())
    {
      hRTV = renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();
    }
  }

  xiiUInt32 uiSampleCount = xiiGALMSAASampleCount::OneSample;
  {
    if (const xiiGALTextureView* pRTV = xiiGALDevice::GetDefaultDevice()->GetTextureView(hRTV))
    {
      uiSampleCount = pRTV->GetTexture()->GetDescription().m_uiSampleCount;
    }

    if (uiSampleCount > 1)
    {
      SetShaderPermutationVariable("MSAA", "TRUE");
    }
    else
    {
      SetShaderPermutationVariable("MSAA", "FALSE");
    }
  }

  {
    auto& gc          = WriteGlobalConstants();
    gc.ViewportSize   = xiiVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
    gc.NumMsaaSamples = uiSampleCount;
  }

  {
    m_bHasActiveScope = !sName.IsEmpty();

    if (m_bHasActiveScope)
    {
      m_pCommandList->BeginDebugGroup(sName);
    }
  }

  GetRenderPassAndFramebuffer(renderingSetup, m_hCurrentRenderPass, m_hCurrentFramebuffer);

  xiiGALViewport viewPort;
  viewPort.m_fTopLeftX = viewport.x;
  viewPort.m_fTopLeftY = viewport.y;
  viewPort.m_fWidth    = viewport.width;
  viewPort.m_fHeight   = viewport.height;
  viewPort.m_fMinDepth = 0.0f;
  viewPort.m_fMaxDepth = 0.1f;

  m_pCommandList->SetViewports(xiiMakeArrayPtr(&viewPort, 1U));
}

void xiiRenderContext::EndRendering()
{
  if (m_bNeedsClear)
  {
    BeginRenderPass();

    m_bNeedsClear = false;
  }

  EndRenderPass();

  if (m_bHasActiveScope)
  {
    m_pCommandList->EndDebugGroup();

    m_bHasActiveScope = false;
  }

  m_hCurrentFramebuffer = xiiGALFramebufferHandle();
  m_hCurrentRenderPass  = xiiGALRenderPassHandle();
  m_bStereoRendering    = false;
  m_bIsRendering        = false;

  // TODO: The render context needs to reset its state after every encoding block if we want to record to separate command buffers.
  // Although this is currently not possible since a lot of high level code binds stuff only once per frame on the render context.
  // Resetting the state after every encoding block breaks those assumptions.
  // ResetContextState();
}

void xiiRenderContext::BeginCompute(xiiStringView sName /*= {}*/)
{
  XII_ASSERT_DEV(m_bIsRendering == false && m_bIsCompute == false, "Already in a scope.");
  XII_ASSERT_DEV(m_pCommandList != nullptr, "Command list has not been set.");

  m_bIsCompute = true;
  {
    m_bHasActiveScope = !sName.IsEmpty();

    if (m_bHasActiveScope)
    {
      m_pCommandList->BeginDebugGroup(sName);
    }
  }
}

void xiiRenderContext::EndCompute()
{
  if (m_bHasActiveScope)
  {
    m_pCommandList->EndDebugGroup();

    m_bHasActiveScope = false;
  }

  m_bIsCompute = false;

  // TODO: See EndRendering
  // ResetContextState();
}

void xiiRenderContext::SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sTempValue)
{
  xiiTempHashedString sHashedName(sName);

  xiiHashedString sNameHash;
  xiiHashedString sValue;
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sHashedName, sTempValue, sNameHash, sValue))
  {
    SetShaderPermutationVariableInternal(sNameHash, sValue);
  }
}

void xiiRenderContext::SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sValue))
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
    BindTexture2D(sSlotName, xiiGALDevice::GetDefaultDevice()->GetTexture(pTexture->GetGALTexture())->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    BindSampler(sSlotName, pTexture->GetGALSampler());
  }
  else
  {
    BindTexture2D(sSlotName, xiiGALTextureViewHandle());
  }
}

void xiiRenderContext::BindTexture3D(const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode /*= xiiResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    xiiResourceLock<xiiTexture3DResource> pTexture(hTexture, acquireMode);
    BindTexture3D(sSlotName, xiiGALDevice::GetDefaultDevice()->GetTexture(pTexture->GetGALTexture())->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    BindSampler(sSlotName, pTexture->GetGALSampler());
  }
  else
  {
    BindTexture3D(sSlotName, xiiGALTextureViewHandle());
  }
}

void xiiRenderContext::BindTextureCube(const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode /*= xiiResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    xiiResourceLock<xiiTextureCubeResource> pTexture(hTexture, acquireMode);
    BindTextureCube(sSlotName, xiiGALDevice::GetDefaultDevice()->GetTexture(pTexture->GetGALTexture())->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    BindSampler(sSlotName, pTexture->GetGALSampler());
  }
  else
  {
    BindTextureCube(sSlotName, xiiGALTextureViewHandle());
  }
}

void xiiRenderContext::BindTexture2D(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hResourceView)
{
  ResourceBinding* pOldResourceBinding = nullptr;
  if (m_BoundResources.TryGetValue(sSlotName.GetHash(), pOldResourceBinding))
  {
    if (pOldResourceBinding->m_Type == ResourceBinding::Texture && pOldResourceBinding->m_hTextureView == hResourceView)
      return;

    *pOldResourceBinding = ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hResourceView};
  }
  else
  {
    m_BoundResources.Insert(sSlotName.GetHash(), ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hResourceView});
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged);
}

void xiiRenderContext::BindTexture3D(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hResourceView)
{
  ResourceBinding* pOldResourceBinding = nullptr;
  if (m_BoundResources.TryGetValue(sSlotName.GetHash(), pOldResourceBinding))
  {
    if (pOldResourceBinding->m_Type == ResourceBinding::Texture && pOldResourceBinding->m_hTextureView == hResourceView)
      return;

    *pOldResourceBinding = ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hResourceView};
  }
  else
  {
    m_BoundResources.Insert(sSlotName.GetHash(), ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hResourceView});
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged);
}

void xiiRenderContext::BindTextureCube(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hResourceView)
{
  ResourceBinding* pOldResourceBinding = nullptr;
  if (m_BoundResources.TryGetValue(sSlotName.GetHash(), pOldResourceBinding))
  {
    if (pOldResourceBinding->m_Type == ResourceBinding::Texture && pOldResourceBinding->m_hTextureView == hResourceView)
      return;

    *pOldResourceBinding = ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hResourceView};
  }
  else
  {
    m_BoundResources.Insert(sSlotName.GetHash(), ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hResourceView});
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged);
}

void xiiRenderContext::BindBufferUAV(const xiiTempHashedString& sSlotName, xiiGALBufferViewHandle hUnorderedAccessView)
{
  ResourceBinding* pOldResourceBinding = nullptr;
  if (m_BoundUAVs.TryGetValue(sSlotName.GetHash(), pOldResourceBinding))
  {
    if (pOldResourceBinding->m_Type == ResourceBinding::Buffer && pOldResourceBinding->m_hBufferView == hUnorderedAccessView)
      return;

    *pOldResourceBinding = ResourceBinding{.m_Type = ResourceBinding::Buffer, .m_hBufferView = hUnorderedAccessView, .m_hTextureView = xiiGALTextureViewHandle()};
  }
  else
  {
    m_BoundUAVs.Insert(sSlotName.GetHash(), ResourceBinding{.m_Type = ResourceBinding::Buffer, .m_hBufferView = hUnorderedAccessView, .m_hTextureView = xiiGALTextureViewHandle()});
  }

  m_StateFlags.Add(xiiRenderContextFlags::UAVBindingChanged);
}

void xiiRenderContext::BindTextureUAV(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hUnorderedAccessView)
{
  ResourceBinding* pOldResourceBinding = nullptr;
  if (m_BoundUAVs.TryGetValue(sSlotName.GetHash(), pOldResourceBinding))
  {
    if (pOldResourceBinding->m_Type == ResourceBinding::Texture && pOldResourceBinding->m_hTextureView == hUnorderedAccessView)
      return;

    *pOldResourceBinding = ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hUnorderedAccessView};
  }
  else
  {
    m_BoundUAVs.Insert(sSlotName.GetHash(), ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hUnorderedAccessView});
  }

  m_StateFlags.Add(xiiRenderContextFlags::UAVBindingChanged);
}


void xiiRenderContext::BindSampler(const xiiTempHashedString& sSlotName, xiiGALSamplerHandle hSamplerSate)
{
  XII_ASSERT_DEBUG(sSlotName != "LinearSampler", "'LinearSampler' is a resevered sampler name and must not be set manually.");
  XII_ASSERT_DEBUG(sSlotName != "LinearClampSampler", "'LinearClampSampler' is a resevered sampler name and must not be set manually.");
  XII_ASSERT_DEBUG(sSlotName != "PointSampler", "'PointSampler' is a resevered sampler name and must not be set manually.");
  XII_ASSERT_DEBUG(sSlotName != "PointClampSampler", "'PointClampSampler' is a resevered sampler name and must not be set manually.");

  xiiGALSamplerHandle* pOldSampler = nullptr;
  if (m_BoundSamplers.TryGetValue(sSlotName.GetHash(), pOldSampler))
  {
    if (*pOldSampler == hSamplerSate)
      return;

    *pOldSampler = hSamplerSate;
  }
  else
  {
    m_BoundSamplers.Insert(sSlotName.GetHash(), hSamplerSate);
  }

  m_StateFlags.Add(xiiRenderContextFlags::SamplerBindingChanged);
}

void xiiRenderContext::BindBuffer(const xiiTempHashedString& sSlotName, xiiGALBufferViewHandle hResourceView)
{
  ResourceBinding* pOldResourceBinding = nullptr;
  if (m_BoundResources.TryGetValue(sSlotName.GetHash(), pOldResourceBinding))
  {
    if (pOldResourceBinding->m_Type == ResourceBinding::Buffer && pOldResourceBinding->m_hBufferView == hResourceView)
      return;

    *pOldResourceBinding = ResourceBinding{.m_Type = ResourceBinding::Buffer, .m_hBufferView = hResourceView, .m_hTextureView = xiiGALTextureViewHandle()};
  }
  else
  {
    m_BoundResources.Insert(sSlotName.GetHash(), ResourceBinding{.m_Type = ResourceBinding::Buffer, .m_hBufferView = hResourceView, .m_hTextureView = xiiGALTextureViewHandle()});
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
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetInputLayout()), pMeshBuffer->GetTopology(), pMeshBuffer->GetPrimitiveCount());
}

void xiiRenderContext::BindMeshBuffer(xiiGALBufferHandle hVertexBuffer, xiiGALBufferHandle hIndexBuffer, const xiiInputLayoutInfo* pInputLayoutInfo, xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount, xiiGALBufferHandle hVertexBuffer2, xiiGALBufferHandle hVertexBuffer3, xiiGALBufferHandle hVertexBuffer4)
{
  if (m_hVertexBuffers[0] == hVertexBuffer && m_hVertexBuffers[1] == hVertexBuffer2 && m_hVertexBuffers[2] == hVertexBuffer3 && m_hVertexBuffers[3] == hVertexBuffer4 && m_hIndexBuffer == hIndexBuffer && m_pInputLayoutInfo == pInputLayoutInfo && m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
    return;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (pInputLayoutInfo)
  {
    for (xiiUInt32 i1 = 0; i1 < pInputLayoutInfo->m_VertexStreams.GetCount(); ++i1)
    {
      for (xiiUInt32 i2 = 0; i2 < pInputLayoutInfo->m_VertexStreams.GetCount(); ++i2)
      {
        if (i1 != i2)
        {
          XII_ASSERT_DEBUG(pInputLayoutInfo->m_VertexStreams[i1].m_Semantic != pInputLayoutInfo->m_VertexStreams[i2].m_Semantic, "Same semantic cannot be used twice in the same input layout");
        }
      }
    }
  }
#endif

  if (m_Topology != topology)
  {
    m_Topology = topology;

    static bool                bInitialized                                     = false;
    static xiiTempHashedString sTopologies[xiiGALPrimitiveTopology::ENUM_COUNT] = {};
    if (!bInitialized)
    {
      sTopologies[xiiGALPrimitiveTopology::PointList]               = xiiTempHashedString("TOPOLOGY_POINT_LIST");
      sTopologies[xiiGALPrimitiveTopology::LineList]                = xiiTempHashedString("TOPOLOGY_LINE_LIST");
      sTopologies[xiiGALPrimitiveTopology::TriangleList]            = xiiTempHashedString("TOPOLOGY_TRIANGLE_LIST");
      sTopologies[xiiGALPrimitiveTopology::TriangleStrip]           = xiiTempHashedString("TOPOLOGY_TRIANGLE_STRIP");
      sTopologies[xiiGALPrimitiveTopology::LineStrip]               = xiiTempHashedString("TOPOLOGY_LINE_STRIP");
      sTopologies[xiiGALPrimitiveTopology::TriangleListAdjacent]    = xiiTempHashedString("TOPOLOGY_TRIANGLE_LIST_ADJACENT");
      sTopologies[xiiGALPrimitiveTopology::TriangleStripAdjacent]   = xiiTempHashedString("TOPOLOGY_TRIANGLE_STRIP_ADJACENT");
      sTopologies[xiiGALPrimitiveTopology::LineListAdjacent]        = xiiTempHashedString("TOPOLOGY_LINE_LIST_ADJACENT");
      sTopologies[xiiGALPrimitiveTopology::LineStripAdjacent]       = xiiTempHashedString("TOPOLOGY_LINE_STRIP_ADJACENT");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList1]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_1");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList2]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_2");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList3]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_3");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList4]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_4");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList5]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_5");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList6]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_6");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList7]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_7");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList8]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_8");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList9]  = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_9");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList10] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_10");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList11] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_11");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList12] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_12");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList13] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_13");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList14] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_14");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList15] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_15");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList16] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_16");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList17] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_17");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList18] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_18");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList19] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_19");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList20] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_20");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList21] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_21");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList22] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_22");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList23] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_23");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList24] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_24");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList25] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_25");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList26] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_26");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList27] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_27");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList28] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_28");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList29] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_29");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList30] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_30");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList31] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_31");
      sTopologies[xiiGALPrimitiveTopology::ControlPointPatchList32] = xiiTempHashedString("TOPOLOGY_CONTROL_POINT_PATCH_LIST_32");
      bInitialized                                                  = true;
    }

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_Topology]);
  }

  m_hVertexBuffers[0]          = hVertexBuffer;
  m_hVertexBuffers[1]          = hVertexBuffer2;
  m_hVertexBuffers[2]          = hVertexBuffer3;
  m_hVertexBuffers[3]          = hVertexBuffer4;
  m_hIndexBuffer               = hIndexBuffer;
  m_pInputLayoutInfo           = pInputLayoutInfo;
  m_hInputLayout               = xiiGALInputLayoutHandle();
  m_uiMeshBufferPrimitiveCount = uiPrimitiveCount;

  m_StateFlags.Add(xiiRenderContextFlags::MeshBufferBindingChanged);
}

void xiiRenderContext::BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer)
{
  xiiResourceLock<xiiDynamicMeshBufferResource> pMeshBuffer(hDynamicMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetInputLayout()), pMeshBuffer->GetDescriptor().m_Topology, pMeshBuffer->GetDescriptor().m_uiMaxPrimitives, pMeshBuffer->GetColorBuffer());
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

  auto pCommandList = GetCommandList();

  xiiGALPipelineState* pPipelineState = xiiGALDevice::GetDefaultDevice()->GetPipelineState(m_hCurrentPipelineState);

  const xiiUInt32 uiVertsPerPrimitive = xiiGALPrimitiveTopology::VerticesPerPrimitive(pPipelineState->GetDescription().m_GraphicsPipeline.m_PrimitiveTopology);

  uiPrimitiveCount *= uiVertsPerPrimitive;
  uiFirstPrimitive *= uiVertsPerPrimitive;
  if (m_bStereoRendering)
  {
    uiInstanceCount *= 2;
  }

  XII_SUCCEED_OR_RETURN(pCommandList->CommitShaderResources());

  BeginRenderPass();
  XII_SCOPE_EXIT(EndRenderPass());

  if (uiInstanceCount > 1)
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      return pCommandList->DrawIndexedInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
    else
    {
      return pCommandList->DrawInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
  }
  else
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      return pCommandList->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      return pCommandList->Draw(uiPrimitiveCount, uiFirstPrimitive);
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

  return GetCommandList()->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
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

  bool bRebuildInputLayout = m_StateFlags.IsAnySet(xiiRenderContextFlags::ShaderStateChanged | xiiRenderContextFlags::MeshBufferBindingChanged);

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
    bool bIsModified = (bForce || m_StateFlags.IsAnySet(xiiRenderContextFlags::TextureBindingChanged | xiiRenderContextFlags::UAVBindingChanged | xiiRenderContextFlags::SamplerBindingChanged | xiiRenderContextFlags::BufferBindingChanged | xiiRenderContextFlags::ConstantBufferBindingChanged));

    if (bIsModified)
    {
      if (pShaderPermutation == nullptr)
      {
        pShaderPermutation = xiiResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
      }
      if (pShaderPermutation == nullptr)
      {
        return XII_FAILURE;
      }
    }

    xiiLogBlock applyBindingsBlock("Applying Shader Bindings", pShaderPermutation != nullptr ? pShaderPermutation->GetResourceDescription().GetData() : "");

    if ((bForce || bRebuildInputLayout) && m_bIsRendering)
    {
      if (m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Vertex)].IsInvalidated())
        return XII_FAILURE;

      auto pCommandList = GetCommandList();

      if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::MeshBufferBindingChanged))
      {
        pCommandList->SetVertexBuffers(0, xiiMakeArrayPtr(&m_hVertexBuffers[0], XII_ARRAY_SIZE(m_hVertexBuffers)), xiiArrayPtr<xiiUInt64>());

        if (!m_hIndexBuffer.IsInvalidated())
          pCommandList->SetIndexBuffer(m_hIndexBuffer);
      }

      if (m_pInputLayoutInfo != nullptr && BuildInputLayout(m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Vertex)], *m_pInputLayoutInfo, m_hInputLayout).Failed())
        return XII_FAILURE;

      // If there is a vertex buffer we need a valid vertex declaration as well.
      if ((!m_hVertexBuffers[0].IsInvalidated() || !m_hVertexBuffers[1].IsInvalidated() || !m_hVertexBuffers[2].IsInvalidated() || !m_hVertexBuffers[3].IsInvalidated()) && m_hInputLayout.IsInvalidated())
        return XII_FAILURE;

      m_StateFlags.Remove(xiiRenderContextFlags::MeshBufferBindingChanged);
    }

    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

    bool bPipelineStateInvalidated = false;
    if (pShaderPermutation != nullptr)
    {
      // Set render state from shader.
      // Create pipeline state that is valid for this scope.
      xiiGALPipelineStateCreationDescription pipelineDescription;
      pipelineDescription.m_PipelineType               = m_bIsCompute ? xiiGALPipelineType::Compute : xiiGALPipelineType::Graphics;
      pipelineDescription.m_hPipelineResourceSignature = (pShaderPermutation != nullptr) ? pShaderPermutation->GetPipelineResourceSignature() : xiiGALPipelineResourceSignatureHandle();

      if (pipelineDescription.IsAnyGraphicsPipeline())
      {
        pipelineDescription.m_GraphicsPipeline.m_hRenderPass          = m_hCurrentRenderPass;
        pipelineDescription.m_GraphicsPipeline.m_hVertexShader        = m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Vertex)];
        pipelineDescription.m_GraphicsPipeline.m_hPixelShader         = m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Pixel)];
        pipelineDescription.m_GraphicsPipeline.m_hDomainShader        = m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Domain)];
        pipelineDescription.m_GraphicsPipeline.m_hHullShader          = m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Hull)];
        pipelineDescription.m_GraphicsPipeline.m_hGeometryShader      = m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Geometry)];
        pipelineDescription.m_GraphicsPipeline.m_hAmplificationShader = m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Amplification)];
        pipelineDescription.m_GraphicsPipeline.m_hMeshShader          = m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Mesh)];

        auto& graphicsPipeline               = pipelineDescription.m_GraphicsPipeline;
        graphicsPipeline.m_PrimitiveTopology = m_Topology;
        graphicsPipeline.m_hInputLayout      = m_hInputLayout;

        if (pShaderPermutation != nullptr)
        {
          if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoBlendState))
            graphicsPipeline.m_hBlendState = pShaderPermutation->GetBlendState();

          if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoRasterizerState))
            graphicsPipeline.m_hRasterizerState = pShaderPermutation->GetRasterizerState();

          if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoDepthStencilState))
            graphicsPipeline.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
        }
      }
      else if (pipelineDescription.IsComputePipeline())
      {
        pipelineDescription.m_ComputePipeline.m_hComputeShader = m_hActiveGALShaders[xiiGALShaderType::GetStageIndex(xiiGALShaderType::Compute)];
      }

      xiiRenderContext::PipelineStateInfo* pPipelineStateInfo = nullptr;
      if (!m_PipelineStateCache.TryGetValue(pipelineDescription, pPipelineStateInfo))
      {
        m_hCurrentPipelineState = pDevice->CreatePipelineState(pipelineDescription);

        xiiRenderContext::PipelineStateInfo newPipelineStateInfo = {.m_hPipelineState = m_hCurrentPipelineState};

        XII_VERIFY(!m_PipelineStateCache.Insert(pipelineDescription, newPipelineStateInfo), "Overwriting an existing cached pipeline state, this is unexpected behavior.");

        m_pCommandList->SetPipelineState(m_hCurrentPipelineState);

        bPipelineStateInvalidated = true;
      }
      else
      {
        if (m_hCurrentPipelineState != pPipelineStateInfo->m_hPipelineState || m_pCommandList->GetPipelineState() != pPipelineStateInfo->m_hPipelineState)
        {
          m_hCurrentPipelineState = pPipelineStateInfo->m_hPipelineState;

          m_pCommandList->SetPipelineState(m_hCurrentPipelineState);

          bPipelineStateInvalidated = true;
        }
      }

      XII_ASSERT_DEV(!m_hCurrentPipelineState.IsInvalidated(), "Implementation error!");
    }

    if (bIsModified || bPipelineStateInvalidated)
    {
      if (bPipelineStateInvalidated)
      {
        if (xiiGALPipelineState* pPipelineState = pDevice->GetPipelineState(m_hCurrentPipelineState))
        {
          if (pPipelineState->GetDescription().IsAnyGraphicsPipeline() && !pPipelineState->GetDescription().m_GraphicsPipeline.m_hRasterizerState.IsInvalidated())
          {
            xiiGALRasterizerState* pRasterizerState = pDevice->GetRasterizerState(pPipelineState->GetDescription().m_GraphicsPipeline.m_hRasterizerState);

            if (pRasterizerState->GetDescription().m_bScissorEnable)
            {
              const auto& framebufferDescription = pDevice->GetFramebuffer(m_hCurrentFramebuffer)->GetDescription();
              auto        scissorRect            = xiiRectU32(framebufferDescription.m_FramebufferSize.width, framebufferDescription.m_FramebufferSize.height);

              m_pCommandList->SetScissorRects(xiiMakeArrayPtr(&scissorRect, 1U));
            }
          }
        }
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::UAVBindingChanged))
      {
        ApplyUnorderedAccessViewBindings();
        m_StateFlags.Remove(xiiRenderContextFlags::UAVBindingChanged);
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::TextureBindingChanged))
      {
        ApplyResourceViewBindings(xiiGALShaderResourceType::TextureSRV);
        m_StateFlags.Remove(xiiRenderContextFlags::TextureBindingChanged);
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::SamplerBindingChanged))
      {
        ApplySamplerBindings();
        m_StateFlags.Remove(xiiRenderContextFlags::SamplerBindingChanged);
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::BufferBindingChanged))
      {
        ApplyResourceViewBindings(xiiGALShaderResourceType::BufferSRV);
        m_StateFlags.Remove(xiiRenderContextFlags::BufferBindingChanged);
      }
    }

    // Note that pMaterial is only valid, if material constants have changed, so this also always implies that ConstantBufferBindingChanged is set.
    if (pMaterial != nullptr)
    {
      pMaterial->UpdateConstantBuffer(pShaderPermutation);
      BindConstantBuffer("xiiMaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    UploadConstants();

    if (bIsModified || bPipelineStateInvalidated)
    {
      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::ConstantBufferBindingChanged))
      {
        ApplyConstantBufferBindings();
        m_StateFlags.Remove(xiiRenderContextFlags::ConstantBufferBindingChanged);
      }
    }
  }

  return XII_SUCCESS;
}

void xiiRenderContext::ResetContextState()
{
  m_StateFlags = xiiRenderContextFlags::AllStatesInvalid;

  m_CurrentRenderingSetup = {};
  m_hCurrentFramebuffer   = xiiGALFramebufferHandle();
  m_hCurrentRenderPass    = xiiGALRenderPassHandle();

  m_hActiveShader.Invalidate();
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hActiveGALShaders); ++i)
  {
    m_hActiveGALShaders[i].Invalidate();
  }

  m_PermutationVariables.Clear();
  m_hNewMaterial.Invalidate();
  m_hMaterial.Invalidate();

  m_hActiveShaderPermutation.Invalidate();

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }

  m_hIndexBuffer.Invalidate();
  m_pInputLayoutInfo           = nullptr;
  m_hInputLayout               = xiiGALInputLayoutHandle();
  m_Topology                   = xiiGALPrimitiveTopology::Undefined;
  m_uiMeshBufferPrimitiveCount = 0;

  m_BoundResources.Clear();

  m_BoundSamplers.Clear();
  m_BoundSamplers.Insert(xiiHashingUtils::StringHash("LinearSampler"), GetDefaultSampler(xiiDefaultSamplerFlags::LinearFiltering));
  m_BoundSamplers.Insert(xiiHashingUtils::StringHash("LinearClampSampler"), GetDefaultSampler(xiiDefaultSamplerFlags::LinearFiltering | xiiDefaultSamplerFlags::Clamp));
  m_BoundSamplers.Insert(xiiHashingUtils::StringHash("PointSampler"), GetDefaultSampler(xiiDefaultSamplerFlags::PointFiltering));
  m_BoundSamplers.Insert(xiiHashingUtils::StringHash("PointClampSampler"), GetDefaultSampler(xiiDefaultSamplerFlags::PointFiltering | xiiDefaultSamplerFlags::Clamp));

  m_BoundUAVs.Clear();

  m_BoundConstantBuffers.Clear();

  m_hCurrentPipelineState = xiiGALPipelineStateHandle();
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
xiiConstantBufferStorageHandle xiiRenderContext::CreateConstantBufferStorage(xiiUInt32 uiSizeInBytes, xiiConstantBufferStorageBase*& out_pStorage)
{
  XII_ASSERT_DEV(xiiMemoryUtils::IsSizeAligned(uiSizeInBytes, 16U), "Storage struct for constant buffer is not aligned to 16 bytes.");

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
    pStorage = XII_DEFAULT_NEW(xiiConstantBufferStorageBase, uiSizeInBytes);
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
xiiGALSamplerHandle xiiRenderContext::GetDefaultSampler(xiiBitflags<xiiDefaultSamplerFlags> flags)
{
  xiiUInt32 uiSamplerIndex = flags.GetValue();
  XII_ASSERT_DEV(uiSamplerIndex < XII_ARRAY_SIZE(s_hDefaultSamplers), "");

  if (s_hDefaultSamplers[uiSamplerIndex].IsInvalidated())
  {
    xiiGALSamplerCreationDescription desc;
    desc.m_ComparisonFunction = xiiGALComparisonFunction::Never;
    desc.m_BorderColor        = xiiColor::Black;
    desc.m_fMipLODBias        = 0.0f;
    desc.m_fMinLOD            = -1.0f;
    desc.m_fMaxLOD            = 42000.0f;
    desc.m_uiMaxAnisotropy    = 4U;

    desc.m_MinFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;
    desc.m_MagFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;
    desc.m_MipFilter = flags.IsSet(xiiDefaultSamplerFlags::LinearFiltering) ? xiiGALFilterType::Linear : xiiGALFilterType::Point;

    desc.m_AddressU = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);
    desc.m_AddressV = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);
    desc.m_AddressW = flags.IsSet(xiiDefaultSamplerFlags::Clamp) ? xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp) : xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Repeat);

    s_hDefaultSamplers[uiSamplerIndex] = xiiGALDevice::GetDefaultDevice()->CreateSampler(desc);
  }

  return s_hDefaultSamplers[uiSamplerIndex];
}

// private functions
//////////////////////////////////////////////////////////////////////////

// static
void xiiRenderContext::LoadBuiltinShader(xiiShaderUtilities::xiiBuiltinShaderType type, xiiShaderUtilities::xiiBuiltinShader& out_shader)
{
  xiiShaderResourceHandle hActiveShader;
  bool                    bStereo = false;
  switch (type)
  {
    case xiiShaderUtilities::xiiBuiltinShaderType::CopyImageArray:
      bStereo = true;
      [[fallthrough]];
    case xiiShaderUtilities::xiiBuiltinShaderType::CopyImage:
      hActiveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/Copy.xiiShader");
      break;
    case xiiShaderUtilities::xiiBuiltinShaderType::DownscaleImageArray:
      bStereo = true;
      [[fallthrough]];
    case xiiShaderUtilities::xiiBuiltinShaderType::DownscaleImage:
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

  if (xiiGALDevice::GetDefaultDevice()->GetFeatures().m_VertexShaderRenderTargetArrayIndex == xiiGALDeviceFeatureState::Enabled)
    permutationVariables.Insert(sVSRTAI, sTrue);
  else
    permutationVariables.Insert(sVSRTAI, sFalse);

  xiiShaderPermutationResourceHandle hActiveShaderPermutation = xiiGALShaderPermutationUtilities::PreloadSinglePermutation(hActiveShader, permutationVariables, false);

  XII_ASSERT_DEV(hActiveShaderPermutation.IsValid(), "Could not load builtin shader permutation!");

  xiiResourceLock<xiiShaderPermutationResource> pShaderPermutation(hActiveShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);

  XII_ASSERT_DEV(pShaderPermutation->IsShaderValid(), "Builtin shader permutation shader is invalid!");

  xiiStaticBitfield32 shaderBitfield = xiiStaticBitfield32::MakeFromMask(pShaderPermutation->GetActiveShaderStages().GetValue());
  for (xiiUInt32 uiStageBitIndex : shaderBitfield)
  {
    out_shader.m_hActiveGALShaders[uiStageBitIndex] = pShaderPermutation->GetGALShader(xiiGALShaderType::GetStageFlag(uiStageBitIndex));

    XII_ASSERT_DEV(!out_shader.m_hActiveGALShaders[uiStageBitIndex].IsInvalidated(), "Invalid GAL {} Shader handle.", xiiGALShaderType::Names[uiStageBitIndex]);
  }

  out_shader.m_hBlendState        = pShaderPermutation->GetBlendState();
  out_shader.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
  out_shader.m_hRasterizerState   = pShaderPermutation->GetRasterizerState();
}

// static
void xiiRenderContext::OnEngineShutdown()
{
  xiiGALShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    XII_DEFAULT_DELETE(rc);

  s_Instances.Clear();

  // Cleanup sampler states
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_hDefaultSamplers); ++i)
  {
    if (!s_hDefaultSamplers[i].IsInvalidated())
    {
      xiiGALDevice::GetDefaultDevice()->DestroySampler(s_hDefaultSamplers[i]);
      s_hDefaultSamplers[i].Invalidate();
    }
  }

  // Cleanup input layouts
  {
    for (auto it = s_GALInputLayouts.GetIterator(); it.IsValid(); ++it)
    {
      xiiGALDevice::GetDefaultDevice()->DestroyInputLayout(it.Value());
    }

    s_GALInputLayouts.Clear();
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

void xiiRenderContext::GetRenderPassAndFramebuffer(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPassHandle& out_hRenderPass, xiiGALFramebufferHandle& out_hFramebuffer)
{
  // XII_ASSERT_DEV(out_hRenderPass.IsInvalidated() && out_hFramebuffer.IsInvalidated(), "Render pass and frame buffer are still active.");

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  RenderPassFrameBufferInfo frameBufferInfo;
  if (!m_FramebufferCache.TryGetValue(renderingSetup, frameBufferInfo))
  {
    // Retrieve an existing render pass handle if any, a new one is created otherwise.

    xiiGALRenderPassHandle hRenderPass;
    if (!m_RenderPassCache.TryGetValue(renderingSetup, hRenderPass))
    {
      xiiGALRenderPassCreationDescription renderPassDescription;

      const bool      bHasDepthAttachment    = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
      const xiiUInt32 uiColorAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

      // Build render pass description.
      {
        if (bHasDepthAttachment)
        {
          xiiGALTexture*                          pDepthTexture      = pDevice->GetTextureView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget())->GetTexture();
          const xiiGALTextureCreationDescription& textureDescription = pDepthTexture->GetDescription();

          xiiGALRenderPassAttachmentDescription& attachmentReference = renderPassDescription.m_Attachments.ExpandAndGetRef();
          attachmentReference.m_Format                               = textureDescription.m_Format;
          attachmentReference.m_uiSampleCount                        = static_cast<xiiUInt8>(textureDescription.m_uiSampleCount);
          attachmentReference.m_InitialStateFlags                    = xiiGALResourceStateFlags::DepthWrite;
          attachmentReference.m_FinalStateFlags                      = xiiGALResourceStateFlags::DepthWrite;

          if (renderingSetup.m_bDiscardDepth)
          {
            attachmentReference.m_LoadOperation = xiiGALAttachmentLoadOperation::Discard;
          }
          else
          {
            attachmentReference.m_LoadOperation = renderingSetup.m_bClearDepth ? xiiGALAttachmentLoadOperation::Clear : xiiGALAttachmentLoadOperation::Load;
          }
          attachmentReference.m_StoreOperation = xiiGALAttachmentStoreOperation::Store;

          if (textureDescription.m_Format.IsStencilFormat(textureDescription.m_Format))
          {
            attachmentReference.m_StencilLoadOperation  = renderingSetup.m_bClearStencil ? xiiGALAttachmentLoadOperation::Clear : xiiGALAttachmentLoadOperation::Load;
            attachmentReference.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Store;
          }
          else
          {
            attachmentReference.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Discard;
            attachmentReference.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Discard;
          }
        }

        for (xiiUInt32 i = 0; i < uiColorAttachmentCount; ++i)
        {
          xiiGALTexture*                          pColourTexture     = pDevice->GetTextureView(renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i)))->GetTexture();
          const xiiGALTextureCreationDescription& textureDescription = pColourTexture->GetDescription();

          xiiGALRenderPassAttachmentDescription& attachmentReference = renderPassDescription.m_Attachments.ExpandAndGetRef();
          attachmentReference.m_Format                               = textureDescription.m_Format;
          attachmentReference.m_uiSampleCount                        = static_cast<xiiUInt8>(textureDescription.m_uiSampleCount);
          attachmentReference.m_InitialStateFlags                    = xiiGALResourceStateFlags::RenderTarget;
          attachmentReference.m_FinalStateFlags                      = xiiGALResourceStateFlags::RenderTarget;

          if (renderingSetup.m_bDiscardColor)
          {
            attachmentReference.m_LoadOperation = xiiGALAttachmentLoadOperation::Discard;
          }
          else
          {
            if (renderingSetup.m_uiRenderTargetClearMask & XII_BIT(i))
            {
              attachmentReference.m_LoadOperation = xiiGALAttachmentLoadOperation::Clear;
            }
            else
            {
              attachmentReference.m_LoadOperation = xiiGALAttachmentLoadOperation::Load;
            }
          }

          attachmentReference.m_StoreOperation        = xiiGALAttachmentStoreOperation::Store;
          attachmentReference.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Discard;
          attachmentReference.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Discard;
        }
      }

      // Build render pass attachment description.
      {
        xiiHybridArray<xiiGALAttachmentReferenceDescription, 1U> depthAttachmentRefs;
        xiiHybridArray<xiiGALAttachmentReferenceDescription, 4U> colorAttachmentRefs;

        const xiiUInt32 uiAttachmentCount = renderPassDescription.m_Attachments.GetCount();
        for (xiiUInt32 i = 0; i < uiAttachmentCount; ++i)
        {
          auto& attachment = renderPassDescription.m_Attachments[i];

          const bool bIsDepthAttachment = xiiGALResourceFormat::IsDepthFormat(attachment.m_Format);
          if (bIsDepthAttachment)
          {
            attachment.m_FinalStateFlags = xiiGALResourceStateFlags::DepthWrite;

            xiiGALAttachmentReferenceDescription& attachmentRef = depthAttachmentRefs.ExpandAndGetRef();
            attachmentRef.m_uiAttachmentIndex                   = i;
            attachmentRef.m_ResourceStateFlags                  = xiiGALResourceStateFlags::DepthWrite;
          }
          else
          {
            attachment.m_FinalStateFlags = xiiGALResourceStateFlags::RenderTarget;

            xiiGALAttachmentReferenceDescription& attachmentRef = colorAttachmentRefs.ExpandAndGetRef();
            attachmentRef.m_uiAttachmentIndex                   = i;
            attachmentRef.m_ResourceStateFlags                  = xiiGALResourceStateFlags::RenderTarget;
          }
        }

        XII_ASSERT_DEV(depthAttachmentRefs.GetCount() <= 1U, "There can only be a maximum of 1 bound depth attachment.");

        xiiGALSubPassDescription& subpassDescription = renderPassDescription.m_SubPasses.ExpandAndGetRef();
        subpassDescription.m_RenderTargetAttachments = colorAttachmentRefs;
        subpassDescription.m_DepthStencilAttachment  = depthAttachmentRefs;

        xiiGALSubPassDependencyDescription& subpassDependency = renderPassDescription.m_Dependencies.ExpandAndGetRef();
        subpassDependency.m_uiSourceSubPass                   = XII_GAL_SUBPASS_EXTERNAL;
        subpassDependency.m_uiDestinationSubPass              = 0;
        subpassDependency.m_SourceStageFlags                  = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
        subpassDependency.m_DestinationStageFlags             = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;

        if (!depthAttachmentRefs.IsEmpty())
          subpassDependency.m_DestinationAccessFlags |= xiiGALAccessFlags::DepthStencilWrite | xiiGALAccessFlags::DepthStencilRead;

        if (!colorAttachmentRefs.IsEmpty())
          subpassDependency.m_DestinationAccessFlags |= xiiGALAccessFlags::RenderTargetWrite | xiiGALAccessFlags::RenderTargetRead;
      }

      hRenderPass = pDevice->CreateRenderPass(renderPassDescription);
      XII_VERIFY(!m_RenderPassCache.Insert(renderingSetup, hRenderPass), "Overwrote existing render pass, this is unexpected behaviour.");
    }

    XII_ASSERT_DEV(!hRenderPass.IsInvalidated(), "Render pass handle is invalidated!");

    // Since no framebuffer was retrieved, a new one needs to be created.
    {
      xiiGALFramebufferCreationDescription frameBufferDescription;
      frameBufferDescription.m_hRenderPass = hRenderPass;

      const bool      bHasDepthAttachment    = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
      const xiiUInt32 uiColorAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

      if (bHasDepthAttachment)
      {
        xiiGALTexture*                          pDepthTexture      = pDevice->GetTextureView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget())->GetTexture();
        const xiiGALTextureCreationDescription& textureDescription = pDepthTexture->GetDescription();

        xiiVec3U32 size                                 = xiiGALTextureUtilities::GetMipLevelSize(pDevice->GetTextureView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget())->GetDescription().m_uiMostDetailedMip, textureDescription);
        frameBufferDescription.m_FramebufferSize.width  = size.x;
        frameBufferDescription.m_FramebufferSize.height = size.y;
        frameBufferDescription.m_uiArraySliceCount      = textureDescription.m_uiArraySizeOrDepth;

        frameBufferDescription.m_Attachments.PushBack(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget());
      }

      for (xiiUInt32 i = 0; i < uiColorAttachmentCount; ++i)
      {
        xiiGALTexture*                          pColourTexture     = pDevice->GetTextureView(renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i)))->GetTexture();
        const xiiGALTextureCreationDescription& textureDescription = pColourTexture->GetDescription();

        xiiVec3U32 size                                 = xiiGALTextureUtilities::GetMipLevelSize(pDevice->GetTextureView(renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i)))->GetDescription().m_uiMostDetailedMip, textureDescription);
        frameBufferDescription.m_FramebufferSize.width  = size.x;
        frameBufferDescription.m_FramebufferSize.height = size.y;
        frameBufferDescription.m_uiArraySliceCount      = textureDescription.m_uiArraySizeOrDepth;

        frameBufferDescription.m_Attachments.PushBack(renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<xiiUInt8>(i)));
      }

      // In some places rendering is started with an empty xiiGALRenderTargetSetup just to be able to run GPU commands.
      // An empty size is invalid in both D3D12 and Vulkan so we just set it so (1, 1).
      if (xiiVec2U32(frameBufferDescription.m_FramebufferSize.width, frameBufferDescription.m_FramebufferSize.height) == xiiVec2U32(0, 0))
      {
        frameBufferDescription.m_FramebufferSize.width  = 1U;
        frameBufferDescription.m_FramebufferSize.height = 1U;
        frameBufferDescription.m_uiArraySliceCount      = 1U;
      }

      frameBufferInfo.hRenderPass  = hRenderPass;
      frameBufferInfo.hFrameBuffer = pDevice->CreateFramebuffer(frameBufferDescription);

      XII_VERIFY(!m_FramebufferCache.Insert(renderingSetup, frameBufferInfo), "Overwrote existing frame buffer, this is unexpected behaviour.");
    }
  }

  XII_ASSERT_DEV(!frameBufferInfo.hRenderPass.IsInvalidated(), "Render Pass handle is invalidated!");
  XII_ASSERT_DEV(!frameBufferInfo.hFrameBuffer.IsInvalidated(), "Framebuffer handle is invalidated!");

  out_hRenderPass  = frameBufferInfo.hRenderPass;
  out_hFramebuffer = frameBufferInfo.hFrameBuffer;
}

void xiiRenderContext::FlushPipelineStateCache()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  for (auto iter : m_PipelineStateCache)
  {
    if (!iter.IsValid())
      continue;

    const auto& pipelineStateInfo = iter.Value();

    if (!pipelineStateInfo.m_hPipelineResourceSignature.IsInvalidated())
    {
      pDevice->DestroyPipelineResourceSignature(pipelineStateInfo.m_hPipelineResourceSignature);
    }
    if (!pipelineStateInfo.m_hPipelineState.IsInvalidated())
    {
      pDevice->DestroyPipelineState(pipelineStateInfo.m_hPipelineState);
    }
  }

  m_PipelineStateCache.Clear();
  m_PipelineStateCache.Compact();
}

void xiiRenderContext::BeginRenderPass()
{
  XII_ASSERT_DEV(!m_bIsCompute && m_bIsRendering, "Cannot begin render pass while compute pipeline is active!");

  const bool     bHasDepthAttachment    = !m_CurrentRenderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const xiiUInt8 uiColorAttachmentCount = m_CurrentRenderingSetup.m_RenderTargetSetup.GetRenderTargetCount();

  if (!m_bRenderPassActive && (bHasDepthAttachment || uiColorAttachmentCount > 0))
  {
    if (!m_bNeedsClear)
    {
      xiiGALRenderingSetup renderingSetup      = m_CurrentRenderingSetup;
      renderingSetup.m_bClearDepth             = false;
      renderingSetup.m_bClearStencil           = false;
      renderingSetup.m_uiRenderTargetClearMask = 0x00U;
      renderingSetup.m_bDiscardColor           = false;
      renderingSetup.m_bDiscardDepth           = false;

      GetRenderPassAndFramebuffer(renderingSetup, m_hCurrentRenderPass, m_hCurrentFramebuffer);
    }

    xiiGALBeginRenderPassDescription beginRenderPassDescription;
    beginRenderPassDescription.m_hFramebuffer = m_hCurrentFramebuffer;
    beginRenderPassDescription.m_hRenderPass  = m_hCurrentRenderPass;

    if (bHasDepthAttachment)
    {
      xiiGALOptimizedClearValue& depthClearValue = beginRenderPassDescription.m_ClearValues.ExpandAndGetRef();
      depthClearValue.m_DepthStencil.m_fDepth    = 1.0f;
      depthClearValue.m_DepthStencil.m_uiStencil = 0U;
    }
    for (xiiUInt8 i = 0; i < uiColorAttachmentCount; ++i)
    {
      xiiGALOptimizedClearValue& colorClearValue = beginRenderPassDescription.m_ClearValues.ExpandAndGetRef();
      colorClearValue.m_ClearColor               = m_CurrentRenderingSetup.m_ClearColor;
    }

    GetCommandList()->BeginRenderPass(beginRenderPassDescription);

    m_bRenderPassActive = true;
    m_bNeedsClear       = false;
  }
}

void xiiRenderContext::EndRenderPass()
{
  if (m_bRenderPassActive)
  {
    GetCommandList()->EndRenderPass();

    m_bRenderPassActive = false;
  }
}

xiiResult xiiRenderContext::BuildInputLayout(xiiGALShaderHandle hVertexShader, const xiiInputLayoutInfo& decl, xiiGALInputLayoutHandle& out_Declaration)
{
  ShaderVertexDecl svd;
  svd.m_hShader           = hVertexShader;
  svd.m_uiInputLayoutHash = decl.m_uiHash;

  bool bExisted = false;
  auto it       = s_GALInputLayouts.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    xiiGALDevice*       pDevice = xiiGALDevice::GetDefaultDevice();
    const xiiGALShader* pShader = pDevice->GetShader(hVertexShader);

    xiiGALInputLayoutCreationDescription vd;
    vd.m_hVertexShader = hVertexShader;

    for (xiiUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
    {
      auto& stream = decl.m_VertexStreams[slot];

      // stream.m_Format
      xiiGALLayoutElement gal;
      gal.m_Format                 = stream.m_Format;
      gal.m_Semantic               = stream.m_Semantic;
      gal.m_uiRelativeOffset       = stream.m_uiOffset;
      gal.m_uiStride               = pDevice->GetBuffer(m_hVertexBuffers[stream.m_uiVertexBufferSlot])->GetDescription().m_uiElementByteStride;
      gal.m_uiBufferSlot           = stream.m_uiVertexBufferSlot;
      gal.m_Frequency              = xiiGALInputElementFrequency::PerVertex;
      gal.m_uiInstanceDataStepRate = 0;
      vd.m_LayoutElements.PushBack(gal);
    }

    out_Declaration = pDevice->CreateInputLayout(vd);

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

      xiiLog::Warning("Failed to create input layout.");
      return XII_FAILURE;
    }

    it.Value() = out_Declaration;
  }

  out_Declaration = it.Value();
  return XII_SUCCESS;
}

void xiiRenderContext::UploadConstants()
{
  BindConstantBuffer("xiiGlobalConstants", m_hGlobalConstantBufferStorage);

  for (auto it = m_BoundConstantBuffers.GetIterator(); it.IsValid(); ++it)
  {
    xiiConstantBufferStorageHandle hConstantBufferStorage = it.Value().m_hConstantBufferStorage;
    xiiConstantBufferStorageBase*  pConstantBufferStorage = nullptr;
    if (TryGetConstantBufferStorage(hConstantBufferStorage, pConstantBufferStorage))
    {
      pConstantBufferStorage->UploadData(m_pCommandList);
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
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hActiveGALShaders); ++i)
  {
    m_hActiveGALShaders[i].Invalidate();
  }

  m_StateFlags.Add(xiiRenderContextFlags::TextureBindingChanged | xiiRenderContextFlags::SamplerBindingChanged | xiiRenderContextFlags::BufferBindingChanged | xiiRenderContextFlags::ConstantBufferBindingChanged);

  if (!m_hActiveShader.IsValid())
    return nullptr;

  m_hActiveShaderPermutation = xiiGALShaderPermutationUtilities::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, m_bAllowAsyncShaderLoading);

  if (!m_hActiveShaderPermutation.IsValid())
    return nullptr;

  xiiShaderPermutationResource* pShaderPermutation = xiiResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, m_bAllowAsyncShaderLoading ? xiiResourceAcquireMode::AllowLoadingFallback : xiiResourceAcquireMode::BlockTillLoaded);

  if (!pShaderPermutation->IsShaderValid())
  {
    xiiResourceManager::EndAcquireResource(pShaderPermutation);
    return nullptr;
  }

  xiiStaticBitfield32 shaderBitfield = xiiStaticBitfield32::MakeFromMask(pShaderPermutation->GetActiveShaderStages().GetValue());
  for (xiiUInt32 uiStageBitIndex : shaderBitfield)
  {
    m_hActiveGALShaders[uiStageBitIndex] = pShaderPermutation->GetGALShader(xiiGALShaderType::GetStageFlag(uiStageBitIndex));

    XII_ASSERT_DEV(!m_hActiveGALShaders[uiStageBitIndex].IsInvalidated(), "Invalid GAL {} Shader handle.", xiiGALShaderType::Names[uiStageBitIndex]);
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
      BindConstantBuffer("xiiMaterialConstants", pMaterial->m_hConstantBufferStorage);
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

void xiiRenderContext::ApplyConstantBufferBindings()
{
  xiiGALDevice*                    pDevice            = xiiGALDevice::GetDefaultDevice();
  xiiGALPipelineState*             pPipelineState     = pDevice->GetPipelineState(m_hCurrentPipelineState);
  xiiGALPipelineResourceSignature* pResourceSignature = pDevice->GetPipelineResourceSignature(pPipelineState->GetDescription().m_hPipelineResourceSignature);

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    if (binding.m_ResourceType != xiiGALShaderResourceType::ConstantBuffer)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    BoundConstantBuffer boundConstantBuffer;
    if (!m_BoundConstantBuffers.TryGetValue(uiResourceHash, boundConstantBuffer))
    {
      // If the shader was compiled with debug info the shader compiler will not strip unused resources and
      // thus this error would trigger although the shader doesn't actually uses the resource.
      /// \todo if (!pBinary->m_bWasCompiledWithDebug)
      {
        xiiLog::Error("No resource is bound for constant buffer slot '{0}'", binding.m_sName);
      }
      m_pCommandList->SetConstantBuffer(binding, xiiGALBufferHandle());
      continue;
    }

    if (!boundConstantBuffer.m_hConstantBuffer.IsInvalidated())
    {
      m_pCommandList->SetConstantBuffer(binding, boundConstantBuffer.m_hConstantBuffer);
    }
    else
    {
      xiiConstantBufferStorageBase* pConstantBufferStorage = nullptr;
      if (TryGetConstantBufferStorage(boundConstantBuffer.m_hConstantBufferStorage, pConstantBufferStorage))
      {
        m_pCommandList->SetConstantBuffer(binding, pConstantBufferStorage->GetGALBufferHandle());
      }
      else
      {
        xiiLog::Error("Invalid constant buffer storage is bound for slot '{0}'", binding.m_sName);
        m_pCommandList->SetConstantBuffer(binding, xiiGALBufferHandle());
      }
    }
  }
}

void xiiRenderContext::ApplyResourceViewBindings(xiiEnum<xiiGALShaderResourceType> type)
{
  XII_ASSERT_DEV(type == xiiGALShaderResourceType::BufferSRV || type == xiiGALShaderResourceType::TextureSRV, "");

  xiiGALDevice*                    pDevice            = xiiGALDevice::GetDefaultDevice();
  xiiGALPipelineState*             pPipelineState     = pDevice->GetPipelineState(m_hCurrentPipelineState);
  xiiGALPipelineResourceSignature* pResourceSignature = pDevice->GetPipelineResourceSignature(pPipelineState->GetDescription().m_hPipelineResourceSignature);

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();
    ResourceBinding resourceBinding;

    if (binding.m_ResourceType == xiiGALShaderResourceType::BufferSRV && binding.m_ResourceType == type)
    {
      m_BoundResources.TryGetValue(uiResourceHash, resourceBinding);
      m_pCommandList->SetShaderResourceBufferView(binding, resourceBinding.m_hBufferView);
    }
    else if ((binding.m_ResourceType == xiiGALShaderResourceType::TextureSRV || binding.m_ResourceType == xiiGALShaderResourceType::TextureAndSampler) && (type == xiiGALShaderResourceType::TextureSRV || type == xiiGALShaderResourceType::TextureAndSampler))
    {
      m_BoundResources.TryGetValue(uiResourceHash, resourceBinding);
      m_pCommandList->SetShaderResourceTextureView(binding, resourceBinding.m_hTextureView);
    }
  }
}

void xiiRenderContext::ApplyUnorderedAccessViewBindings()
{
  xiiGALDevice*                    pDevice            = xiiGALDevice::GetDefaultDevice();
  xiiGALPipelineState*             pPipelineState     = pDevice->GetPipelineState(m_hCurrentPipelineState);
  xiiGALPipelineResourceSignature* pResourceSignature = pDevice->GetPipelineResourceSignature(pPipelineState->GetDescription().m_hPipelineResourceSignature);

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();
    ResourceBinding resourceBinding;

    if (binding.m_ResourceType == xiiGALShaderResourceType::BufferUAV)
    {
      m_BoundUAVs.TryGetValue(uiResourceHash, resourceBinding);
      m_pCommandList->SetUnorderedAccessBufferView(binding, resourceBinding.m_hBufferView);
    }
    else if (binding.m_ResourceType == xiiGALShaderResourceType::TextureUAV)
    {
      m_BoundUAVs.TryGetValue(uiResourceHash, resourceBinding);
      m_pCommandList->SetUnorderedAccessTextureView(binding, resourceBinding.m_hTextureView);
    }
  }
}

void xiiRenderContext::ApplySamplerBindings()
{
  xiiGALDevice*                    pDevice            = xiiGALDevice::GetDefaultDevice();
  xiiGALPipelineState*             pPipelineState     = pDevice->GetPipelineState(m_hCurrentPipelineState);
  xiiGALPipelineResourceSignature* pResourceSignature = pDevice->GetPipelineResourceSignature(pPipelineState->GetDescription().m_hPipelineResourceSignature);

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    if (binding.m_ResourceType != xiiGALShaderResourceType::Sampler && binding.m_ResourceType != xiiGALShaderResourceType::TextureAndSampler)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiGALSamplerHandle hSampler;
    if (!m_BoundSamplers.TryGetValue(uiResourceHash, hSampler))
    {
      hSampler = GetDefaultSampler(xiiDefaultSamplerFlags::LinearFiltering); // Bind a default state to avoid DX11 errors.
    }

    m_pCommandList->SetSampler(binding, hSampler);
  }
}

void xiiRenderContext::SetDefaultTextureFilter(xiiEnum<xiiTextureFilterSetting> filter)
{
  XII_ASSERT_DEBUG(filter >= xiiTextureFilterSetting::FixedBilinear && filter <= xiiTextureFilterSetting::FixedAnisotropic16x, "Invalid default texture filter");

  filter = xiiMath::Clamp((xiiTextureFilterSetting::Enum)filter, xiiTextureFilterSetting::FixedBilinear, xiiTextureFilterSetting::FixedAnisotropic16x);

  if (m_DefaultTextureFilter == filter)
    return;

  m_DefaultTextureFilter = filter;
}

xiiEnum<xiiTextureFilterSetting> xiiRenderContext::GetSpecificTextureFilter(xiiEnum<xiiTextureFilterSetting> configuration) const
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

///////////////////////////////////////////////////////////////////////////////////////////////
// Resource Cache Hash

static_assert(sizeof(xiiUInt32) == sizeof(xiiGALTextureViewHandle));
namespace
{
  XII_ALWAYS_INLINE xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiGALTextureViewHandle& value)
  {
    stream << reinterpret_cast<const xiiUInt32&>(value);
    return stream;
  }
} // namespace

xiiUInt32 xiiRenderContext::ResourceCacheHash::Hash(const xiiGALRenderTargetSetup& renderTargetSetup)
{
  xiiHashStreamWriter32 writer;
  writer << renderTargetSetup.GetDepthStencilTarget();

  const xiiUInt8 uiCount = renderTargetSetup.GetRenderTargetCount();
  writer << uiCount;

  for (xiiUInt8 i = 0; i < uiCount; ++i)
  {
    writer << renderTargetSetup.GetRenderTarget(i);
  }

  return writer.GetHashValue();
}

bool xiiRenderContext::ResourceCacheHash::Equal(const xiiGALRenderTargetSetup& a, const xiiGALRenderTargetSetup& b)
{
  return a == b;
}

xiiUInt32 xiiRenderContext::ResourceCacheHash::Hash(const xiiGALRenderingSetup& renderingSetup)
{
  xiiHashStreamWriter32 writer;
  writer << renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();

  const xiiUInt8 uiCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  writer << uiCount;

  for (xiiUInt8 i = 0; i < uiCount; ++i)
  {
    writer << renderingSetup.m_RenderTargetSetup.GetRenderTarget(i);
  }

  writer << renderingSetup.m_uiRenderTargetClearMask;
  writer << renderingSetup.m_bClearDepth;
  writer << renderingSetup.m_bClearStencil;
  writer << renderingSetup.m_bDiscardColor;
  writer << renderingSetup.m_bDiscardDepth;

  return writer.GetHashValue();
}

bool xiiRenderContext::ResourceCacheHash::Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b)
{
  return a == b;
}

xiiUInt32 xiiRenderContext::ResourceCacheHash::Hash(const xiiGALPipelineStateCreationDescription& pipelineCreationDescription)
{
  return xiiGALDescriptorHash::Hash(pipelineCreationDescription);
}

bool xiiRenderContext::ResourceCacheHash::Equal(const xiiGALPipelineStateCreationDescription& a, const xiiGALPipelineStateCreationDescription& b)
{
  return xiiGALDescriptorHash::Equal(a, b);
}

///////////////////////////////////////////////////////////////////////////////////////////////

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_RenderContext_Implementation_RenderContext);
