#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/DynamicMeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

xiiRenderContext*                     xiiRenderContext::s_pDefaultInstance = nullptr;
xiiHybridArray<xiiRenderContext*, 2U> xiiRenderContext::s_Instances;

xiiRenderContext::xiiRenderContext()
{
  s_Instances.PushBack(this);

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_pCommandList = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  XII_ASSERT_DEV(m_pCommandList != nullptr, "Failed to create command list!");

  m_pGlobalConstantsBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(xiiGALDevice::GetDefaultDevice(), sizeof(xiiGlobalConstants), "xiiGlobalConstants");
  m_pGlobalConstants       = xiiMakeBlobPtr(reinterpret_cast<xiiGlobalConstants*>(xiiFoundation::GetAlignedAllocator()->Allocate(sizeof(xiiGlobalConstants), 16U)), 1U);

  xiiMemoryUtils::ZeroFill(m_pGlobalConstants.GetPtr(), 1U);

  XII_ASSERT_DEBUG(!m_pGlobalConstants.IsEmpty(), "Invalid global constants buffer.");

  ResetContextState();
}

xiiRenderContext::~xiiRenderContext()
{
  xiiFoundation::GetAlignedAllocator()->Deallocate(m_pGlobalConstants.GetPtr());

  m_pGlobalConstants.Clear();
  m_pGlobalConstantsBuffer.Clear();
}

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

void xiiRenderContext::DestroyInstance(xiiRenderContext* pRenderContext)
{
  XII_DEFAULT_DELETE(pRenderContext);
}

void xiiRenderContext::BeginRendering(const xiiRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName, bool bStereoRendering)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::None, "Already in a scope.");

  m_RenderContextScope = RenderContextScope::Graphics;
  m_bStereoRendering   = bStereoRendering;
  m_RenderingSetup     = renderingSetup;

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

  m_pCommandList->Begin();

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

  m_pCommandList->End();

  xiiSharedPtr<xiiGALDevice> pDevice       = xiiGALDevice::GetDefaultDevice();
  xiiGALCommandQueue*        pCommandQueue = pDevice->GetCommandQueue();

  pCommandQueue->Submit(m_pCommandList);

  m_bStereoRendering   = false;
  m_RenderContextScope = RenderContextScope::None;
}

void xiiRenderContext::BeginCompute(xiiStringView sName)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::None, "Already in a scope.");

  m_RenderContextScope = RenderContextScope::Compute;

  m_pCommandList->Begin();

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

  m_pCommandList->End();

  xiiSharedPtr<xiiGALDevice> pDevice       = xiiGALDevice::GetDefaultDevice();
  xiiGALCommandQueue*        pCommandQueue = pDevice->GetCommandQueue();

  pCommandQueue->Submit(m_pCommandList);

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
  // Don't set m_hMaterial directly since we first need to check whether the material has been modified in the mean time.
  m_hNewMaterial = hMaterial;

  m_StateFlags.Add(xiiRenderContextFlags::MaterialBindingChanged);
}

void xiiRenderContext::BindShader(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags)
{
  m_hMaterial.Invalidate();

  m_StateFlags.Remove(xiiRenderContextFlags::MaterialBindingChanged);

  BindShaderInternal(hShader, flags);
}

void xiiRenderContext::SetBlendState(xiiSharedPtr<xiiGALBlendState> pBlendState)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "SetBlendState() is only valid in a graphics pipeline.");
  XII_ASSERT_DEV(pBlendState != nullptr, "The blend state is invalid.");

  m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pBlendState = pBlendState;

  m_StateFlags.Add(xiiRenderContextFlags::PipelineChanged);
}

void xiiRenderContext::SetDepthStencilState(xiiSharedPtr<xiiGALDepthStencilState> pDepthStencilState)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "SetDepthStencilState() is only valid in a graphics pipeline.");
  XII_ASSERT_DEV(pDepthStencilState != nullptr, "The depth stencil state is invalid.");

  m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pDepthStencilState = pDepthStencilState;

  m_StateFlags.Add(xiiRenderContextFlags::PipelineChanged);
}

