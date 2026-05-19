/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsD3D12/Pools/CommandListPoolD3D12.h>
#include <GraphicsD3D12/Pools/DescriptorSetPoolD3D12.h>
#include <GraphicsD3D12/Pools/DynamicBufferPoolD3D12.h>
#include <GraphicsD3D12/Pools/QueryPoolD3D12.h>
#include <GraphicsD3D12/Pools/StagingBufferPoolD3D12.h>
#include <GraphicsD3D12/Resources/BottomLevelASD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>
#include <GraphicsD3D12/Resources/FramebufferD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>
#include <GraphicsD3D12/States/ComputePipelineStateD3D12.h>
#include <GraphicsD3D12/States/GraphicsPipelineStateD3D12.h>
#include <GraphicsD3D12/States/PipelineResourceSignatureD3D12.h>
#include <GraphicsD3D12/States/RayTracingPipelineStateD3D12.h>
#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandListD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  [[nodiscard]] bool TransitionOrVerifyResourceStateForRayTracing(ID3D12GraphicsCommandList* pD3D12CommandList, xiiGALResource* pResource, ID3D12Resource* pD3D12Resource, xiiEnum<xiiGALStateTransitionMode> transitionMode, xiiBitflags<xiiGALResourceStateFlags> trackedState, D3D12_RESOURCE_STATES d3d12State, xiiStringView sUsageName, xiiStringView sCommandListName)
  {
    if (pD3D12CommandList == nullptr || pResource == nullptr || pD3D12Resource == nullptr)
      return false;

    const xiiBitflags<xiiGALResourceStateFlags> currentState = pResource->GetResourceState();

    if (transitionMode == xiiGALStateTransitionMode::Transition)
    {
      xiiBitflags<xiiGALResourceStateFlags> previousState = currentState;
      if (previousState == xiiGALResourceStateFlags::Unknown)
      {
        previousState = xiiGALResourceStateFlags::Common;
      }

      const D3D12_RESOURCE_STATES d3d12PreviousState = xiiD3D12TypeConversions::GetResourceState(previousState);
      if (d3d12PreviousState != d3d12State)
      {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = pD3D12Resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = d3d12PreviousState;
        barrier.Transition.StateAfter  = d3d12State;

        pD3D12CommandList->ResourceBarrier(1U, &barrier);
      }

      pResource->SetResourceState(trackedState);
    }
    else if (transitionMode == xiiGALStateTransitionMode::Verify)
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      if (currentState != xiiGALResourceStateFlags::Unknown && !currentState.AreAllSet(trackedState))
      {
        XII_ASSERT_DEV(false, "D3D12 resource state verification failed on command list '{}': '{}' is not in expected state mask 0x{:X}; current state mask is 0x{:X}.", sCommandListName, sUsageName, trackedState.GetValue(), currentState.GetValue());
      }
#endif
    }

    return true;
  }
} // namespace

xiiGALCommandListD3D12::xiiGALCommandListD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALCommandListD3D12::~xiiGALCommandListD3D12()
{
  Reset();
}

xiiResult xiiGALCommandListD3D12::InitPlatform()
{
  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::BeginPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12          = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiGALCommandListPoolD3D12*     pCommandListPoolD3D12 = pDeviceD3D12->GetCommandListPool(m_Description.m_QueueFlags);
  if (pCommandListPoolD3D12 == nullptr)
  {
    xiiLog::Error("Failed to begin D3D12 command list '{}': command-list pool is unavailable for queue flags {}.", GetDebugName(), xiiArgEnum(m_Description.m_QueueFlags));
    return;
  }

  m_CommandListData                            = {};
  m_CommandListData.m_pDynamicBufferPoolD3D12  = XII_NEW(pDeviceD3D12->GetAllocator(), xiiGALDynamicBufferPoolD3D12, pDeviceD3D12.Borrow(), 16U, xiiGALBindFlags::VertexBuffer | xiiGALBindFlags::IndexBuffer | xiiGALBindFlags::UniformBuffer | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments | xiiGALBindFlags::RayTracing);
  m_CommandListData.m_pUploadStagingBufferPool = XII_NEW(pDeviceD3D12->GetAllocator(), xiiGALStagingBufferPoolD3D12, pDeviceD3D12.Borrow(), 16U, xiiGALBindFlags::ShaderResource);
  m_CommandListData.m_pDescriptorSetPoolD3D12  = XII_NEW(pDeviceD3D12->GetAllocator(), xiiGALDescriptorSetPoolD3D12, pDeviceD3D12.Borrow(), 1024U);

  if (m_Description.m_Flags.IsSet(xiiGALCommandListFlags::Secondary))
  {
    m_CommandListAllocation = pCommandListPoolD3D12->AllocateSecondaryCommandList();
  }
  else
  {
    m_CommandListAllocation = pCommandListPoolD3D12->AllocatePrimaryCommandList();
  }

  m_pD3D12CommandAllocator = m_CommandListAllocation.GetCommandAllocator();
  m_pD3D12CommandList      = m_CommandListAllocation.GetCommandList();

  if (m_pD3D12CommandAllocator == nullptr || m_pD3D12CommandList == nullptr)
  {
    xiiLog::Error("Failed to begin D3D12 command list '{}': command allocator/list allocation failed.", GetDebugName());
    m_CommandListAllocation = {};
    m_CommandListData       = {};
    return;
  }

  if (FAILED(m_pD3D12CommandAllocator->Reset()))
  {
    xiiLog::Error("Failed to reset D3D12 command allocator for command list '{}'.", GetDebugName());
    m_CommandListAllocation  = {};
    m_CommandListData        = {};
    m_pD3D12CommandAllocator = nullptr;
    m_pD3D12CommandList      = nullptr;
    return;
  }

  if (FAILED(m_pD3D12CommandList->Reset(m_pD3D12CommandAllocator, nullptr)))
  {
    xiiLog::Error("Failed to reset D3D12 command list '{}'.", GetDebugName());
    m_CommandListAllocation  = {};
    m_CommandListData        = {};
    m_pD3D12CommandAllocator = nullptr;
    m_pD3D12CommandList      = nullptr;
    return;
  }

  if (xiiGALQueryPoolD3D12* pQueryPoolD3D12 = pDeviceD3D12->GetCommandQueueQueryPool(m_Description.m_QueueFlags))
  {
    pQueryPoolD3D12->ResetStaleQueries();
  }

  m_uiSubmittedFenceValue = 0ULL;
  m_RecordingState = RecordingState::Recording;
}

