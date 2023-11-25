#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoder.h>
#include <GraphicsFoundation/Resources/BufferView.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/Shader/InputLayout.h>

#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/DynamicMeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>

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
  m_Topology                   = xiiGALPrimitiveTopology::Undefined;
  m_uiMeshBufferPrimitiveCount = 0;
  m_DefaultTextureFilter       = xiiTextureFilterSetting::FixedAnisotropic4x;
  m_bAllowAsyncShaderLoading   = false;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<xiiGlobalConstants>();

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

xiiGALGraphicsCommandEncoder* xiiRenderContext::BeginRendering(xiiGALPass* pGALPass, const xiiGALRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName, bool bStereoSupport)
{
  xiiUInt32 msaaSampleCount = 1;

  xiiGALTextureViewHandle hRTV;
  if (renderingSetup.m_RenderTargetSetup.GetRenderTargetCount() > 0)
  {
    hRTV = renderingSetup.m_RenderTargetSetup.GetRenderTarget(0);
  }
  if (hRTV.IsInvalidated())
  {
    hRTV = renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();
  }

  if (const xiiGALTextureView* pRTV = xiiGALDevice::GetDefaultDevice()->GetTextureView(hRTV))
  {
    msaaSampleCount = pRTV->GetTexture()->GetDescription().m_uiSampleCount;
  }

  if (msaaSampleCount > 1)
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

  auto pGALCommandEncoder = pGALPass->BeginRendering(renderingSetup, sName);

  pGALCommandEncoder->SetViewport(viewport);

  m_pGALPass           = pGALPass;
  m_pGALCommandEncoder = pGALCommandEncoder;
  m_bCompute           = false;
  m_bStereoRendering   = bStereoSupport;

  return pGALCommandEncoder;
}

void xiiRenderContext::EndRendering()
{
  m_pGALPass->EndRendering(GetGraphicsCommandEncoder());

  m_pGALPass           = nullptr;
  m_pGALCommandEncoder = nullptr;
  m_bStereoRendering   = false;

  // TODO: The render context needs to reset its state after every encoding block if we want to record to separate command buffers.
  // Although this is currently not possible since a lot of high level code binds stuff only once per frame on the render context.
  // Resetting the state after every encoding block breaks those assumptions.
  //ResetContextState();
}