void xiiRenderContext::SetRasterizerState(xiiSharedPtr<xiiGALRasterizerState> pRasterizerState)
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "SetRasterizerState() is only valid in a graphics pipeline.");
  XII_ASSERT_DEV(pRasterizerState != nullptr, "The rasterizer state is invalid.");

  m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pRasterizerState = pRasterizerState;

  m_StateFlags.Add(xiiRenderContextFlags::PipelineChanged);
}

void xiiRenderContext::BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer)
{
  xiiResourceLock<xiiDynamicMeshBufferResource> pMeshBuffer(hDynamicMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetInputLayout()), pMeshBuffer->GetDescriptor().m_Topology, pMeshBuffer->GetDescriptor().m_uiMaxPrimitives, xiiMakeArrayPtr(&pMeshBuffer->GetColorBuffer(), 1U));
}

void xiiRenderContext::BindMeshBuffer(const xiiMeshBufferResourceHandle& hMeshBuffer)
{
  xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetInputLayout()), pMeshBuffer->GetTopology(), pMeshBuffer->GetPrimitiveCount());
}

void xiiRenderContext::BindMeshBuffer(xiiSharedPtr<xiiGALBuffer> pVertexBuffer0, xiiSharedPtr<xiiGALBuffer> pIndexBuffer, const xiiInputLayoutInfo* pInputLayoutInfo, xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount, xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>> pVertexBuffers)
{
  if ((!m_VertexBuffers.IsEmpty() && (m_VertexBuffers[0] == pVertexBuffer0 || m_VertexBuffers.GetArrayPtr().GetSubArray(1, pVertexBuffers.GetCount()) == pVertexBuffers)) && m_pIndexBuffer == pIndexBuffer && m_pInputLayoutInfo == pInputLayoutInfo && m_GraphicsPipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
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
          XII_ASSERT_DEBUG(pInputLayoutInfo->m_VertexStreams[i1].m_Semantic != pInputLayoutInfo->m_VertexStreams[i2].m_Semantic, "The same semantic cannot be used twice in the same input layout.");
        }
      }
    }
  }
#endif

  if (m_GraphicsPipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology != topology)
  {
    m_GraphicsPipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology = topology;

    static bool                bInitialized = false;
    static xiiTempHashedString sTopologies[xiiGALPrimitiveTopology::ENUM_COUNT];
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

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_GraphicsPipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology]);
  }

  m_VertexBuffers.EnsureCount(1);
  m_VertexBuffers[0] = pVertexBuffer0;

  if (!pVertexBuffers.IsEmpty())
  {
    m_VertexBuffers.EnsureCount(pVertexBuffers.GetCount() + 1);

    for (xiiUInt32 i = 0; i < pVertexBuffers.GetCount(); ++i)
    {
      m_VertexBuffers[i + 1] = pVertexBuffers[i];
    }
  }

  m_pIndexBuffer                                                  = pIndexBuffer;
  m_pInputLayoutInfo                                              = pInputLayoutInfo;
  m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pInputLayout = nullptr;
  m_uiMeshBufferPrimitiveCount                                    = uiPrimitiveCount;

  m_StateFlags.Add(xiiRenderContextFlags::MeshBufferBindingChanged);
}