void xiiGALCommandListD3D12::EndPlatform()
{
  if (m_pD3D12CommandList != nullptr)
  {
    m_pD3D12CommandList->Close();
  }

  m_RecordingState = RecordingState::Ended;
}

void xiiGALCommandListD3D12::ResetPlatform()
{
  if (m_pD3D12CommandList != nullptr && m_RecordingState == RecordingState::Recording)
  {
    m_pD3D12CommandList->Close();
  }

  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12          = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  xiiGALCommandListPoolD3D12*     pCommandListPoolD3D12 = pDeviceD3D12->GetCommandListPool(m_Description.m_QueueFlags);

  if (m_CommandListAllocation.GetCommandList() != nullptr)
  {
    if (m_uiSubmittedFenceValue != 0ULL && pCommandListPoolD3D12 != nullptr)
    {
      pCommandListPoolD3D12->RecycleAfterSubmit(std::move(m_CommandListAllocation), std::move(m_CommandListData), m_uiSubmittedFenceValue);
    }
    else
    {
      m_CommandListData       = {};
      m_CommandListAllocation = {};
    }
  }

  m_CommandListAllocation  = {};
  m_pD3D12CommandAllocator = nullptr;
  m_pD3D12CommandList      = nullptr;
  m_CommandListData        = {};
  m_uiSubmittedFenceValue  = 0ULL;
  m_RecordingState = RecordingState::Reset;
}

void xiiGALCommandListD3D12::SubmitPlatform(xiiGALCommandList* pSecondaryCommandList)
{
  if (pSecondaryCommandList == nullptr || m_pD3D12CommandList == nullptr)
    return;

  xiiGALCommandListD3D12* pSecondaryCommandListD3D12 = xiiDynamicCast<xiiGALCommandListD3D12*>(pSecondaryCommandList);
  if (pSecondaryCommandListD3D12 == nullptr || pSecondaryCommandListD3D12->GetD3D12GraphicsCommandList() == nullptr)
    return;

  if (!pSecondaryCommandListD3D12->GetDescription().m_Flags.IsSet(xiiGALCommandListFlags::Secondary))
  {
    xiiLog::Warning("Ignoring D3D12 secondary command list submission for '{}': command list was not created with the Secondary flag.", pSecondaryCommandListD3D12->GetDebugName());
    return;
  }

  m_pD3D12CommandList->ExecuteBundle(pSecondaryCommandListD3D12->GetD3D12GraphicsCommandList());
}

void xiiGALCommandListD3D12::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  XII_IGNORE_UNUSED(pPipelineState);

  m_CommandListData.m_bPipelineStateModified = true;
}

void xiiGALCommandListD3D12::PushConstantsPlatform(xiiUInt32 uiOffset, xiiArrayPtr<const xiiUInt8> pData)
{
}

void xiiGALCommandListD3D12::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
}

void xiiGALCommandListD3D12::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
}

void xiiGALCommandListD3D12::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports)
{
}

void xiiGALCommandListD3D12::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)
{
}

void xiiGALCommandListD3D12::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset, xiiEnum<xiiGALStateTransitionMode> transitionMode)
{
}

void xiiGALCommandListD3D12::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<VertexStreamDescription> pVertexStreams, xiiBitflags<xiiGALSetVertexBufferFlags> flags, xiiEnum<xiiGALStateTransitionMode> transitionMode)
{
}