xiiGALComputeCommandEncoder* xiiRenderContext::BeginCompute(xiiGALPass* pGALPass, xiiStringView sName /*= {}*/)
{
  auto pGALCommandEncoder = pGALPass->BeginCompute(sName);

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

void xiiRenderContext::SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sTempValue)
{
  xiiTempHashedString sHashedName(sName);

  xiiHashedString sNameHash;
  xiiHashedString sValue;
  if (xiiShaderManager::IsPermutationValueAllowed(sNameHash, sHashedName, sTempValue, sNameHash, sValue))
  {
    SetShaderPermutationVariableInternal(sNameHash, sValue);
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
    BindTexture3D(sSlotName, xiiGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
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
    BindTextureCube(sSlotName, xiiGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
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
  if (m_BoundResources.TryGetValue(sSlotName.GetHash(), pOldResourceBinding))
  {
    if (pOldResourceBinding->m_Type == ResourceBinding::Buffer && pOldResourceBinding->m_hBufferView == hUnorderedAccessView)
      return;

    *pOldResourceBinding = ResourceBinding{.m_Type = ResourceBinding::Buffer, .m_hBufferView = hUnorderedAccessView, .m_hTextureView = xiiGALTextureViewHandle()};
  }
  else
  {
    m_BoundResources.Insert(sSlotName.GetHash(), ResourceBinding{.m_Type = ResourceBinding::Buffer, .m_hBufferView = hUnorderedAccessView, .m_hTextureView = xiiGALTextureViewHandle()});
  }

  m_StateFlags.Add(xiiRenderContextFlags::UAVBindingChanged);
}

void xiiRenderContext::BindTextureUAV(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hUnorderedAccessView)
{
  ResourceBinding* pOldResourceBinding = nullptr;
  if (m_BoundResources.TryGetValue(sSlotName.GetHash(), pOldResourceBinding))
  {
    if (pOldResourceBinding->m_Type == ResourceBinding::Texture && pOldResourceBinding->m_hTextureView == hUnorderedAccessView)
      return;

    *pOldResourceBinding = ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hUnorderedAccessView};
  }
  else
  {
    m_BoundResources.Insert(sSlotName.GetHash(), ResourceBinding{.m_Type = ResourceBinding::Texture, .m_hBufferView = xiiGALBufferViewHandle(), .m_hTextureView = hUnorderedAccessView});
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

void xiiRenderContext::BindMeshBuffer(xiiGALBufferHandle hVertexBuffer, xiiGALBufferHandle hIndexBuffer, const xiiInputLayoutInfo* pInputLayoutInfo, xiiGALPrimitiveTopology::Enum topology, xiiUInt32 uiPrimitiveCount, xiiGALBufferHandle hVertexBuffer2, xiiGALBufferHandle hVertexBuffer3, xiiGALBufferHandle hVertexBuffer4)
{
  if (m_hVertexBuffers[0] == hVertexBuffer && m_hVertexBuffers[1] == hVertexBuffer2 && m_hVertexBuffers[2] == hVertexBuffer3 && m_hVertexBuffers[3] == hVertexBuffer4 && m_hIndexBuffer == hIndexBuffer && m_pInputLayoutInfo == pInputLayoutInfo && m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
  {
    return;
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (pInputLayoutInfo)
  {
    for (xiiUInt32 i1 = 0; i1 < pInputLayoutInfo->m_VertexStreams.GetCount(); ++i1)
    {
      for (xiiUInt32 i2 = 0; i2 < pInputLayoutInfo->m_VertexStreams.GetCount(); ++i2)
      {
        if (i1 != i2)
        {
          XII_ASSERT_DEBUG(pInputLayoutInfo->m_VertexStreams[i1].m_Semantic != pInputLayoutInfo->m_VertexStreams[i2].m_Semantic, "Same semantic cannot be used twice in the same vertex declaration");
        }
      }
    }
  }
#endif

  if (m_Topology != topology)
  {
    m_Topology = topology;

    xiiTempHashedString sTopologies[xiiGALPrimitiveTopology::ENUM_COUNT] = {
      xiiTempHashedString("TRIANGLE_LIST"),
      xiiTempHashedString("TRIANGLE_STRIP"),
      xiiTempHashedString("POINT_LIST"),
      xiiTempHashedString("LINE_LIST"),
      xiiTempHashedString("LINE_STRIP"),
      xiiTempHashedString("TRIANGLE_LIST_ADJACENT"),
      xiiTempHashedString("TRANGLE_STRIP_ADJACENT"),
      xiiTempHashedString("LINE_LIST_ADJACENT"),
      xiiTempHashedString("LINE_STRIP_ADJACENT"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_1"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_2"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_3"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_4"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_5"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_6"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_7"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_8"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_9"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_10"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_11"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_12"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_13"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_14"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_15"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_16"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_17"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_18"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_19"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_20"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_21"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_22"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_23"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_24"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_25"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_26"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_27"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_28"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_29"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_30"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_31"),
      xiiTempHashedString("CONTROL_POINT_PATCH_LIST_32"),
    };

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_Topology]);
  }

  m_hVertexBuffers[0]          = hVertexBuffer;
  m_hVertexBuffers[1]          = hVertexBuffer2;
  m_hVertexBuffers[2]          = hVertexBuffer3;
  m_hVertexBuffers[3]          = hVertexBuffer4;
  m_hIndexBuffer               = hIndexBuffer;
  m_pInputLayoutInfo           = pInputLayoutInfo;
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

  auto pCommandEncoder = GetGraphicsCommandEncoder();

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
    if ((bForce || m_StateFlags.IsAnySet(xiiRenderContextFlags::TextureBindingChanged | xiiRenderContextFlags::UAVBindingChanged | xiiRenderContextFlags::SamplerBindingChanged | xiiRenderContextFlags::BufferBindingChanged | xiiRenderContextFlags::ConstantBufferBindingChanged)))
    {
      if (pShaderPermutation == nullptr)
      {
        pShaderPermutation = xiiResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
      }
    }

    xiiLogBlock applyBindingsBlock("Applying Shader Bindings", pShaderPermutation != nullptr ? pShaderPermutation->GetResourceDescription().GetData() : "");

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::UAVBindingChanged))
    {
      if (pShaderPermutation == nullptr)
      {
        return XII_FAILURE;
      }

      // RWTextures/UAV are usually only supported in compute and pixel shader.
      if (auto pBin = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::Compute))
      {
        ApplyUnorderedAccessViewBindings(pBin);
      }
      if (auto pBin = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::Pixel))
      {
        ApplyUnorderedAccessViewBindings(pBin);
      }

      m_StateFlags.Remove(xiiRenderContextFlags::UAVBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::TextureBindingChanged))
    {
      if (pShaderPermutation == nullptr)
      {
        return XII_FAILURE;
      }

      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::GetStageFlag(stage)))
        {
          ApplyResourceViewBindings(xiiGALShaderStage::GetStageFlag(stage), pBin, xiiGALShaderResourceType::TextureSRV);
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::TextureBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::SamplerBindingChanged))
    {
      if (pShaderPermutation == nullptr)
      {
        return XII_FAILURE;
      }

      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::GetStageFlag(stage)))
        {
          ApplySamplerBindings(xiiGALShaderStage::GetStageFlag(stage), pBin);
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::SamplerBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::BufferBindingChanged))
    {
      if (pShaderPermutation == nullptr)
      {
        return XII_FAILURE;
      }

      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::GetStageFlag(stage)))
        {
          ApplyResourceViewBindings(xiiGALShaderStage::GetStageFlag(stage), pBin, xiiGALShaderResourceType::BufferSRV);
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::BufferBindingChanged);
    }

    if (pMaterial != nullptr)
    {
      pMaterial->UpdateConstantBuffer(pShaderPermutation);
      BindConstantBuffer("xiiMaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    UploadConstants();

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::ConstantBufferBindingChanged))
    {
      if (pShaderPermutation == nullptr)
      {
        return XII_FAILURE;
      }

      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::GetStageFlag(stage)))
        {
          ApplyConstantBufferBindings(pBin);
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::ConstantBufferBindingChanged);
    }
  }

  if ((bForce || bRebuildInputLayout) && !m_bCompute)
  {
    if (m_hActiveGALShader.IsInvalidated())
      return XII_FAILURE;

    auto pCommandEncoder = GetGraphicsCommandEncoder();

    if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::MeshBufferBindingChanged))
    {
      pCommandEncoder->SetPrimitiveTopology(m_Topology);

      for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_hVertexBuffers); ++i)
      {
        pCommandEncoder->SetVertexBuffer(i, m_hVertexBuffers[i]);
      }

      if (!m_hIndexBuffer.IsInvalidated())
        pCommandEncoder->SetIndexBuffer(m_hIndexBuffer, 0);
    }

    xiiGALInputLayoutHandle hInputLayout;
    if (m_pInputLayoutInfo != nullptr && BuildInputLayout(m_hActiveGALShader, *m_pInputLayoutInfo, hInputLayout).Failed())
      return XII_FAILURE;

    // If there is a vertex buffer we need a valid vertex declaration as well.
    if ((!m_hVertexBuffers[0].IsInvalidated() || !m_hVertexBuffers[1].IsInvalidated() || !m_hVertexBuffers[2].IsInvalidated() || !m_hVertexBuffers[3].IsInvalidated()) && hInputLayout.IsInvalidated())
      return XII_FAILURE;

    pCommandEncoder->SetInputLayout(hInputLayout);

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
  m_pInputLayoutInfo           = nullptr;
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

  /// \todo Check vertex shader render target array index.
  permutationVariables.Insert(sVSRTAI, sTrue);

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
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_hDefaultSamplers); ++i)
  {
    if (!s_hDefaultSamplers[i].IsInvalidated())
    {
      xiiGALDevice::GetDefaultDevice()->DestroySampler(s_hDefaultSamplers[i]);
      s_hDefaultSamplers[i].Invalidate();
    }
  }

  // Cleanup vertex declarations
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