xiiResult xiiRenderContext::DrawMeshBuffer(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiInstanceCount)
{
  BeginClearThenLoadInternalRenderPass();

  if (ApplyContextStates().Failed() || uiPrimitiveCount == 0U || uiInstanceCount == 0U)
    return XII_FAILURE;

  XII_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount, "Invalid primitive range: first primitive ({0}) can't be larger than number of primitives ({1})", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = xiiMath::Min(uiPrimitiveCount, m_uiMeshBufferPrimitiveCount - uiFirstPrimitive);
  XII_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  const xiiUInt32 uiVertsPerPrimitive = xiiGALPrimitiveTopology::VerticesPerPrimitive(m_GraphicsPipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology);

  uiPrimitiveCount *= uiVertsPerPrimitive;
  uiFirstPrimitive *= uiVertsPerPrimitive;
  if (m_bStereoRendering)
  {
    uiInstanceCount *= 2;
  }

  XII_SUCCEED_OR_RETURN(m_pCommandList->CommitShaderResources());

  BeginInternalRenderPass();
  XII_SCOPE_EXIT(EndInternalRenderPass());

  if (m_pIndexBuffer)
  {
    xiiEnum<xiiGALValueType> indexType = m_pIndexBuffer->GetDescription().m_uiElementByteStride <= 2 ? xiiGALValueType::UInt16 : xiiGALValueType::UInt32;

    m_pCommandList->DrawIndexed({uiPrimitiveCount, indexType, uiInstanceCount, uiFirstPrimitive});
  }
  else
  {
    m_pCommandList->Draw({uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive});
  }

  return XII_SUCCESS;
}