void xiiGALCommandListD3D12::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)
{
}

void xiiGALCommandListD3D12::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALCommandListD3D12::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALCommandListD3D12::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
}

void xiiGALCommandListD3D12::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
}

void xiiGALCommandListD3D12::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)
{
}

void xiiGALCommandListD3D12::SetAccelerationStructurePlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTopLevelAS* pTopLevelAS)
{
}

xiiResult xiiGALCommandListD3D12::CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode)
{

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
}

void xiiGALCommandListD3D12::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
}

void xiiGALCommandListD3D12::BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues)
{
}

void xiiGALCommandListD3D12::NextSubpassPlatform()
{
}

void xiiGALCommandListD3D12::EndRenderPassPlatform()
{
}

void xiiGALCommandListD3D12::DrawPlatform(const xiiGALDrawDescription& description)
{
}

void xiiGALCommandListD3D12::DrawIndexedPlatform(const xiiGALDrawIndexedDescription& description)
{
}

void xiiGALCommandListD3D12::DrawIndirectPlatform(const xiiGALDrawIndirectDescription& description)
{
}

void xiiGALCommandListD3D12::DrawIndexedIndirectPlatform(const xiiGALDrawIndexedIndirectDescription& description)
{
}

void xiiGALCommandListD3D12::DrawMeshPlatform(const xiiGALDrawMeshDescription& description)
{
}

void xiiGALCommandListD3D12::DrawMeshIndirectPlatform(const xiiGALDrawMeshIndirectDescription& description)
{
}

void xiiGALCommandListD3D12::MultiDrawPlatform(const xiiGALMultiDrawDescription& description)
{
}

void xiiGALCommandListD3D12::MultiDrawIndexedPlatform(const xiiGALMultiDrawIndexedDescription& description)
{
}

void xiiGALCommandListD3D12::DispatchComputePlatform(const xiiGALDispatchComputeDescription& description)
{
}

void xiiGALCommandListD3D12::DispatchComputeIndirectPlatform(const xiiGALDispatchComputeIndirectDescription& description)
{
}

void xiiGALCommandListD3D12::TraceRaysPlatform(const xiiGALTraceRaysDescription& description)
{
}

void xiiGALCommandListD3D12::TraceRaysIndirectPlatform(const xiiGALTraceRaysIndirectDescription& description)
{
}

void xiiGALCommandListD3D12::UpdateSBTPlatform(const xiiGALUpdateSBTDescription& description)
{
}

