#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/Blob.h>
#include <GraphicsCore/Meshes/DynamicMeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
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

  XII_ASSERT_DEBUG(!m_pGlobalConstants.IsEmpty(), "Invalid global constants buffer.");
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
  if (ApplyContextStates().Succeeded() || uiPrimitiveCount == 0U || uiInstanceCount == 0U)
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

  if (uiInstanceCount > 1)
  {
    if (m_pIndexBuffer)
    {
      return m_pCommandList->DrawIndexedInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
    else
    {
      return m_pCommandList->DrawInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
  }
  else
  {
    if (m_pIndexBuffer)
    {
      return m_pCommandList->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      return m_pCommandList->Draw(uiPrimitiveCount, uiFirstPrimitive);
    }
  }

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
    m_ActiveGALShaders[uiStageBitIndex] = pShaderPermutation->GetGALShader(xiiGALShaderType::GetStageFlag(uiStageBitIndex));

    XII_ASSERT_DEV(m_ActiveGALShaders[uiStageBitIndex] != nullptr, "Invalid GAL {} Shader handle.", xiiGALShaderType::Names[uiStageBitIndex]);
  }

  return pShaderPermutation;
}

xiiMaterialResource* xiiRenderContext::ApplyMaterialState()
{
  return nullptr;
}

void xiiRenderContext::ApplyConstantBufferBindings()
{
}

void xiiRenderContext::ApplyBufferSRVBindings()
{
}

void xiiRenderContext::ApplyTextureSRVBindings()
{
}

void xiiRenderContext::ApplyBufferUAVBindings()
{
}

void xiiRenderContext::ApplyTextureUAVBindings()
{
}

void xiiRenderContext::ApplySamplerBindings()
{
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

      m_pCommandList->BeginRenderPass({m_pActiveRenderPass, GetCurrentFramebuffer(), m_RenderingSetup.GetClearValues()});

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