xiiResult xiiRenderContext::Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ)
{
  if (ApplyContextStates().Succeeded())
  {
    m_pCommandList->DispatchCompute({uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ});
  }
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
    bool bIsModified = (bForce || m_StateFlags.IsAnySet(xiiRenderContextFlags::ConstantBufferBindingChanged | xiiRenderContextFlags::TextureBindingChanged | xiiRenderContextFlags::BufferBindingChanged | xiiRenderContextFlags::TextureUAVBindingChanged | xiiRenderContextFlags::BufferUAVBindingChanged | xiiRenderContextFlags::SamplerBindingChanged));

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

    if ((bForce || bRebuildInputLayout) && m_RenderContextScope == RenderContextScope::Graphics)
    {
      if (!m_ActiveGALShaders.IsEmpty() && !m_ActiveGALShaders[xiiGALShaderType::Vertex])
        return XII_FAILURE;

      if (bForce || m_StateFlags.IsSet(xiiRenderContextFlags::MeshBufferBindingChanged))
      {
        m_pCommandList->SetVertexBuffers(0, m_VertexBuffers, xiiArrayPtr<xiiUInt64>());

        if (m_pIndexBuffer)
        {
          m_pCommandList->SetIndexBuffer(m_pIndexBuffer);
        }
      }

      if (m_pInputLayoutInfo != nullptr && BuildInputLayout(m_ActiveGALShaders[xiiGALShaderType::Vertex], *m_pInputLayoutInfo, m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pInputLayout).Failed())
        return XII_FAILURE;

      // If there is a vertex buffer we need a valid vertex declaration as well.
      for (const auto& pVertexBuffer : m_VertexBuffers)
      {
        if (pVertexBuffer != nullptr && !m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pInputLayout)
        {
          return XII_FAILURE;
        }
      }

      m_StateFlags.Remove(xiiRenderContextFlags::MeshBufferBindingChanged);
    }

    bool bPipelineStateInvalidated = false;
    if (pShaderPermutation != nullptr)
    {
      // Set render state from shader.
      // Create pipeline state that is valid for this scope.

      xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

      if (m_RenderContextScope == RenderContextScope::Graphics)
      {
        m_GraphicsPipelineDescription.m_pPipelineResourceSignature = (pShaderPermutation != nullptr) ? pShaderPermutation->GetPipelineResourceSignature() : nullptr;

        m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pRenderPass = m_pActiveRenderPass;

        for (auto it : m_ActiveGALShaders)
        {
          auto& pShader = it.Value();

          switch (it.Key())
          {
            case xiiGALShaderType::Vertex:
              m_GraphicsPipelineDescription.m_pVertexShader = m_ActiveGALShaders[xiiGALShaderType::Vertex];
              break;
            case xiiGALShaderType::Pixel:
              m_GraphicsPipelineDescription.m_pPixelShader = m_ActiveGALShaders[xiiGALShaderType::Pixel];
              break;
            case xiiGALShaderType::Domain:
              m_GraphicsPipelineDescription.m_pDomainShader = m_ActiveGALShaders[xiiGALShaderType::Domain];
              break;
            case xiiGALShaderType::Hull:
              m_GraphicsPipelineDescription.m_pHullShader = m_ActiveGALShaders[xiiGALShaderType::Hull];
              break;
            case xiiGALShaderType::Geometry:
              m_GraphicsPipelineDescription.m_pGeometryShader = m_ActiveGALShaders[xiiGALShaderType::Geometry];
              break;
            case xiiGALShaderType::Amplification:
              m_GraphicsPipelineDescription.m_pAmplificationShader = m_ActiveGALShaders[xiiGALShaderType::Amplification];
              break;
            case xiiGALShaderType::Mesh:
              m_GraphicsPipelineDescription.m_pMeshShader = m_ActiveGALShaders[xiiGALShaderType::Mesh];
              break;

              XII_DEFAULT_CASE_NOT_IMPLEMENTED;
          }
        }

        if (pShaderPermutation != nullptr)
        {
          if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoBlendState))
          {
            m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pBlendState = pShaderPermutation->GetBlendState();
          }
          if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoRasterizerState))
          {
            m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pRasterizerState = pShaderPermutation->GetRasterizerState();
          }
          if (!m_ShaderBindFlags.IsSet(xiiShaderBindFlags::NoDepthStencilState))
          {
            m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pDepthStencilState = pShaderPermutation->GetDepthStencilState();
          }
        }

        if (!m_GraphicsPipelineCreationCache.TryGetValue(m_GraphicsPipelineDescription, m_pGraphicsPipelineState))
        {
          m_pGraphicsPipelineState = pDevice->CreateGraphicsPipelineState(m_GraphicsPipelineDescription);

          XII_VERIFY(!m_GraphicsPipelineCreationCache.Insert(m_GraphicsPipelineDescription, m_pGraphicsPipelineState), "Overwriting an existing cached pipeline state, this is unexpected behavior.");

          m_pCommandList->SetPipelineState(m_pGraphicsPipelineState);

          bPipelineStateInvalidated = true;
        }
      }
      else if (m_RenderContextScope == RenderContextScope::Compute)
      {
        m_ComputePipelineDescription.m_pComputeShader = m_ActiveGALShaders[xiiGALShaderType::Compute];

        if (!m_ComputePipelineCreationCache.TryGetValue(m_ComputePipelineDescription, m_pComputePipelineState))
        {
          m_pComputePipelineState = pDevice->CreateComputePipelineState(m_ComputePipelineDescription);

          XII_VERIFY(!m_ComputePipelineCreationCache.Insert(m_ComputePipelineDescription, m_pComputePipelineState), "Overwriting an existing cached pipeline state, this is unexpected behavior.");

          m_pCommandList->SetPipelineState(m_pComputePipelineState);

          bPipelineStateInvalidated = true;
        }
      }

      XII_ASSERT_DEV(m_pGraphicsPipelineState || m_pComputePipelineState, "Implementation error!");
    }

    if (bIsModified || bPipelineStateInvalidated)
    {
      if (bPipelineStateInvalidated)
      {
        if (m_pGraphicsPipelineState && m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pRasterizerState)
        {
          if (m_GraphicsPipelineDescription.m_GraphicsPipeline.m_pRasterizerState->GetDescription().m_bScissorEnable)
          {
            const auto& framebufferDescription = GetCurrentFramebuffer()->GetDescription();

            m_pCommandList->SetScissorRect({framebufferDescription.m_FramebufferSize.width, framebufferDescription.m_FramebufferSize.height});
          }
        }
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::BufferUAVBindingChanged))
      {
        ApplyBufferUAVBindings();

        m_StateFlags.Remove(xiiRenderContextFlags::BufferUAVBindingChanged);
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::TextureUAVBindingChanged))
      {
        ApplyTextureUAVBindings();

        m_StateFlags.Remove(xiiRenderContextFlags::TextureUAVBindingChanged);
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::BufferBindingChanged))
      {
        ApplyBufferSRVBindings();

        m_StateFlags.Remove(xiiRenderContextFlags::BufferBindingChanged);
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::TextureBindingChanged))
      {
        ApplyTextureSRVBindings();

        m_StateFlags.Remove(xiiRenderContextFlags::TextureBindingChanged);
      }

      if (bPipelineStateInvalidated || bForce || m_StateFlags.IsSet(xiiRenderContextFlags::SamplerBindingChanged))
      {
        ApplySamplerBindings();

        m_StateFlags.Remove(xiiRenderContextFlags::SamplerBindingChanged);
      }
    }

    // Note that pMaterial is only valid, if material constants have changed, so this also always implies that ConstantBufferBindingChanged is set.
    if (pMaterial != nullptr)
    {
      pMaterial->UpdateConstantBuffer(pShaderPermutation);

      BindConstantBuffer("xiiMaterialConstants", pMaterial->m_pMaterialConstantsBuffer);
    }

    BindConstantBuffer(XII_PP_STRINGIFY(xiiGlobalConstants), m_pGlobalConstantsBuffer);

    {
      xiiGALMapHelper<xiiGlobalConstants> pGlobalConstants(m_pCommandList, m_pGlobalConstantsBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);

      memcpy(pGlobalConstants.GetMappedData(), m_pGlobalConstants.GetPtr(), sizeof(xiiGlobalConstants));
    }

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
  m_ActiveGALShaders.Clear();

  m_StateFlags.Add(xiiRenderContextFlags::ConstantBufferBindingChanged | xiiRenderContextFlags::TextureBindingChanged | xiiRenderContextFlags::BufferBindingChanged | xiiRenderContextFlags::TextureUAVBindingChanged | xiiRenderContextFlags::BufferUAVBindingChanged | xiiRenderContextFlags::SamplerBindingChanged);

  if (!m_hActiveShader.IsValid())
    return nullptr;

  m_hActiveShaderPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, m_bAllowAsyncShaderLoading);

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
    m_ActiveGALShaders[xiiGALShaderType::GetStageFlag(uiStageBitIndex)] = pShaderPermutation->GetGALShader(xiiGALShaderType::GetStageFlag(uiStageBitIndex));

    XII_ASSERT_DEV(m_ActiveGALShaders[xiiGALShaderType::GetStageFlag(uiStageBitIndex)] != nullptr, "Invalid GAL {} Shader handle.", xiiGALShaderType::Names[uiStageBitIndex]);
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

  // Check whether material has been modified.
  xiiMaterialResource* pMaterial = xiiResourceManager::BeginAcquireResource(m_hNewMaterial, xiiResourceAcquireMode::AllowLoadingFallback);

  if (m_hNewMaterial != m_hMaterial || pMaterial->IsModified())
  {
    auto pCachedValues = pMaterial->GetOrUpdateCachedValues();

    BindShaderInternal(pCachedValues->m_hShader, xiiShaderBindFlags::Default);

    if (pMaterial->m_pMaterialConstantsBuffer)
    {
      BindConstantBuffer("xiiMaterialConstants", pMaterial->m_pMaterialConstantsBuffer);
    }

    for (auto it = pCachedValues->m_PermutationVariables.GetIterator(); it.IsValid(); ++it)
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
  xiiSharedPtr<xiiGALPipelineResourceSignature> pResourceSignature;
  if (m_RenderContextScope == RenderContextScope::Graphics)
  {
    pResourceSignature = m_pGraphicsPipelineState->GetDescription().m_pPipelineResourceSignature;
  }
  else if (m_RenderContextScope == RenderContextScope::Compute)
  {
    pResourceSignature = m_pComputePipelineState->GetDescription().m_pPipelineResourceSignature;
  }

  const auto& resourceBindings = pResourceSignature->GetDescription().m_Resources;

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    if (binding.m_ResourceType != xiiGALShaderResourceType::ConstantBuffer)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiSharedPtr<xiiGALBuffer> pConstantBuffer;
    if (!m_BoundConstantBuffers.TryGetValue(uiResourceHash, pConstantBuffer))
    {
      // If the shader was compiled with debug info the shader compiler will not strip unused resources and thus this error would trigger although the shader doesn't actually use the resource.
      /// \todo if (!pBinary->m_bWasCompiledWithDebug)
      {
        xiiLog::Error("No resource is bound for constant buffer slot '{0}'", binding.m_sName);
      }

      m_pCommandList->SetConstantBuffer(binding, nullptr);
      continue;
    }

    m_pCommandList->SetConstantBuffer(binding, pConstantBuffer);
  }
}