void xiiGALCommandListD3D12::BuildBLASPlatform(const xiiGALBuildBLASDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  xiiGALBottomLevelASD3D12* pBottomLevelASD3D12 = xiiDynamicCast<xiiGALBottomLevelASD3D12*>(description.m_pBottomLevelAS);
  xiiGALBufferD3D12*        pScratchBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pScratchBuffer);
  if (pBottomLevelASD3D12 == nullptr || pScratchBufferD3D12 == nullptr)
  {
    xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': incompatible BLAS/scratch backend resource types.", GetDebugName());
    return;
  }

  ID3D12Resource* pD3D12BLASResource    = pBottomLevelASD3D12->GetD3D12Resource();
  ID3D12Resource* pD3D12ScratchResource = pScratchBufferD3D12->GetD3D12Buffer();
  if (pD3D12BLASResource == nullptr || pD3D12ScratchResource == nullptr)
  {
    xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': BLAS/scratch native resources are unavailable.", GetDebugName());
    return;
  }

  ID3D12GraphicsCommandList4* pD3D12CommandList4 = nullptr;
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(IID_PPV_ARGS(&pD3D12CommandList4));
  if (FAILED(hResult) || pD3D12CommandList4 == nullptr)
  {
    xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': ID3D12GraphicsCommandList4 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList4);
    });

  const xiiGALBottomLevelASCreationDescription& blasDescription = description.m_pBottomLevelAS->GetDescription();

  if (description.m_Triangles.GetCount() != blasDescription.m_Triangles.GetCount() || description.m_BoundingBoxes.GetCount() != blasDescription.m_BoundingBoxes.GetCount())
  {
    xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': geometry build input counts do not match BLAS description counts.", GetDebugName());
    return;
  }

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pBottomLevelASD3D12, pD3D12BLASResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASWrite, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "BLAS destination resource", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pScratchBufferD3D12, pD3D12ScratchResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASWrite, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "BLAS scratch buffer", GetDebugName()))
    return;

  xiiTemporaryArray<D3D12_RAYTRACING_GEOMETRY_DESC> d3d12Geometries;
  d3d12Geometries.Reserve(blasDescription.m_Triangles.GetCount() + blasDescription.m_BoundingBoxes.GetCount());

  for (xiiUInt32 i = 0U; i < blasDescription.m_Triangles.GetCount(); ++i)
  {
    const xiiGALBLASTriangleDescription&      triangleDescription = blasDescription.m_Triangles[i];
    const xiiGALBLASTriangleBuildDescription& triangleBuildData   = description.m_Triangles[i];

    xiiGALBufferD3D12* pVertexBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(triangleBuildData.m_pVertexBuffer);
    if (pVertexBufferD3D12 == nullptr || pVertexBufferD3D12->GetD3D12Buffer() == nullptr)
    {
      xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': triangle geometry {} has an invalid vertex buffer.", GetDebugName(), i);
      return;
    }

    if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pVertexBufferD3D12, pVertexBufferD3D12->GetD3D12Buffer(), description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::ShaderResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "BLAS vertex buffer", GetDebugName()))
      return;

    xiiGALBufferD3D12* pIndexBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(triangleBuildData.m_pIndexBuffer);
    if (pIndexBufferD3D12 != nullptr)
    {
      if (pIndexBufferD3D12->GetD3D12Buffer() == nullptr)
      {
        xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': triangle geometry {} has an invalid index buffer.", GetDebugName(), i);
        return;
      }

      if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pIndexBufferD3D12, pIndexBufferD3D12->GetD3D12Buffer(), description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::ShaderResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "BLAS index buffer", GetDebugName()))
        return;
    }

    xiiGALBufferD3D12* pTransformBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(triangleBuildData.m_pTransformBuffer);
    if (pTransformBufferD3D12 != nullptr)
    {
      if (pTransformBufferD3D12->GetD3D12Buffer() == nullptr)
      {
        xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': triangle geometry {} has an invalid transform buffer.", GetDebugName(), i);
        return;
      }

      if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTransformBufferD3D12, pTransformBufferD3D12->GetD3D12Buffer(), description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::ShaderResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "BLAS transform buffer", GetDebugName()))
        return;
    }

    const xiiUInt32 uiPrimitiveCount = triangleBuildData.m_uiPrimitiveCount != 0U ? triangleBuildData.m_uiPrimitiveCount : triangleDescription.m_uiMaxPrimitiveCount;
    const xiiUInt64 uiVertexStride   = triangleBuildData.m_uiVertexStride != 0U ? triangleBuildData.m_uiVertexStride : xiiD3D12TypeConversions::GetBLASTriangleVertexStride(triangleDescription);

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDescription       = {};
    geometryDescription.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDescription.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geometryDescription.Triangles.Transform3x4               = pTransformBufferD3D12 != nullptr ? (pTransformBufferD3D12->GetD3D12BufferGPUVirtualAddress() + triangleBuildData.m_uiTransformOffset) : 0ULL;
    geometryDescription.Triangles.IndexFormat                = pIndexBufferD3D12 != nullptr ? xiiD3D12TypeConversions::GetBLASIndexFormat(triangleDescription.m_IndexType) : DXGI_FORMAT_UNKNOWN;
    geometryDescription.Triangles.VertexFormat               = xiiD3D12TypeConversions::GetBLASTriangleVertexFormat(triangleDescription);
    geometryDescription.Triangles.IndexCount                 = pIndexBufferD3D12 != nullptr ? uiPrimitiveCount * 3U : 0U;
    geometryDescription.Triangles.VertexCount                = triangleDescription.m_uiMaxVertexCount;
    geometryDescription.Triangles.IndexBuffer                = pIndexBufferD3D12 != nullptr ? (pIndexBufferD3D12->GetD3D12BufferGPUVirtualAddress() + triangleBuildData.m_uiIndexBufferOffset) : 0ULL;
    geometryDescription.Triangles.VertexBuffer.StartAddress  = pVertexBufferD3D12->GetD3D12BufferGPUVirtualAddress() + triangleBuildData.m_uiVertexBufferOffset;
    geometryDescription.Triangles.VertexBuffer.StrideInBytes = static_cast<UINT>(uiVertexStride);

    if (geometryDescription.Triangles.VertexFormat == DXGI_FORMAT_UNKNOWN || geometryDescription.Triangles.VertexBuffer.StrideInBytes == 0U)
    {
      xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': triangle geometry {} has unsupported vertex format/stride.", GetDebugName(), i);
      return;
    }

    d3d12Geometries.PushBack(geometryDescription);
  }

  for (xiiUInt32 i = 0U; i < blasDescription.m_BoundingBoxes.GetCount(); ++i)
  {
    const xiiGALBLASBoundingBoxDescription&      boxDescription = blasDescription.m_BoundingBoxes[i];
    const xiiGALBLASBoundingBoxBuildDescription& boxBuildData   = description.m_BoundingBoxes[i];

    xiiGALBufferD3D12* pBoundingBoxBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(boxBuildData.m_pBoundingBoxBuffer);
    if (pBoundingBoxBufferD3D12 == nullptr || pBoundingBoxBufferD3D12->GetD3D12Buffer() == nullptr)
    {
      xiiLog::Error("Failed to build BLAS on D3D12 command list '{}': AABB geometry {} has an invalid source buffer.", GetDebugName(), i);
      return;
    }

    if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pBoundingBoxBufferD3D12, pBoundingBoxBufferD3D12->GetD3D12Buffer(), description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::ShaderResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "BLAS AABB buffer", GetDebugName()))
      return;

    const xiiUInt32 uiBoxCount = boxBuildData.m_uiBoxCount != 0U ? boxBuildData.m_uiBoxCount : boxDescription.m_uiMaxBoxCount;
    const xiiUInt64 uiStride   = boxBuildData.m_uiBoundingBoxStride != 0U ? boxBuildData.m_uiBoundingBoxStride : sizeof(float) * 6ULL;

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDescription   = {};
    geometryDescription.Type                             = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    geometryDescription.Flags                            = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geometryDescription.AABBs.AABBCount                  = uiBoxCount;
    geometryDescription.AABBs.AABBs.StartAddress         = pBoundingBoxBufferD3D12->GetD3D12BufferGPUVirtualAddress() + boxBuildData.m_uiBoundingBoxOffset;
    geometryDescription.AABBs.AABBs.StrideInBytes        = uiStride;

    d3d12Geometries.PushBack(geometryDescription);
  }

  xiiBitflags<xiiGALRayTracingBuildASFlags> buildFlags = description.m_BuildFlags.IsAnyFlagSet() ? description.m_BuildFlags : blasDescription.m_BuildASFlags;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS d3d12BuildFlags = xiiD3D12TypeConversions::GetAccelerationStructureBuildFlags(buildFlags);
  if (description.m_bUpdate)
  {
    d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3d12BuildDescription = {};
  d3d12BuildDescription.Inputs.Type                                         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  d3d12BuildDescription.Inputs.DescsLayout                                  = D3D12_ELEMENTS_LAYOUT_ARRAY;
  d3d12BuildDescription.Inputs.Flags                                        = d3d12BuildFlags;
  d3d12BuildDescription.Inputs.NumDescs                                     = d3d12Geometries.GetCount();
  d3d12BuildDescription.Inputs.pGeometryDescs                               = d3d12Geometries.GetData();
  d3d12BuildDescription.SourceAccelerationStructureData                     = description.m_bUpdate ? pBottomLevelASD3D12->GetD3D12GPUVirtualAddress() : 0ULL;
  d3d12BuildDescription.DestAccelerationStructureData                       = pBottomLevelASD3D12->GetD3D12GPUVirtualAddress();
  d3d12BuildDescription.ScratchAccelerationStructureData                    = pScratchBufferD3D12->GetD3D12BufferGPUVirtualAddress() + description.m_uiScratchBufferOffset;

  pD3D12CommandList4->BuildRaytracingAccelerationStructure(&d3d12BuildDescription, 0U, nullptr);

  D3D12_RESOURCE_BARRIER uavBarriers[2] = {};
  uavBarriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  uavBarriers[0].UAV.pResource          = pD3D12BLASResource;
  uavBarriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  uavBarriers[1].UAV.pResource          = pD3D12ScratchResource;
  m_pD3D12CommandList->ResourceBarrier(2U, uavBarriers);
}