// static
xiiResult xiiRenderContext::BuildInputLayout(xiiGALShaderHandle hShader, const xiiInputLayoutInfo& decl, xiiGALInputLayoutHandle& out_Declaration)
{
  ShaderVertexDecl svd;
  svd.m_hShader           = hShader;
  svd.m_uiInputLayoutHash = decl.m_uiHash;

  bool bExisted = false;
  auto it       = s_GALInputLayouts.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    const xiiGALShader* pShader = xiiGALDevice::GetDefaultDevice()->GetShader(hShader);

    auto pBytecode = pShader->GetDescription().m_ByteCodes[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex)];

    xiiGALInputLayoutCreationDescription vd;
    vd.m_hShader = hShader;

    for (xiiUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
    {
      auto& stream = decl.m_VertexStreams[slot];

      // stream.m_Format
      xiiGALLayoutElement gal;
      gal.m_Format                 = stream.m_Format;
      gal.m_Semantic               = stream.m_Semantic;
      gal.m_uiRelativeOffset       = stream.m_uiOffset;
      gal.m_uiBufferSlot           = stream.m_uiVertexBufferSlot;
      gal.m_Frequency              = xiiGALInputElementFrequency::PerVertex;
      gal.m_uiInstanceDataStepRate = 0;
      vd.m_LayoutElements.PushBack(gal);
    }

    out_Declaration = xiiGALDevice::GetDefaultDevice()->CreateInputLayout(vd);

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
  BindConstantBuffer("xiiGlobalConstants", m_hGlobalConstantBufferStorage);

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
    auto pCommandEncoder = GetGraphicsCommandEncoder();

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

void xiiRenderContext::ApplyConstantBufferBindings(const xiiShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != xiiGALShaderResourceType::ConstantBuffer)
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

void xiiRenderContext::ApplyResourceViewBindings(xiiBitflags<xiiGALShaderStage> stage, const xiiShaderStageBinary* pBinary, xiiEnum<xiiGALShaderResourceType> type)
{
  XII_ASSERT_DEV(type == xiiGALShaderResourceType::BufferSRV || type == xiiGALShaderResourceType::TextureSRV, "");

  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();
    ResourceBinding resourceBinding;

    if (binding.m_Type == xiiGALShaderResourceType::BufferSRV && binding.m_Type == type)
    {
      m_BoundResources.TryGetValue(uiResourceHash, resourceBinding);
      m_pGALCommandEncoder->SetBufferView(stage, binding.m_iSlot, resourceBinding.m_hBufferView);
    }
    else if (binding.m_Type == xiiGALShaderResourceType::TextureSRV && binding.m_Type == type)
    {
      m_BoundResources.TryGetValue(uiResourceHash, resourceBinding);
      m_pGALCommandEncoder->SetTextureView(stage, binding.m_iSlot, resourceBinding.m_hTextureView);
    }
  }
}