void xiiRenderContext::ApplyBufferSRVBindings()
{
  xiiSharedPtr<xiiGALPipelineResourceSignature> pResourceSignature;
  if (m_RenderContextScope == RenderContextScope::Graphics)
  {
    pResourceSignature = m_pGraphicsPipelineState->GetDescription().m_pPipelineResourceSignature;
  }
  else if (m_RenderContextScope == RenderContextScope::Compute)
  {
    pResourceSignature = m_pComputePipelineState->GetDescription().m_pPipelineResourceSignature;
  }

  const auto& resourceBindings = pResourceSignature->GetDescription().m_Resources;

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    if (binding.m_ResourceType != xiiGALShaderResourceType::BufferSRV)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiSharedPtr<xiiGALBufferView> pBufferSRV;
    m_BoundBufferSRVs.TryGetValue(uiResourceHash, pBufferSRV);

    m_pCommandList->SetShaderResourceBufferView(binding, pBufferSRV);
  }
}

void xiiRenderContext::ApplyTextureSRVBindings()
{
  xiiSharedPtr<xiiGALPipelineResourceSignature> pResourceSignature;
  if (m_RenderContextScope == RenderContextScope::Graphics)
  {
    pResourceSignature = m_pGraphicsPipelineState->GetDescription().m_pPipelineResourceSignature;
  }
  else if (m_RenderContextScope == RenderContextScope::Compute)
  {
    pResourceSignature = m_pComputePipelineState->GetDescription().m_pPipelineResourceSignature;
  }

  const auto& resourceBindings = pResourceSignature->GetDescription().m_Resources;

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    if (binding.m_ResourceType != xiiGALShaderResourceType::TextureSRV)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiSharedPtr<xiiGALTextureView> pTextureSRV;
    m_BoundTextureSRVs.TryGetValue(uiResourceHash, pTextureSRV);

    m_pCommandList->SetShaderResourceTextureView(binding, pTextureSRV);
  }
}