void xiiGALCommandListD3D12::BuildTLASPlatform(const xiiGALBuildTLASDescription& description)
{
}

void xiiGALCommandListD3D12::CopyBLASPlatform(const xiiGALCopyBLASDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  xiiGALBottomLevelASD3D12* pSourceBottomLevelASD3D12      = xiiDynamicCast<xiiGALBottomLevelASD3D12*>(description.m_pSourceBottomLevelAS);
  xiiGALBottomLevelASD3D12* pDestinationBottomLevelASD3D12 = xiiDynamicCast<xiiGALBottomLevelASD3D12*>(description.m_pDestinationBottomLevelAS);
  if (pSourceBottomLevelASD3D12 == nullptr || pDestinationBottomLevelASD3D12 == nullptr)
  {
    xiiLog::Error("Failed to copy BLAS on D3D12 command list '{}': incompatible source or destination backend type.", GetDebugName());
    return;
  }

  ID3D12Resource* pD3D12SourceResource      = pSourceBottomLevelASD3D12->GetD3D12Resource();
  ID3D12Resource* pD3D12DestinationResource = pDestinationBottomLevelASD3D12->GetD3D12Resource();
  if (pD3D12SourceResource == nullptr || pD3D12DestinationResource == nullptr)
  {
    xiiLog::Error("Failed to copy BLAS on D3D12 command list '{}': source or destination native resource is unavailable.", GetDebugName());
    return;
  }

  ID3D12GraphicsCommandList4* pD3D12CommandList4 = nullptr;
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(IID_PPV_ARGS(&pD3D12CommandList4));
  if (FAILED(hResult) || pD3D12CommandList4 == nullptr)
  {
    xiiLog::Error("Failed to copy BLAS on D3D12 command list '{}': ID3D12GraphicsCommandList4 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList4);
    });

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pSourceBottomLevelASD3D12, pD3D12SourceResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "BLAS copy source", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDestinationBottomLevelASD3D12, pD3D12DestinationResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASWrite, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "BLAS copy destination", GetDebugName()))
    return;

  const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE d3d12CopyMode = description.m_Mode == xiiGALASCopyMode::Compact ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;

  pD3D12CommandList4->CopyRaytracingAccelerationStructure(pDestinationBottomLevelASD3D12->GetD3D12GPUVirtualAddress(), pSourceBottomLevelASD3D12->GetD3D12GPUVirtualAddress(), d3d12CopyMode);
}