void xiiRenderContext::ApplyUnorderedAccessViewBindings(const xiiShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();
    ResourceBinding resourceBinding;

    if (binding.m_Type == xiiGALShaderResourceType::BufferUAV)
    {
      m_BoundUAVs.TryGetValue(uiResourceHash, resourceBinding);
      m_pGALCommandEncoder->SetUnorderedAccessBufferView(binding.m_iSlot, resourceBinding.m_hBufferView);
    }
    else if (binding.m_Type == xiiGALShaderResourceType::TextureUAV)
    {
      m_BoundUAVs.TryGetValue(uiResourceHash, resourceBinding);
      m_pGALCommandEncoder->SetUnorderedAccessTextureView(binding.m_iSlot, resourceBinding.m_hTextureView);
    }
  }
}

void xiiRenderContext::ApplySamplerBindings(xiiBitflags<xiiGALShaderStage> stage, const xiiShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != xiiGALShaderResourceType::Sampler)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiGALSamplerHandle hSampler;
    if (!m_BoundSamplers.TryGetValue(uiResourceHash, hSampler))
    {
      hSampler = GetDefaultSampler(xiiDefaultSamplerFlags::LinearFiltering); // Bind a default state to avoid DX11 errors.
    }

    m_pGALCommandEncoder->SetSampler(stage, binding.m_iSlot, hSampler);
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

  int iFilter = m_DefaultTextureFilter;

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

  iFilter = xiiMath::Clamp<int>(iFilter, xiiTextureFilterSetting::FixedBilinear, xiiTextureFilterSetting::FixedAnisotropic16x);

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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_RenderContext_Implementation_RenderContext);