void xiiRenderContext::ApplyBufferUAVBindings()
{
  xiiSharedPtr<xiiGALPipelineResourceSignature> pResourceSignature;
  if (m_RenderContextScope == RenderContextScope::Graphics)
  {
    pResourceSignature = m_pGraphicsPipelineState->GetDescription().m_pPipelineResourceSignature;
  }
  else if (m_RenderContextScope == RenderContextScope::Compute)
  {
    pResourceSignature = m_pComputePipelineState->GetDescription().m_pPipelineResourceSignature;
  }

  const auto& resourceBindings = pResourceSignature->GetDescription().m_Resources;

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    if (binding.m_ResourceType != xiiGALShaderResourceType::BufferUAV)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiSharedPtr<xiiGALBufferView> pBufferUAV;
    m_BoundBufferUAVs.TryGetValue(uiResourceHash, pBufferUAV);

    m_pCommandList->SetShaderResourceBufferView(binding, pBufferUAV);
  }
}

void xiiRenderContext::ApplyTextureUAVBindings()
{
  xiiSharedPtr<xiiGALPipelineResourceSignature> pResourceSignature;
  if (m_RenderContextScope == RenderContextScope::Graphics)
  {
    pResourceSignature = m_pGraphicsPipelineState->GetDescription().m_pPipelineResourceSignature;
  }
  else if (m_RenderContextScope == RenderContextScope::Compute)
  {
    pResourceSignature = m_pComputePipelineState->GetDescription().m_pPipelineResourceSignature;
  }

  const auto& resourceBindings = pResourceSignature->GetDescription().m_Resources;

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    if (binding.m_ResourceType != xiiGALShaderResourceType::TextureUAV)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiSharedPtr<xiiGALTextureView> pTextureUAV;
    m_BoundTextureUAVs.TryGetValue(uiResourceHash, pTextureUAV);

    m_pCommandList->SetShaderResourceTextureView(binding, pTextureUAV);
  }
}