void xiiGALCommandListD3D12::CopyTLASPlatform(const xiiGALCopyTLASDescription& description)
{
}

void xiiGALCommandListD3D12::WriteBLASCompactedSizePlatform(const xiiGALWriteBLASCompactedSizeDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  xiiGALBottomLevelASD3D12* pBottomLevelASD3D12     = xiiDynamicCast<xiiGALBottomLevelASD3D12*>(description.m_pBottomLevelAS);
  xiiGALBufferD3D12*        pDestinationBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pDestinationBuffer);
  if (pBottomLevelASD3D12 == nullptr || pDestinationBufferD3D12 == nullptr)
  {
    xiiLog::Error("Failed to write BLAS compacted size on D3D12 command list '{}': incompatible BLAS/destination backend types.", GetDebugName());
    return;
  }

  ID3D12Resource* pD3D12BLASResource              = pBottomLevelASD3D12->GetD3D12Resource();
  ID3D12Resource* pD3D12DestinationBufferResource = pDestinationBufferD3D12->GetD3D12Buffer();
  if (pD3D12BLASResource == nullptr || pD3D12DestinationBufferResource == nullptr)
  {
    xiiLog::Error("Failed to write BLAS compacted size on D3D12 command list '{}': BLAS/destination native resources are unavailable.", GetDebugName());
    return;
  }

  ID3D12GraphicsCommandList4* pD3D12CommandList4 = nullptr;
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(IID_PPV_ARGS(&pD3D12CommandList4));
  if (FAILED(hResult) || pD3D12CommandList4 == nullptr)
  {
    xiiLog::Error("Failed to write BLAS compacted size on D3D12 command list '{}': ID3D12GraphicsCommandList4 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList4);
    });

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pBottomLevelASD3D12, pD3D12BLASResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "BLAS compacted-size source", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDestinationBufferD3D12, pD3D12DestinationBufferResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::UnorderedAccess, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "BLAS compacted-size destination buffer", GetDebugName()))
    return;

  const D3D12_GPU_VIRTUAL_ADDRESS d3d12DestinationAddress = pDestinationBufferD3D12->GetD3D12BufferGPUVirtualAddress() + description.m_uiDestinationBufferOffset;
  const D3D12_GPU_VIRTUAL_ADDRESS d3d12SourceAddress      = pBottomLevelASD3D12->GetD3D12GPUVirtualAddress();
  if (d3d12DestinationAddress == 0ULL || d3d12SourceAddress == 0ULL)
  {
    xiiLog::Error("Failed to write BLAS compacted size on D3D12 command list '{}': invalid source or destination GPU virtual address.", GetDebugName());
    return;
  }

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildDescription = {};
  postBuildDescription.DestBuffer                                                    = d3d12DestinationAddress;
  postBuildDescription.InfoType                                                      = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

  pD3D12CommandList4->EmitRaytracingAccelerationStructurePostbuildInfo(&postBuildDescription, 1U, &d3d12SourceAddress);
}

void xiiGALCommandListD3D12::WriteTLASCompactedSizePlatform(const xiiGALWriteTLASCompactedSizeDescription& description)
{
}