void xiiRenderContext::ApplySamplerBindings()
{
  xiiSharedPtr<xiiGALPipelineResourceSignature> pResourceSignature;
  if (m_RenderContextScope == RenderContextScope::Graphics)
  {
    pResourceSignature = m_pGraphicsPipelineState->GetDescription().m_pPipelineResourceSignature;
  }
  else if (m_RenderContextScope == RenderContextScope::Compute)
  {
    pResourceSignature = m_pComputePipelineState->GetDescription().m_pPipelineResourceSignature;
  }

  const auto& resourceBindings = pResourceSignature->GetDescription().m_Resources;

  for (const auto& binding : pResourceSignature->GetDescription().m_Resources)
  {
    if (binding.m_ResourceType != xiiGALShaderResourceType::Sampler && binding.m_ResourceType != xiiGALShaderResourceType::TextureAndSampler)
      continue;

    const xiiUInt64 uiResourceHash = binding.m_sName.GetHash();

    xiiSharedPtr<xiiGALSampler> pSampler;
    m_BoundSamplers.TryGetValue(uiResourceHash, pSampler);

    m_pCommandList->SetSampler(binding, pSampler);
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

  if (!m_bIsRenderPassActive && !m_RenderingSetup.GetRenderPassDescription().m_Attachments.IsEmpty())
  {
    if (m_bNeedsClear)
    {
      BeginClearThenLoadInternalRenderPass();
    }
    else if (!m_pActiveRenderPass || m_pActiveRenderPass->GetDescription() != m_RenderingSetup.GetRenderPassDescription())
    {
      m_pActiveRenderPass = CreateInternalRenderPass(m_RenderingSetup.GetRenderPassDescription());
    }

    m_pCommandList->BeginRenderPass({m_pActiveRenderPass, GetCurrentFramebuffer()});

    m_bIsRenderPassActive = true;
  }
}

void xiiRenderContext::BeginClearThenLoadInternalRenderPass()
{
  XII_ASSERT_DEV(m_RenderContextScope == RenderContextScope::Graphics, "Render pass can only be begun in a graphics scope.");

  if (!m_bIsRenderPassActive && !m_RenderingSetup.GetRenderPassDescription().m_Attachments.IsEmpty())
  {
    if (m_bNeedsClear)
    {
      m_pActiveRenderPass = CreateInternalRenderPass(m_RenderingSetup.GetRenderPassDescription());

      m_pCommandList->BeginRenderPass({m_pActiveRenderPass, GetCurrentFramebuffer(), m_RenderingSetup.GetClearValues()});

      m_bNeedsClear         = false;
      m_bIsRenderPassActive = true;
    }

    EndInternalRenderPass();

    for (auto& attachment : m_RenderingSetup.m_RenderPassDescription.m_Attachments)
    {
      if (attachment.m_LoadOperation == xiiGALAttachmentLoadOperation::Clear)
      {
        attachment.m_LoadOperation = xiiGALAttachmentLoadOperation::Load;
      }
      if (attachment.m_StencilLoadOperation == xiiGALAttachmentLoadOperation::Clear)
      {
        attachment.m_StencilLoadOperation = xiiGALAttachmentLoadOperation::Load;
      }
    }

    m_pActiveRenderPass = CreateInternalRenderPass(m_RenderingSetup.GetRenderPassDescription());
  }
}

void xiiRenderContext::EndInternalRenderPass()
{
  if (m_bIsRenderPassActive)
  {
    m_pCommandList->EndRenderPass();

    m_bIsRenderPassActive = false;
  }
}

xiiResult xiiRenderContext::BuildInputLayout(xiiSharedPtr<xiiGALShader> pVertexShader, const xiiInputLayoutInfo& declaration, xiiSharedPtr<xiiGALInputLayout>& out_Declaration)
{
  ShaderVertexDeclaration vertexDeclaration;
  vertexDeclaration.m_pShader           = pVertexShader;
  vertexDeclaration.m_uiInputLayoutHash = declaration.m_uiHash;

  bool bExisted = false;
  auto it       = m_InputLayouts.FindOrAdd(vertexDeclaration, &bExisted);

  if (!bExisted)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

    xiiGALInputLayoutCreationDescription inputLayoutDescription;

    for (xiiUInt32 uiSlot = 0; uiSlot < declaration.m_VertexStreams.GetCount(); ++uiSlot)
    {
      auto& stream = declaration.m_VertexStreams[uiSlot];

      xiiGALLayoutElement& layoutElement     = inputLayoutDescription.m_LayoutElements.ExpandAndGetRef();
      layoutElement.m_Format                 = stream.m_Format;
      layoutElement.m_Semantic               = stream.m_Semantic;
      layoutElement.m_uiRelativeOffset       = stream.m_uiOffset;
      layoutElement.m_uiStride               = m_VertexBuffers[stream.m_uiVertexBufferSlot]->GetDescription().m_uiElementByteStride;
      layoutElement.m_uiBufferSlot           = stream.m_uiVertexBufferSlot;
      layoutElement.m_Frequency              = xiiGALInputElementFrequency::PerVertex;
      layoutElement.m_uiInstanceDataStepRate = 0;
    }

    out_Declaration = pVertexShader->CreateInputLayout(inputLayoutDescription);

    if (!out_Declaration)
    {
      /*
        This can happen when the resource system gives you a fallback resource, which then selects a shader that does not fit the mesh layout.
        E.g. when a material is not yet loaded and the fallback material is used, that fallback material may use another shader, that requires more data streams, than what the mesh provides.
        This problem will go away, once the proper material is loaded.
        
        This can be fixed by ensuring that the fallback material uses a shader that only requires data that is always there, e.g. only position and maybe a texcoord, and of course all meshes must provide at least those data streams.
        
        Otherwise, this is harmless, the renderer will ignore invalid drawcalls and once all the correct stuff is available, it will work.
      */

      xiiLog::Warning("Failed to create vertex input layout.");
      return XII_FAILURE;
    }

    it.Value() = out_Declaration;
  }

  out_Declaration = it.Value();
  return XII_SUCCESS;
}

void xiiRenderContext::SetGlobalAndWorldTimeConstants()
{
  xiiGlobalConstants* pGlobalConstants = GetGlobalConstants();

  // Wrap around to prevent floating point issues. A wrap around of 1000 allows all frequencies with 3 digits after the decimal.
  const double fWrapAround     = 1000.0;
  pGlobalConstants->DeltaTime  = (float)xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  pGlobalConstants->GlobalTime = (float)xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), fWrapAround);
  pGlobalConstants->WorldTime  = pGlobalConstants->GlobalTime;
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

// static
void xiiRenderContext::GALStaticDeviceEventHandler(const xiiGALDeviceEvent& e)
{
  if (e.m_Type == xiiGALDeviceEventType::BeforeBeginFrame)
  {
    if (s_pDefaultInstance)
    {
      s_pDefaultInstance->ResetContextState();
    }
  }
}