void xiiGALCommandListD3D12::BeginQueryPlatform(xiiGALQuery* pQuery)
{
  if (m_pD3D12CommandList == nullptr || pQuery == nullptr)
    return;

  xiiGALQueryD3D12* pQueryD3D12 = xiiDynamicCast<xiiGALQueryD3D12*>(pQuery);
  if (pQueryD3D12 == nullptr)
  {
    xiiLog::Error("Failed to begin D3D12 query on command list '{}': incompatible query backend type.", GetDebugName());
    return;
  }

  const xiiGALQueryType::Enum queryType = pQueryD3D12->GetDescription().m_Type;
  if (queryType == xiiGALQueryType::Timestamp)
  {
    xiiLog::Error("BeginQuery() is not supported for timestamp queries in D3D12. Use EndQuery() to write timestamp values.");
    return;
  }

  if (!pQueryD3D12->OnBeginQuery(this))
    return;

  xiiGALQueryPoolD3D12* pQueryPoolD3D12 = pQueryD3D12->GetQueryPoolD3D12();
  if (pQueryPoolD3D12 == nullptr)
  {
    xiiLog::Error("Failed to begin D3D12 query '{}': query pool is unavailable.", pQueryD3D12->GetDebugName());
    return;
  }

  ID3D12QueryHeap* pD3D12QueryHeap = pQueryPoolD3D12->GetQueryHeap(queryType);
  if (pD3D12QueryHeap == nullptr)
  {
    xiiLog::Error("Failed to begin D3D12 query '{}': query heap is unavailable for type {}.", pQueryD3D12->GetDebugName(), xiiArgEnum(xiiEnum<xiiGALQueryType>(queryType)));
    return;
  }

  const xiiUInt32 uiBeginQueryIndex = pQueryD3D12->GetQueryPoolIndex(0U);
  if (uiBeginQueryIndex == xiiInvalidIndex)
  {
    xiiLog::Error("Failed to begin D3D12 query '{}': query index allocation failed.", pQueryD3D12->GetDebugName());
    return;
  }

  const D3D12_QUERY_TYPE d3d12QueryType = pQueryPoolD3D12->GetD3D12QueryType(queryType);
  if (queryType == xiiGALQueryType::Duration)
  {
    m_pD3D12CommandList->EndQuery(pD3D12QueryHeap, d3d12QueryType, uiBeginQueryIndex);
  }
  else
  {
    ++m_CommandListData.m_uiActiveQueriesCounter;
    m_pD3D12CommandList->BeginQuery(pD3D12QueryHeap, d3d12QueryType, uiBeginQueryIndex);
  }
}

void xiiGALCommandListD3D12::EndQueryPlatform(xiiGALQuery* pQuery)
{
  if (m_pD3D12CommandList == nullptr || pQuery == nullptr)
    return;

  xiiGALQueryD3D12* pQueryD3D12 = xiiDynamicCast<xiiGALQueryD3D12*>(pQuery);
  if (pQueryD3D12 == nullptr)
  {
    xiiLog::Error("Failed to end D3D12 query on command list '{}': incompatible query backend type.", GetDebugName());
    return;
  }

  if (!pQueryD3D12->OnEndQuery(this))
    return;

  const xiiGALQueryType::Enum queryType = pQueryD3D12->GetDescription().m_Type;
  xiiGALQueryPoolD3D12*       pQueryPoolD3D12 = pQueryD3D12->GetQueryPoolD3D12();
  if (pQueryPoolD3D12 == nullptr)
  {
    xiiLog::Error("Failed to end D3D12 query '{}': query pool is unavailable.", pQueryD3D12->GetDebugName());
    return;
  }

  ID3D12QueryHeap* pD3D12QueryHeap = pQueryPoolD3D12->GetQueryHeap(queryType);
  ID3D12Resource*  pReadbackBuffer = pQueryPoolD3D12->GetReadbackBuffer(queryType);
  if (pD3D12QueryHeap == nullptr || pReadbackBuffer == nullptr)
  {
    xiiLog::Error("Failed to end D3D12 query '{}': query heap/readback resources are unavailable for type {}.", pQueryD3D12->GetDebugName(), xiiArgEnum(xiiEnum<xiiGALQueryType>(queryType)));
    return;
  }

  const D3D12_QUERY_TYPE d3d12QueryType = pQueryPoolD3D12->GetD3D12QueryType(queryType);
  if (queryType == xiiGALQueryType::Timestamp)
  {
    const xiiUInt32 uiQueryIndex = pQueryD3D12->GetQueryPoolIndex(0U);
    if (uiQueryIndex == xiiInvalidIndex)
    {
      xiiLog::Error("Failed to end D3D12 timestamp query '{}': query index allocation failed.", pQueryD3D12->GetDebugName());
      return;
    }

    m_pD3D12CommandList->EndQuery(pD3D12QueryHeap, d3d12QueryType, uiQueryIndex);
    m_pD3D12CommandList->ResolveQueryData(pD3D12QueryHeap, d3d12QueryType, uiQueryIndex, 1U, pReadbackBuffer, pQueryPoolD3D12->GetQueryReadbackOffset(queryType, uiQueryIndex));
    return;
  }

  if (queryType == xiiGALQueryType::Duration)
  {
    const xiiUInt32 uiStartQueryIndex = pQueryD3D12->GetQueryPoolIndex(0U);
    const xiiUInt32 uiEndQueryIndex   = pQueryD3D12->GetQueryPoolIndex(1U);
    if (uiStartQueryIndex == xiiInvalidIndex || uiEndQueryIndex == xiiInvalidIndex)
    {
      xiiLog::Error("Failed to end D3D12 duration query '{}': query index allocation failed.", pQueryD3D12->GetDebugName());
      return;
    }

    m_pD3D12CommandList->EndQuery(pD3D12QueryHeap, d3d12QueryType, uiEndQueryIndex);
    m_pD3D12CommandList->ResolveQueryData(pD3D12QueryHeap, d3d12QueryType, uiStartQueryIndex, 1U, pReadbackBuffer, pQueryPoolD3D12->GetQueryReadbackOffset(queryType, uiStartQueryIndex));
    m_pD3D12CommandList->ResolveQueryData(pD3D12QueryHeap, d3d12QueryType, uiEndQueryIndex, 1U, pReadbackBuffer, pQueryPoolD3D12->GetQueryReadbackOffset(queryType, uiEndQueryIndex));
    return;
  }

  if (m_CommandListData.m_uiActiveQueriesCounter == 0U)
  {
    xiiLog::Warning("Ending D3D12 query '{}' with no active non-timestamp queries tracked on command list '{}'.", pQueryD3D12->GetDebugName(), GetDebugName());
  }
  else
  {
    --m_CommandListData.m_uiActiveQueriesCounter;
  }

  const xiiUInt32 uiQueryIndex = pQueryD3D12->GetQueryPoolIndex(0U);
  if (uiQueryIndex == xiiInvalidIndex)
  {
    xiiLog::Error("Failed to end D3D12 query '{}': query index allocation failed.", pQueryD3D12->GetDebugName());
    return;
  }

  m_pD3D12CommandList->EndQuery(pD3D12QueryHeap, d3d12QueryType, uiQueryIndex);
  m_pD3D12CommandList->ResolveQueryData(pD3D12QueryHeap, d3d12QueryType, uiQueryIndex, 1U, pReadbackBuffer, pQueryPoolD3D12->GetQueryReadbackOffset(queryType, uiQueryIndex));
}

void xiiGALCommandListD3D12::UpdateBufferPlatform(xiiGALBuffer* pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData)
{
}

void xiiGALCommandListD3D12::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
}

void xiiGALCommandListD3D12::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
}

xiiResult xiiGALCommandListD3D12::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
}

void xiiGALCommandListD3D12::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
}

void xiiGALCommandListD3D12::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
}

void xiiGALCommandListD3D12::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture, const xiiGALResolveTextureSubresourceDescription& description)
{
}

xiiResult xiiGALCommandListD3D12::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
  XII_IGNORE_UNUSED(pTextureView);

  xiiLog::Error("GenerateMips is currently unsupported in the D3D12 backend.");
}

void xiiGALCommandListD3D12::TransitionResourceStatesPlatform(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers)
{
  for (xiiGALStateTransitionDescription& barrier : pResourceBarriers)
  {
    if (barrier.m_pResource == nullptr || !barrier.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState))
      continue;

    barrier.m_pResource->SetResourceState(barrier.m_NewState);
  }
}

void xiiGALCommandListD3D12::SetShadingRatePlatform(xiiBitflags<xiiGALShadingRateFlags> baseRateFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> primitiveCombinerFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> textureCombinerFlags)
{
}

void xiiGALCommandListD3D12::EnqueueSignalPlatform(xiiGALFence* pFence, xiiUInt64 uiValue)
{
}

void xiiGALCommandListD3D12::DeviceWaitForFencePlatform(xiiGALFence* pFence, xiiUInt64 uiValue)
{
}

void xiiGALCommandListD3D12::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
}

void xiiGALCommandListD3D12::EndDebugGroupPlatform()
{
}

void xiiGALCommandListD3D12::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
}

void xiiGALCommandListD3D12::InvalidateStatePlatform()
{
  m_CommandListData.Invalidate();
}

void xiiGALCommandListD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiStringBuilder sb;
  const char*      szName       = sName.GetData(sb);
  const xiiUInt32  uiNameLength = static_cast<xiiUInt32>(sName.GetElementCount());

  if (m_pD3D12CommandAllocator != nullptr)
  {
    if (FAILED(m_pD3D12CommandAllocator->SetPrivateData(WKPDID_D3DDebugObjectName, uiNameLength, szName)))
    {
      xiiLog::Error("Failed to set the D3D12 command allocator debug name.");
    }
  }

  if (m_pD3D12CommandList != nullptr)
  {
    if (FAILED(m_pD3D12CommandList->SetPrivateData(WKPDID_D3DDebugObjectName, uiNameLength, szName)))
    {
      xiiLog::Error("Failed to set the D3D12 command list debug name.");
    }
  }
}

void xiiGALCommandListD3D12::PrepareForDraw()
{
}

void xiiGALCommandListD3D12::PrepareForIndexedDraw(xiiEnum<xiiGALValueType> indexType)
{
}

void xiiGALCommandListD3D12::PrepareForDispatchCompute()
{
}

void xiiGALCommandListD3D12::PrepareForRayTracing()
{
}

[[nodiscard]] inline bool ResourceStateHasWriteAccess(xiiBitflags<xiiGALResourceStateFlags> flags)
{
  xiiBitflags<xiiGALResourceStateFlags> writeAccessStates = xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::BuildASWrite;

  return writeAccessStates.IsAnySet(flags);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandListD3D12);
