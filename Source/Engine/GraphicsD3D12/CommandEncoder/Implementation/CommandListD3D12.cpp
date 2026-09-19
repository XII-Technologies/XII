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
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>
#include <GraphicsD3D12/Resources/FramebufferD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>
#include <GraphicsD3D12/Resources/RenderPassD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>
#include <GraphicsD3D12/States/ComputePipelineStateD3D12.h>
#include <GraphicsD3D12/States/GraphicsPipelineStateD3D12.h>
#include <GraphicsD3D12/States/PipelineResourceSignatureD3D12.h>
#include <GraphicsD3D12/States/RayTracingPipelineStateD3D12.h>
#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

#if BUILDSYSTEM_ENABLE_PIX_EVENT_RUNTIME_SUPPORT
// PIX instrumentation is only enabled if one of the preprocessor symbols USE_PIX, DBG, _DEBUG, PROFILE, or PROFILE_BUILD is defined.
#  define USE_PIX
#  include <WinPixEventRuntime/include/pix3.h>
#endif

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandListD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  [[nodiscard]] ID3D12CommandSignature* CreateIndirectCommandSignature(ID3D12Device* pD3D12Device, D3D12_INDIRECT_ARGUMENT_TYPE argumentType, xiiUInt32 uiByteStride)
  {
    XII_ASSERT_DEBUG(pD3D12Device != nullptr, "Invalid D3D12 device pointer.");

    D3D12_INDIRECT_ARGUMENT_DESC argumentDescription = {};
    argumentDescription.Type                         = argumentType;

    D3D12_COMMAND_SIGNATURE_DESC signatureDescription = {};
    signatureDescription.ByteStride                   = uiByteStride;
    signatureDescription.NumArgumentDescs             = 1U;
    signatureDescription.pArgumentDescs               = &argumentDescription;
    signatureDescription.NodeMask                     = 0U;

    ID3D12CommandSignature* pCommandSignature = nullptr;
    if (FAILED(pD3D12Device->CreateCommandSignature(&signatureDescription, nullptr, IID_PPV_ARGS(&pCommandSignature))))
      return nullptr;

    return pCommandSignature;
  }

  enum class D3D12PipelineRootBindingType : xiiUInt8
  {
    None = 0U,
    Graphics,
    Compute,
  };

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

  [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE OffsetCPUDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle, xiiUInt32 uiDescriptorSize, xiiUInt32 uiDescriptorIndex)
  {
    handle.ptr += static_cast<SIZE_T>(uiDescriptorSize) * static_cast<SIZE_T>(uiDescriptorIndex);
    return handle;
  }

  [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE OffsetGPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle, xiiUInt32 uiDescriptorSize, xiiUInt32 uiDescriptorIndex)
  {
    handle.ptr += static_cast<UINT64>(uiDescriptorSize) * static_cast<UINT64>(uiDescriptorIndex);
    return handle;
  }

  [[nodiscard]] bool IsValidDescriptorAllocation(const xiiGALDescriptorSetPoolD3D12::DescriptorAllocation& descriptorAllocation)
  {
    return descriptorAllocation.m_pDescriptorHeap != nullptr && descriptorAllocation.m_CPUHandle.ptr != 0U && descriptorAllocation.m_GPUHandle.ptr != 0U && descriptorAllocation.m_uiDescriptorSize != 0U;
  }

  [[nodiscard]] bool SubpassUsesAttachment(const xiiGALSubPassDescription& subpass, xiiUInt32 uiAttachmentIndex)
  {
    for (const xiiGALAttachmentReferenceDescription& attachmentReference : subpass.m_InputAttachments)
    {
      if (attachmentReference.m_uiAttachmentIndex == uiAttachmentIndex)
        return true;
    }

    for (const xiiGALAttachmentReferenceDescription& attachmentReference : subpass.m_RenderTargetAttachments)
    {
      if (attachmentReference.m_uiAttachmentIndex == uiAttachmentIndex)
        return true;
    }

    for (const xiiGALAttachmentReferenceDescription& attachmentReference : subpass.m_ResolveAttachments)
    {
      if (attachmentReference.m_uiAttachmentIndex == uiAttachmentIndex)
        return true;
    }

    if (!subpass.m_DepthStencilAttachment.IsEmpty() && subpass.m_DepthStencilAttachment[0].m_uiAttachmentIndex == uiAttachmentIndex)
      return true;

    if (!subpass.m_DepthResolveAttachment.IsEmpty() && subpass.m_DepthResolveAttachment[0].m_Attachment.m_uiAttachmentIndex == uiAttachmentIndex)
      return true;

    if (!subpass.m_ShadingRateAttachment.IsEmpty() && subpass.m_ShadingRateAttachment[0].m_AttachmentReference.m_uiAttachmentIndex == uiAttachmentIndex)
      return true;

    for (xiiUInt32 uiPreserveAttachmentIndex : subpass.m_PreserveAttachments)
    {
      if (uiPreserveAttachmentIndex == uiAttachmentIndex)
        return true;
    }

    return false;
  }

  [[nodiscard]] bool WasAttachmentUsedInPreviousSubpass(const xiiGALRenderPassCreationDescription& renderPassDescription, xiiUInt32 uiAttachmentIndex, xiiUInt32 uiCurrentSubpassIndex)
  {
    for (xiiUInt32 uiSubpassIndex = 0U; uiSubpassIndex < uiCurrentSubpassIndex; ++uiSubpassIndex)
    {
      if (SubpassUsesAttachment(renderPassDescription.m_SubPasses[uiSubpassIndex], uiAttachmentIndex))
        return true;
    }

    return false;
  }

#if BUILDSYSTEM_ENABLE_PIX_EVENT_RUNTIME_SUPPORT
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 xiiColorToPixColor(const xiiColor& color)
  {
    return (static_cast<xiiUInt32>(color.r * 255.0f) << 24) | (static_cast<xiiUInt32>(color.g * 255.0f) << 16) | (static_cast<xiiUInt32>(color.b * 255.0f) << 8) | static_cast<xiiUInt32>(color.a * 255.0f);
  }
#endif
} // namespace

///////////////////////////////////////////////////////////////////////////

class xiiLocalStateTransitionHelper
{
public:
  xiiLocalStateTransitionHelper(const xiiGALStateTransitionDescription& description, xiiGALCommandListD3D12* pCommandListD3D12) :
    m_Description(description), m_pCommandListD3D12(pCommandListD3D12), m_ResourceStateMask(xiiD3D12TypeConversions::GetSupportedD3D12ResourceStatesForCommandList(pCommandListD3D12->GetDescription().m_QueueFlags))
  {
  }

  XII_ALWAYS_INLINE static ID3D12Resource* GetD3D12Resource(const xiiGALResource* pResource)
  {
    if (auto pTextureD3D12 = xiiDynamicCast<const xiiGALTextureD3D12*>(pResource))
      return pTextureD3D12->GetD3D12Texture();

    if (auto pBuffer = xiiDynamicCast<const xiiGALBufferD3D12*>(pResource))
      return pBuffer->GetD3D12Buffer();

    if (auto pTopLevelAS = xiiDynamicCast<const xiiGALTopLevelASD3D12*>(pResource))
      return pTopLevelAS->GetD3D12Resource();

    if (auto pBottomLevelAS = xiiDynamicCast<const xiiGALBottomLevelASD3D12*>(pResource))
      return pBottomLevelAS->GetD3D12Resource();

    return nullptr;
  }

  XII_ALWAYS_INLINE static xiiUInt32 ResolveMipCount(xiiGALTextureD3D12* pTextureD3D12, xiiUInt32 uiMipCount) noexcept
  {
    if (pTextureD3D12)
    {
      const xiiGALTextureCreationDescription& description = pTextureD3D12->GetDescription();

      if (uiMipCount == XII_GAL_REMAINING_MIP_LEVELS)
        return description.m_uiMipLevels;

      return (uiMipCount > description.m_uiMipLevels) ? description.m_uiMipLevels : uiMipCount;
    }
    return 0;
  }

  XII_ALWAYS_INLINE static xiiUInt32 ResolveArrayCount(xiiGALTextureD3D12* pTextureD3D12, xiiUInt32 uiArrayCount) noexcept
  {
    if (pTextureD3D12)
    {
      const xiiGALTextureCreationDescription& description = pTextureD3D12->GetDescription();

      if (uiArrayCount == XII_GAL_REMAINING_ARRAY_SLICES)
        return description.m_uiArraySizeOrDepth;

      return (uiArrayCount > description.m_uiArraySizeOrDepth) ? description.m_uiArraySizeOrDepth : uiArrayCount;
    }
    return 0;
  }

  XII_ALWAYS_INLINE static bool IsWholeResource(xiiGALTextureD3D12* pTextureD3D12, xiiUInt32 uiFirstMip, xiiUInt32 uiMipCount, xiiUInt32 uiFirstSlice, xiiUInt32 uiSliceCount) noexcept
  {
    if (pTextureD3D12)
    {
      const xiiGALTextureCreationDescription& description          = pTextureD3D12->GetDescription();
      const xiiUInt32                         uiResolvedMipCount   = ResolveMipCount(pTextureD3D12, uiMipCount);
      const xiiUInt32                         uiResolvedSliceCount = ResolveArrayCount(pTextureD3D12, uiSliceCount);

      return uiFirstMip == 0 && uiResolvedMipCount == description.m_uiMipLevels && uiFirstSlice == 0 && uiResolvedSliceCount == description.m_uiArraySizeOrDepth;
    }
    return false;
  }

  void Process()
  {
    // Resolve old state: if Unknown, use resource tracked state.
    xiiBitflags<xiiGALResourceStateFlags> oldState = m_Description.m_OldState;

    if (oldState == xiiGALResourceStateFlags::Unknown)
    {
      oldState = m_Description.m_pResource->GetResourceState();

      XII_ASSERT_DEV(oldState != xiiGALResourceStateFlags::Unknown, "Resource '{}' has unknown tracked state, and OldState was also Unknown.", m_Description.m_pResource->GetDebugName());
    }
    else
    {
      xiiBitflags<xiiGALResourceStateFlags> tracked = m_Description.m_pResource->GetResourceState();

      XII_ASSERT_DEV(tracked == xiiGALResourceStateFlags::Unknown || tracked == oldState, "Transition: resource '{}' tracked state ({}) differs from provided OldState ({}).", m_Description.m_pResource->GetDebugName(), xiiArgEnum(tracked), xiiArgEnum(oldState));
    }

    // Determine if UAV barrier is required (both old and new are UAV-like)
    // RESOURCE_STATE_UNORDERED_ACCESS and RESOURCE_STATE_BUILD_AS_WRITE are converted to D3D12_RESOURCE_STATE_UNORDERED_ACCESS.
    // UAV barrier must be inserted between D3D12_RESOURCE_STATE_UNORDERED_ACCESS resource usages.
    const bool bOldStateIsUAV      = oldState.IsAnySet(xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::BuildASWrite);
    const bool bNewStateIsUAV      = m_Description.m_NewState.IsAnySet(xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::BuildASWrite);
    const bool bRequiresUAVBarrier = bOldStateIsUAV && bNewStateIsUAV;

    // If the new state is already covered by old state, nothing to do except maybe update tracked state.
    if ((oldState & m_Description.m_NewState) == m_Description.m_NewState)
    {
      if (m_Description.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState))
      {
        // Update tracked state only if allowed by transition type.
        if (m_Description.m_TransitionType == xiiGALStateTransitionType::Begin)
        {
          xiiLog::Error("UpdateState cannot be used with Begin split barrier for resource '{}'.", m_Description.m_pResource->GetDebugName());
        }
        else
        {
          m_Description.m_pResource->SetResourceState(m_Description.m_NewState);
        }
      }

      // Still may need UAV barrier if both are UAV and not covered (rare).
      if (bRequiresUAVBarrier)
      {
        EmitUAVBarrier();
      }
      return;
    }

    // Combine read-only generic read states if both are read-only.
    xiiBitflags<xiiGALResourceStateFlags> newState = m_Description.m_NewState;
    if ((oldState & xiiGALResourceStateFlags::GenericRead) == oldState && (newState & xiiGALResourceStateFlags::GenericRead) == newState)
    {
      newState |= oldState;
    }

    D3D12_RESOURCE_BARRIER d3dBarrier = {};
    d3dBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    d3dBarrier.Flags                  = xiiD3D12TypeConversions::GetResourceBarrierFlags(m_Description.m_TransitionType);
    d3dBarrier.Transition.pResource   = GetD3D12Resource(m_Description.m_pResource);
    d3dBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    d3dBarrier.Transition.StateBefore = xiiD3D12TypeConversions::GetResourceState(oldState) & m_ResourceStateMask;
    d3dBarrier.Transition.StateAfter  = xiiD3D12TypeConversions::GetResourceState(newState) & m_ResourceStateMask;

    // If states are equal after mapping, skip.
    if (d3dBarrier.Transition.StateBefore == d3dBarrier.Transition.StateAfter)
    {
      if (m_Description.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState))
      {
        if (m_Description.m_TransitionType == xiiGALStateTransitionType::Begin)
        {
          xiiLog::Error("UpdateState cannot be used with Begin split barrier for resource '{}'.", m_Description.m_pResource->GetDebugName());
        }
        else
        {
          m_Description.m_pResource->SetResourceState(newState);
        }
      }

      if (bRequiresUAVBarrier)
      {
        EmitUAVBarrier();
      }
      return;
    }

    // If texture: expand subresource ranges if needed.
    if (auto pTextureD3D12 = xiiDynamicCast<xiiGALTextureD3D12*>(m_Description.m_pResource))
    {
      const xiiGALTextureCreationDescription& description  = pTextureD3D12->GetDescription();
      const xiiUInt32                         uiMipCount   = ResolveMipCount(pTextureD3D12, m_Description.m_uiMipLevelCount);
      const xiiUInt32                         uiSliceCount = ResolveArrayCount(pTextureD3D12, m_Description.m_uiArraySliceCount);

      if (IsWholeResource(pTextureD3D12, m_Description.m_uiFirstMipLevel, m_Description.m_uiMipLevelCount, m_Description.m_uiFirstArraySlice, m_Description.m_uiArraySliceCount))
      {
        // Possibly discard before transition.
        DiscardIfAppropriate(description, d3dBarrier.Transition.StateBefore, /*uiEndMip*/ XII_GAL_REMAINING_MIP_LEVELS, /*uiEndSlice*/ XII_GAL_REMAINING_ARRAY_SLICES);

        // Emit single ALL_SUBRESOURCES barrier.
        m_pCommandListD3D12->GetD3D12CommandList()->ResourceBarrier(1, &d3dBarrier);

        // Possibly discard after transition.
        DiscardIfAppropriate(description, d3dBarrier.Transition.StateAfter, /*uiEndMip*/ XII_GAL_REMAINING_MIP_LEVELS, /*uiEndSlice*/ XII_GAL_REMAINING_ARRAY_SLICES);
      }
      else
      {
        // Partial range: expand per-subresource.
        const xiiUInt32 uiEndMip   = (m_Description.m_uiMipLevelCount == XII_GAL_REMAINING_MIP_LEVELS) ? description.m_uiMipLevels : (m_Description.m_uiFirstMipLevel + uiMipCount);
        const xiiUInt32 uiEndSlice = (m_Description.m_uiArraySliceCount == XII_GAL_REMAINING_ARRAY_SLICES) ? description.m_uiArraySizeOrDepth : (m_Description.m_uiFirstArraySlice + uiSliceCount);

        DiscardIfAppropriate(description, d3dBarrier.Transition.StateBefore, uiEndMip, uiEndSlice);

        for (xiiUInt32 uiMip = m_Description.m_uiFirstMipLevel; uiMip < uiEndMip; ++uiMip)
        {
          for (xiiUInt32 uiSlice = m_Description.m_uiFirstArraySlice; uiSlice < uiEndSlice; ++uiSlice)
          {
            d3dBarrier.Transition.Subresource = xiiD3D12TypeConversions::CalculateSubResourceIndex(uiMip, uiSlice, 0, description.m_uiMipLevels, description.m_uiArraySizeOrDepth);

            m_pCommandListD3D12->GetD3D12CommandList()->ResourceBarrier(1U, &d3dBarrier);
          }
        }

        DiscardIfAppropriate(description, d3dBarrier.Transition.StateAfter, uiEndMip, uiEndSlice);
      }
    }
    else if (auto pBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(m_Description.m_pResource))
    {
      // Buffers: single subresource
      m_pCommandListD3D12->GetD3D12CommandList()->ResourceBarrier(1U, &d3dBarrier);
    }
    else if (xiiDynamicCast<xiiGALTopLevelASD3D12*>(m_Description.m_pResource) || xiiDynamicCast<xiiGALBottomLevelASD3D12*>(m_Description.m_pResource))
    {
      // Acceleration structures: treat as requiring UAV barrier if write involved.
      if (m_Description.m_OldState == xiiGALResourceStateFlags::BuildASWrite || m_Description.m_NewState == xiiGALResourceStateFlags::BuildASWrite)
      {
        // Emit UAV barrier instead of transition
        D3D12_RESOURCE_BARRIER uavBarrier = {};
        uavBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        uavBarrier.UAV.pResource          = GetD3D12Resource(m_Description.m_pResource);

        m_pCommandListD3D12->GetD3D12CommandList()->ResourceBarrier(1U, &uavBarrier);
      }
      else
      {
        // AS read-only -> no transition needed, but keep compatibility.
        m_pCommandListD3D12->GetD3D12CommandList()->ResourceBarrier(1U, &d3dBarrier);
      }
    }
    else
    {
      xiiLog::Error("Transition: unsupported resource type for '{}'.", m_Description.m_pResource->GetDebugName());
      return;
    }

    // Update tracked state if requested (and permitted).
    if (m_Description.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::UpdateState))
    {
      if (m_Description.m_TransitionType == xiiGALStateTransitionType::Begin)
      {
        xiiLog::Error("UpdateState cannot be used with Begin split barrier for resource '{}'.", m_Description.m_pResource->GetDebugName());
      }
      else
      {
        m_Description.m_pResource->SetResourceState(newState);
      }
    }

    if (bRequiresUAVBarrier)
    {
      EmitUAVBarrier();
    }
  }

private:
  void EmitUAVBarrier()
  {
    // UAV barrier must be immediate (not split).
    if (m_Description.m_TransitionType != xiiGALStateTransitionType::Immediate)
    {
      xiiLog::Error("UAV barriers must be Immediate for resource '{}'.", m_Description.m_pResource->GetDebugName());
      return;
    }

    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    uavBarrier.UAV.pResource          = GetD3D12Resource(m_Description.m_pResource); // nullptr is permitted for global UAV barrier.

    m_pCommandListD3D12->GetD3D12CommandList()->ResourceBarrier(1U, &uavBarrier);
  }

  void DiscardIfAppropriate(const xiiGALTextureCreationDescription& textureDescription, D3D12_RESOURCE_STATES d3dState, xiiUInt32 uiEndMip = XII_GAL_REMAINING_MIP_LEVELS, xiiUInt32 uiEndSlice = XII_GAL_REMAINING_ARRAY_SLICES)
  {
    if (!m_Description.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::DiscardContent))
      return;

    const xiiGALCommandListCreationDescription& commandListDescription = m_pCommandListD3D12->GetDescription();
    bool                                        bIsPermitted           = false;

    if (commandListDescription.m_QueueFlags == xiiGALCommandQueueFlags::Graphics)
    {
      if ((d3dState & D3D12_RESOURCE_STATE_RENDER_TARGET) != 0)
      {
        XII_ASSERT_DEV(textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget), "");

        bIsPermitted = true;
      }
      if ((d3dState & D3D12_RESOURCE_STATE_DEPTH_WRITE) != 0)
      {
        XII_ASSERT_DEV(textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil), "");

        bIsPermitted = true;
      }
    }
    else if (commandListDescription.m_QueueFlags == xiiGALCommandQueueFlags::Compute)
    {
      if ((d3dState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0)
      {
        XII_ASSERT_DEV(textureDescription.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess), "");

        bIsPermitted = true;
      }
    }

    if (!bIsPermitted)
      return;

    m_pCommandListD3D12->FlushBarriers();

    if (m_Description.m_uiFirstMipLevel == 0 && uiEndMip == XII_GAL_REMAINING_MIP_LEVELS && m_Description.m_uiFirstArraySlice == 0 && uiEndSlice == XII_GAL_REMAINING_ARRAY_SLICES)
    {
      m_pCommandListD3D12->GetD3D12CommandList()->DiscardResource(GetD3D12Resource(m_Description.m_pResource), nullptr);
    }
    else
    {
      D3D12_DISCARD_REGION region = {};
      region.NumSubresources      = uiEndMip - m_Description.m_uiFirstMipLevel;

      for (xiiUInt32 uiSlice = m_Description.m_uiFirstArraySlice; uiSlice < uiEndSlice; ++uiSlice)
      {
        region.FirstSubresource = xiiD3D12TypeConversions::CalculateSubResourceIndex(m_Description.m_uiFirstMipLevel, uiSlice, 0, textureDescription.m_uiMipLevels, textureDescription.m_uiArraySizeOrDepth);

        m_pCommandListD3D12->GetD3D12CommandList()->DiscardResource(GetD3D12Resource(m_Description.m_pResource), &region);
      }
    }
  }

private:
  const xiiGALStateTransitionDescription& m_Description;
  xiiGALCommandListD3D12*                 m_pCommandListD3D12;
  const D3D12_RESOURCE_STATES             m_ResourceStateMask;
};

///////////////////////////////////////////////////////////////////////////

void xiiGALCommandListD3D12::FlushBarriers()
{
  if (!m_PendingResourceBarriers.IsEmpty())
  {
    m_pD3D12CommandList->ResourceBarrier(m_PendingResourceBarriers.GetCount(), m_PendingResourceBarriers.GetData());

    m_PendingResourceBarriers.Clear();
  }
}

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

  m_CommandListFlags                           = {};
  m_CommandListState                           = {};
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
    m_CommandListFlags      = {};
    m_CommandListState      = {};
    m_CommandListData       = {};
    return;
  }

  if (FAILED(m_pD3D12CommandAllocator->Reset()))
  {
    xiiLog::Error("Failed to reset D3D12 command allocator for command list '{}'.", GetDebugName());
    m_CommandListAllocation  = {};
    m_CommandListFlags       = {};
    m_CommandListState       = {};
    m_CommandListData        = {};
    m_pD3D12CommandAllocator = nullptr;
    m_pD3D12CommandList      = nullptr;
    return;
  }

  if (FAILED(m_pD3D12CommandList->Reset(m_pD3D12CommandAllocator, nullptr)))
  {
    xiiLog::Error("Failed to reset D3D12 command list '{}'.", GetDebugName());
    m_CommandListAllocation  = {};
    m_CommandListFlags       = {};
    m_CommandListState       = {};
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
  m_RecordingState        = RecordingState::Recording;
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
      m_CommandListFlags      = {};
      m_CommandListState      = {};
      m_CommandListData       = {};
      m_CommandListAllocation = {};
    }
  }

  m_CommandListAllocation  = {};
  m_pD3D12CommandAllocator = nullptr;
  m_pD3D12CommandList      = nullptr;
  m_CommandListFlags       = {};
  m_CommandListState       = {};
  m_CommandListData        = {};
  m_uiSubmittedFenceValue  = 0ULL;
  m_RecordingState         = RecordingState::Reset;
}

void xiiGALCommandListD3D12::SubmitPlatform(xiiGALCommandList* pSecondaryCommandList)
{
  if (pSecondaryCommandList == nullptr || m_pD3D12CommandList == nullptr)
    return;

  xiiGALCommandListD3D12* pSecondaryCommandListD3D12 = xiiDynamicCast<xiiGALCommandListD3D12*>(pSecondaryCommandList);
  if (pSecondaryCommandListD3D12 == nullptr || pSecondaryCommandListD3D12->GetD3D12CommandList() == nullptr)
    return;

  if (!pSecondaryCommandListD3D12->GetDescription().m_Flags.IsSet(xiiGALCommandListFlags::Secondary))
  {
    xiiLog::Warning("Ignoring D3D12 secondary command list submission for '{}': command list was not created with the Secondary flag.", pSecondaryCommandListD3D12->GetDebugName());
    return;
  }

  m_pD3D12CommandList->ExecuteBundle(pSecondaryCommandListD3D12->GetD3D12CommandList());
}

void xiiGALCommandListD3D12::SetPipelineStatePlatform(xiiGALPipelineState* pPipelineState)
{
  XII_IGNORE_UNUSED(pPipelineState);

  m_CommandListData.m_bPipelineStateModified = true;
}

void xiiGALCommandListD3D12::PushConstantsPlatform(xiiUInt32 uiOffset, xiiArrayPtr<const xiiUInt8> pData)
{
  if (m_pD3D12CommandList == nullptr || m_pPipelineState == nullptr || m_pPipelineResourceSignature == nullptr || pData.IsEmpty())
    return;

  if ((uiOffset % sizeof(xiiUInt32)) != 0U || (pData.GetCount() % sizeof(xiiUInt32)) != 0U)
  {
    xiiLog::Error("Failed to set push constants on D3D12 command list '{}': offset ({}) and size ({}) must be multiples of 4 bytes.", GetDebugName(), uiOffset, pData.GetCount());
    return;
  }

  const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

  xiiUInt32 uiRootDescriptorParameterCount = 0U;
  for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
  {
    D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    if (xiiD3D12TypeConversions::TryGetDescriptorRangeType(resource.m_ResourceType, rangeType))
    {
      ++uiRootDescriptorParameterCount;
    }
  }

  const xiiGALPipelineStateCreationDescription& pipelineDescription = m_pPipelineState->GetDescription();

  xiiUInt32 uiPushConstantRootParameterIndex = uiRootDescriptorParameterCount;
  for (const xiiGALPushConstantRange& range : signatureDescription.m_PushConstantRanges)
  {
    if (range.m_uiSize == 0U)
      continue;

    const xiiUInt32 uiRangeBegin = range.m_uiOffset;
    const xiiUInt32 uiRangeEnd   = range.m_uiOffset + range.m_uiSize;
    const xiiUInt32 uiDataEnd    = uiOffset + pData.GetCount();

    if (uiOffset >= uiRangeBegin && uiDataEnd <= uiRangeEnd)
    {
      const xiiUInt32  uiDestinationDWORDOffset = (uiOffset - uiRangeBegin) / sizeof(xiiUInt32);
      const xiiUInt32  uiSourceDWORDCount       = pData.GetCount() / sizeof(xiiUInt32);
      const xiiUInt32* pSourceConstants         = reinterpret_cast<const xiiUInt32*>(pData.GetPtr());

      if (pipelineDescription.IsAnyGraphicsPipeline())
      {
        m_pD3D12CommandList->SetGraphicsRoot32BitConstants(uiPushConstantRootParameterIndex, uiSourceDWORDCount, pSourceConstants, uiDestinationDWORDOffset);
      }
      else if (pipelineDescription.IsComputePipeline() || pipelineDescription.IsRayTracingPipeline())
      {
        m_pD3D12CommandList->SetComputeRoot32BitConstants(uiPushConstantRootParameterIndex, uiSourceDWORDCount, pSourceConstants, uiDestinationDWORDOffset);
      }
      else
      {
        xiiLog::Error("Push constants are unsupported for pipeline type '{}' on command list '{}'.", xiiArgEnum(pipelineDescription.m_PipelineType), GetDebugName());
      }

      return;
    }

    ++uiPushConstantRootParameterIndex;
  }

  xiiLog::Error("Failed to set push constants on D3D12 command list '{}': no matching push-constant range for offset {} and size {}.", GetDebugName(), uiOffset, pData.GetCount());
}

void xiiGALCommandListD3D12::SetStencilRefPlatform(xiiUInt32 uiStencilRef)
{
  XII_ASSERT_DEBUG(m_pD3D12CommandList != nullptr, "Invalid D3D12 command list.");

  m_pD3D12CommandList->OMSetStencilRef(uiStencilRef);
}

void xiiGALCommandListD3D12::SetBlendFactorPlatform(const xiiColor& blendFactor)
{
  XII_ASSERT_DEBUG(m_pD3D12CommandList != nullptr, "Invalid D3D12 command list.");

  m_pD3D12CommandList->OMSetBlendFactor(blendFactor.GetData());
}

void xiiGALCommandListD3D12::SetViewportsPlatform(xiiArrayPtr<xiiGALViewport> pViewports)
{
  XII_ASSERT_DEBUG(m_Viewports.GetCount() == pViewports.GetCount(), "Unexpected number of viewports.");
  XII_ASSERT_DEBUG(m_pD3D12CommandList != nullptr, "Invalid D3D12 command list.");

  xiiTemporaryHybridArray<D3D12_VIEWPORT, 2U> d3d12Viewports;
  d3d12Viewports.SetCountUninitialized(pViewports.GetCount());

  for (xiiUInt32 i = 0U; i < pViewports.GetCount(); ++i)
  {
    const xiiGALViewport& viewport = pViewports[i];

    D3D12_VIEWPORT& d3d12Viewport = d3d12Viewports[static_cast<size_t>(i)];
    d3d12Viewport.TopLeftX        = viewport.m_fTopLeftX;
    d3d12Viewport.TopLeftY        = viewport.m_fTopLeftY;
    d3d12Viewport.Width           = viewport.m_fWidth;
    d3d12Viewport.Height          = viewport.m_fHeight;
    d3d12Viewport.MinDepth        = viewport.m_fMinDepth;
    d3d12Viewport.MaxDepth        = viewport.m_fMaxDepth;
  }

  m_pD3D12CommandList->RSSetViewports(d3d12Viewports.GetCount(), d3d12Viewports.GetData());
}

void xiiGALCommandListD3D12::SetScissorRectsPlatform(xiiArrayPtr<xiiRectU32> pRects)
{
  XII_ASSERT_DEBUG(m_ScissorRects.GetCount() == pRects.GetCount(), "Unexpected number of scissor rects.");
  XII_ASSERT_DEBUG(m_pD3D12CommandList != nullptr, "Invalid D3D12 command list.");

  xiiTemporaryHybridArray<D3D12_RECT, 2U> d3d12ScissorRects;
  d3d12ScissorRects.SetCountUninitialized(pRects.GetCount());

  for (xiiUInt32 uiScissorRectIndex = 0U; uiScissorRectIndex < pRects.GetCount(); ++uiScissorRectIndex)
  {
    const xiiRectU32& rect      = pRects[uiScissorRectIndex];
    D3D12_RECT&       d3d12Rect = d3d12ScissorRects[static_cast<size_t>(uiScissorRectIndex)];
    d3d12Rect.left              = static_cast<LONG>(rect.x);
    d3d12Rect.top               = static_cast<LONG>(rect.y);
    d3d12Rect.right             = static_cast<LONG>(rect.x + rect.width);
    d3d12Rect.bottom            = static_cast<LONG>(rect.y + rect.height);
  }

  m_pD3D12CommandList->RSSetScissorRects(d3d12ScissorRects.GetCount(), d3d12ScissorRects.GetData());
}

void xiiGALCommandListD3D12::SetIndexBufferPlatform(xiiGALBuffer* pIndexBuffer, xiiUInt64 uiByteOffset, xiiEnum<xiiGALStateTransitionMode> transitionMode)
{
  XII_IGNORE_UNUSED(uiByteOffset);

  if (pIndexBuffer == nullptr || m_pD3D12CommandList == nullptr)
    return;

  xiiGALBufferD3D12* pBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(pIndexBuffer);
  if (pBufferD3D12 == nullptr || pBufferD3D12->GetD3D12Buffer() == nullptr)
  {
    xiiLog::Error("Failed to set index buffer on D3D12 command list '{}': incompatible backend buffer type.", GetDebugName());
    return;
  }

  if (!TransitionOrVerifyResourceStateForRayTracing(
        m_pD3D12CommandList,
        pBufferD3D12,
        pBufferD3D12->GetD3D12Buffer(),
        transitionMode,
        xiiGALResourceStateFlags::IndexBuffer,
        xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndexBuffer),
        "index buffer",
        GetDebugName()))
  {
    return;
  }
}

void xiiGALCommandListD3D12::SetVertexBuffersPlatform(xiiUInt32 uiStartSlot, xiiArrayPtr<VertexStreamDescription> pVertexStreams, xiiBitflags<xiiGALSetVertexBufferFlags> flags, xiiEnum<xiiGALStateTransitionMode> transitionMode)
{
  XII_IGNORE_UNUSED(uiStartSlot);
  XII_IGNORE_UNUSED(flags);

  if (m_pD3D12CommandList == nullptr)
    return;

  for (xiiUInt32 uiSlot = 0U; uiSlot < pVertexStreams.GetCount(); ++uiSlot)
  {
    VertexStreamDescription& vertexStream = pVertexStreams[uiSlot];

    if (xiiGALBufferD3D12* pBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(vertexStream.m_pBuffer))
    {
      if (pBufferD3D12->GetD3D12Buffer() == nullptr)
      {
        xiiLog::Error("Failed to set vertex buffers on D3D12 command list '{}': vertex stream {} has no native buffer.", GetDebugName(), uiSlot);
        return;
      }

      if (!TransitionOrVerifyResourceStateForRayTracing(
            m_pD3D12CommandList,
            pBufferD3D12,
            pBufferD3D12->GetD3D12Buffer(),
            transitionMode,
            xiiGALResourceStateFlags::VertexBuffer,
            xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::VertexBuffer),
            "vertex buffer",
            GetDebugName()))
      {
        return;
      }
    }
  }
}

void xiiGALCommandListD3D12::SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer)
{
  xiiGALBufferD3D12* pConstantBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(pConstantBuffer);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1U);

  xiiGALCommandListDataD3D12::ResourceSetBindings& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundConstantBuffers.EnsureCount(bindingInformation.m_uiBindSlot + 1U);
  bindSetResources.m_pBoundConstantBuffers[bindingInformation.m_uiBindSlot] = pConstantBufferD3D12 != nullptr ? pConstantBufferD3D12 : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListD3D12::SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewD3D12* pBufferViewD3D12 = xiiDynamicCast<xiiGALBufferViewD3D12*>(pBufferView);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1U);

  xiiGALCommandListDataD3D12::ResourceSetBindings& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundBufferResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1U);
  bindSetResources.m_pBoundBufferResourceViews[bindingInformation.m_uiBindSlot] = pBufferViewD3D12 != nullptr ? pBufferViewD3D12 : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListD3D12::SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewD3D12* pTextureViewD3D12 = xiiDynamicCast<xiiGALTextureViewD3D12*>(pTextureView);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1U);

  xiiGALCommandListDataD3D12::ResourceSetBindings& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundTextureResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1U);
  bindSetResources.m_pBoundTextureResourceViews[bindingInformation.m_uiBindSlot] = pTextureViewD3D12 != nullptr ? pTextureViewD3D12 : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListD3D12::SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView)
{
  xiiGALBufferViewD3D12* pBufferViewD3D12 = xiiDynamicCast<xiiGALBufferViewD3D12*>(pBufferView);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1U);

  xiiGALCommandListDataD3D12::ResourceSetBindings& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundUnorderedAccessBufferResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1U);
  bindSetResources.m_pBoundUnorderedAccessBufferResourceViews[bindingInformation.m_uiBindSlot] = pBufferViewD3D12 != nullptr ? pBufferViewD3D12 : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListD3D12::SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView)
{
  xiiGALTextureViewD3D12* pTextureViewD3D12 = xiiDynamicCast<xiiGALTextureViewD3D12*>(pTextureView);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1U);

  xiiGALCommandListDataD3D12::ResourceSetBindings& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundUnorderedAccessTextureResourceViews.EnsureCount(bindingInformation.m_uiBindSlot + 1U);
  bindSetResources.m_pBoundUnorderedAccessTextureResourceViews[bindingInformation.m_uiBindSlot] = pTextureViewD3D12 != nullptr ? pTextureViewD3D12 : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListD3D12::SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler)
{
  xiiGALSamplerD3D12* pSamplerD3D12 = xiiDynamicCast<xiiGALSamplerD3D12*>(pSampler);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1U);

  xiiGALCommandListDataD3D12::ResourceSetBindings& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundSamplerStates.EnsureCount(bindingInformation.m_uiBindSlot + 1U);
  bindSetResources.m_pBoundSamplerStates[bindingInformation.m_uiBindSlot] = pSamplerD3D12 != nullptr ? pSamplerD3D12 : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

void xiiGALCommandListD3D12::SetAccelerationStructurePlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTopLevelAS* pTopLevelAS)
{
  xiiGALTopLevelASD3D12* pTopLevelASD3D12 = xiiDynamicCast<xiiGALTopLevelASD3D12*>(pTopLevelAS);

  m_CommandListData.m_ResourceSets.EnsureCount(bindingInformation.m_uiBindSet + 1U);

  xiiGALCommandListDataD3D12::ResourceSetBindings& bindSetResources = m_CommandListData.m_ResourceSets[bindingInformation.m_uiBindSet];

  bindSetResources.m_pBoundAccelerationStructures.EnsureCount(bindingInformation.m_uiBindSlot + 1U);
  bindSetResources.m_pBoundAccelerationStructures[bindingInformation.m_uiBindSlot] = pTopLevelASD3D12 != nullptr ? pTopLevelASD3D12 : nullptr;

  m_CommandListData.m_bDescriptorsModified = true;
}

xiiResult xiiGALCommandListD3D12::CommitShaderResourcesPlatform(xiiEnum<xiiGALStateTransitionMode> mode)
{
  if (m_pD3D12CommandList == nullptr)
    return XII_FAILURE;

  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  if (pDeviceD3D12 == nullptr || pDeviceD3D12->GetD3D12Device() == nullptr)
  {
    xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': D3D12 device is unavailable.", GetDebugName());
    return XII_FAILURE;
  }

  D3D12PipelineRootBindingType pipelineRootBindingType = D3D12PipelineRootBindingType::None;

  if (m_CommandListData.m_bPipelineStateModified)
  {
    if (m_pPipelineState != nullptr)
    {
      const xiiGALPipelineStateCreationDescription& pipelineDescription = m_pPipelineState->GetDescription();

      if (pipelineDescription.IsAnyGraphicsPipeline())
      {
        xiiGALGraphicsPipelineStateD3D12* pGraphicsPipelineStateD3D12 = xiiDynamicCast<xiiGALGraphicsPipelineStateD3D12*>(m_pPipelineState);
        if (pGraphicsPipelineStateD3D12 == nullptr || pGraphicsPipelineStateD3D12->GetD3D12PipelineState() == nullptr || pGraphicsPipelineStateD3D12->GetD3D12RootSignature() == nullptr)
        {
          xiiLog::Error("Failed to bind graphics pipeline on D3D12 command list '{}': pipeline state or root signature is unavailable.", GetDebugName());
          return XII_FAILURE;
        }

        m_pD3D12CommandList->SetPipelineState(pGraphicsPipelineStateD3D12->GetD3D12PipelineState());
        m_pD3D12CommandList->SetGraphicsRootSignature(pGraphicsPipelineStateD3D12->GetD3D12RootSignature());
        m_pD3D12CommandList->IASetPrimitiveTopology(pGraphicsPipelineStateD3D12->GetD3D12PrimitiveTopology());

        pipelineRootBindingType = D3D12PipelineRootBindingType::Graphics;
      }
      else if (pipelineDescription.IsComputePipeline())
      {
        xiiGALComputePipelineStateD3D12* pComputePipelineStateD3D12 = xiiDynamicCast<xiiGALComputePipelineStateD3D12*>(m_pPipelineState);
        if (pComputePipelineStateD3D12 == nullptr || pComputePipelineStateD3D12->GetD3D12PipelineState() == nullptr || pComputePipelineStateD3D12->GetD3D12RootSignature() == nullptr)
        {
          xiiLog::Error("Failed to bind compute pipeline on D3D12 command list '{}': pipeline state or root signature is unavailable.", GetDebugName());
          return XII_FAILURE;
        }

        m_pD3D12CommandList->SetPipelineState(pComputePipelineStateD3D12->GetD3D12PipelineState());
        m_pD3D12CommandList->SetComputeRootSignature(pComputePipelineStateD3D12->GetD3D12RootSignature());

        pipelineRootBindingType = D3D12PipelineRootBindingType::Compute;
      }
      else if (pipelineDescription.IsRayTracingPipeline())
      {
        xiiGALRayTracingPipelineStateD3D12* pRayTracingPipelineStateD3D12 = xiiDynamicCast<xiiGALRayTracingPipelineStateD3D12*>(m_pPipelineState);
        if (pRayTracingPipelineStateD3D12 == nullptr || pRayTracingPipelineStateD3D12->GetD3D12StateObject() == nullptr || pRayTracingPipelineStateD3D12->GetD3D12RootSignature() == nullptr)
        {
          xiiLog::Error("Failed to bind ray tracing pipeline on D3D12 command list '{}': state object or root signature is unavailable.", GetDebugName());
          return XII_FAILURE;
        }

        ID3D12GraphicsCommandList4* pD3D12CommandList4 = nullptr;
        const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList4**>(&pD3D12CommandList4)));
        if (FAILED(hResult) || pD3D12CommandList4 == nullptr)
        {
          xiiLog::Error("Failed to bind ray tracing pipeline on D3D12 command list '{}': ID3D12GraphicsCommandList4 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
          return XII_FAILURE;
        }

        XII_SCOPE_EXIT(
          {
            XII_GAL_D3D12_RELEASE(pD3D12CommandList4);
          });

        pD3D12CommandList4->SetPipelineState1(pRayTracingPipelineStateD3D12->GetD3D12StateObject());
        m_pD3D12CommandList->SetComputeRootSignature(pRayTracingPipelineStateD3D12->GetD3D12RootSignature());

        pipelineRootBindingType = D3D12PipelineRootBindingType::Compute;
      }
      else
      {
        xiiLog::Error("Failed to bind D3D12 pipeline on command list '{}': pipeline type '{}' is unsupported.", GetDebugName(), xiiArgEnum(pipelineDescription.m_PipelineType));
        return XII_FAILURE;
      }
    }

    m_CommandListData.m_bPipelineStateModified = false;
    m_CommandListData.m_bDescriptorsModified   = true; // Changes to root signature layout require descriptor-table rebinding.
  }

  if (m_CommandListData.m_bDescriptorsModified)
  {
    if (m_pPipelineState != nullptr)
    {
      const xiiGALPipelineStateCreationDescription& pipelineDescription = m_pPipelineState->GetDescription();
      if (pipelineRootBindingType == D3D12PipelineRootBindingType::None)
      {
        if (pipelineDescription.IsAnyGraphicsPipeline())
          pipelineRootBindingType = D3D12PipelineRootBindingType::Graphics;
        else if (pipelineDescription.IsComputePipeline() || pipelineDescription.IsRayTracingPipeline())
          pipelineRootBindingType = D3D12PipelineRootBindingType::Compute;
      }
    }

    if (m_pPipelineResourceSignature != nullptr)
    {
      if (pipelineRootBindingType == D3D12PipelineRootBindingType::None)
      {
        xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no compatible pipeline state is bound.", GetDebugName());
        return XII_FAILURE;
      }

      xiiGALDescriptorSetPoolD3D12* pDescriptorPoolD3D12 = m_CommandListData.m_pDescriptorSetPoolD3D12.Borrow();
      if (pDescriptorPoolD3D12 == nullptr)
      {
        xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': descriptor pool is unavailable.", GetDebugName());
        return XII_FAILURE;
      }

      const xiiGALPipelineResourceSignatureCreationDescription& signatureDescription = m_pPipelineResourceSignature->GetDescription();

      xiiUInt32 uiCBVSRVUAVDescriptorCount = 0U;
      xiiUInt32 uiSamplerDescriptorCount   = 0U;

      for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
      {
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        if (!xiiD3D12TypeConversions::TryGetDescriptorRangeType(resource.m_ResourceType, rangeType))
          continue;

        const xiiUInt32 uiDescriptorCount = xiiMath::Max(1U, resource.m_uiArraySize);
        if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
        {
          uiSamplerDescriptorCount += uiDescriptorCount;
        }
        else
        {
          uiCBVSRVUAVDescriptorCount += uiDescriptorCount;
        }
      }

      xiiGALDescriptorSetPoolD3D12::DescriptorAllocation cbvSrvUavAllocation = {};
      xiiGALDescriptorSetPoolD3D12::DescriptorAllocation samplerAllocation   = {};

      if (uiCBVSRVUAVDescriptorCount > 0U)
      {
        cbvSrvUavAllocation = pDescriptorPoolD3D12->RequestDescriptorAllocation(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, uiCBVSRVUAVDescriptorCount);
        if (!IsValidDescriptorAllocation(cbvSrvUavAllocation))
        {
          xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': shader-resource descriptor allocation failed.", GetDebugName());
          return XII_FAILURE;
        }
      }

      if (uiSamplerDescriptorCount > 0U)
      {
        samplerAllocation = pDescriptorPoolD3D12->RequestDescriptorAllocation(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, uiSamplerDescriptorCount);
        if (!IsValidDescriptorAllocation(samplerAllocation))
        {
          xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': sampler descriptor allocation failed.", GetDebugName());
          return XII_FAILURE;
        }
      }

      ID3D12DescriptorHeap* pDescriptorHeaps[2]   = {};
      xiiUInt32             uiDescriptorHeapCount = 0U;

      if (cbvSrvUavAllocation.m_pDescriptorHeap != nullptr)
      {
        pDescriptorHeaps[uiDescriptorHeapCount++] = cbvSrvUavAllocation.m_pDescriptorHeap;
      }
      if (samplerAllocation.m_pDescriptorHeap != nullptr)
      {
        pDescriptorHeaps[uiDescriptorHeapCount++] = samplerAllocation.m_pDescriptorHeap;
      }

      if (uiDescriptorHeapCount > 0U)
      {
        m_pD3D12CommandList->SetDescriptorHeaps(uiDescriptorHeapCount, pDescriptorHeaps);
      }

      xiiUInt32 uiRootParameterIndex      = 0U;
      xiiUInt32 uiCBVSRVUAVDescriptorBase = 0U;
      xiiUInt32 uiSamplerDescriptorBase   = 0U;

      for (const xiiGALPipelineResourceDescription& resource : signatureDescription.m_Resources)
      {
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        if (!xiiD3D12TypeConversions::TryGetDescriptorRangeType(resource.m_ResourceType, rangeType))
          continue;

        const xiiUInt32 uiDescriptorCount = xiiMath::Max(1U, resource.m_uiArraySize);

        const bool                                                bSamplerRange    = rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        const xiiGALDescriptorSetPoolD3D12::DescriptorAllocation& activeAllocation = bSamplerRange ? samplerAllocation : cbvSrvUavAllocation;
        xiiUInt32&                                                uiDescriptorBase = bSamplerRange ? uiSamplerDescriptorBase : uiCBVSRVUAVDescriptorBase;

        if (!IsValidDescriptorAllocation(activeAllocation))
        {
          xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': descriptor allocation is invalid while binding resource '{}'.", GetDebugName(), resource.m_sName.GetView());
          return XII_FAILURE;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE destinationCPUHandleBase = OffsetCPUDescriptorHandle(activeAllocation.m_CPUHandle, activeAllocation.m_uiDescriptorSize, uiDescriptorBase);
        const D3D12_GPU_DESCRIPTOR_HANDLE destinationGPUHandleBase = OffsetGPUDescriptorHandle(activeAllocation.m_GPUHandle, activeAllocation.m_uiDescriptorSize, uiDescriptorBase);

        if (resource.m_uiBindSet >= m_CommandListData.m_ResourceSets.GetCount())
        {
          xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no resources were bound for descriptor set {} (resource '{}').", GetDebugName(), resource.m_uiBindSet, resource.m_sName.GetView());
          return XII_FAILURE;
        }

        xiiGALCommandListDataD3D12::ResourceSetBindings& boundSetResources = m_CommandListData.m_ResourceSets[resource.m_uiBindSet];

        auto BindDescriptorTable = [&](xiiUInt32 uiRootIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
          if (pipelineRootBindingType == D3D12PipelineRootBindingType::Graphics)
          {
            m_pD3D12CommandList->SetGraphicsRootDescriptorTable(uiRootIndex, gpuHandle);
          }
          else
          {
            m_pD3D12CommandList->SetComputeRootDescriptorTable(uiRootIndex, gpuHandle);
          }
        };

        switch (resource.m_ResourceType)
        {
          case xiiGALShaderResourceType::ConstantBuffer:
          {
            xiiGALBufferD3D12* pConstantBufferD3D12 = resource.m_uiBindSlot < boundSetResources.m_pBoundConstantBuffers.GetCount() ? boundSetResources.m_pBoundConstantBuffers[resource.m_uiBindSlot] : nullptr;
            if (pConstantBufferD3D12 == nullptr || pConstantBufferD3D12->GetD3D12Buffer() == nullptr)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no constant buffer bound at '{}'.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            if (mode != xiiGALStateTransitionMode::None)
            {
              if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pConstantBufferD3D12, pConstantBufferD3D12->GetD3D12Buffer(), mode, xiiGALResourceStateFlags::ConstantBuffer, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::ConstantBuffer), resource.m_sName.GetView(), GetDebugName()))
                return XII_FAILURE;
            }

            const xiiUInt64 uiMaxConstantBufferRange    = static_cast<xiiUInt64>(D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT) * sizeof(float) * 4U;
            xiiUInt64       uiConstantBufferSizeInBytes = xiiMath::Min(pConstantBufferD3D12->GetSize(), uiMaxConstantBufferRange);
            uiConstantBufferSizeInBytes                 = xiiMemoryUtils::AlignSize(uiConstantBufferSizeInBytes, static_cast<xiiUInt64>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
            uiConstantBufferSizeInBytes                 = xiiMath::Min(uiConstantBufferSizeInBytes, uiMaxConstantBufferRange);

            if (uiConstantBufferSizeInBytes == 0U || (uiConstantBufferSizeInBytes % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) != 0U)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': invalid constant-buffer size {} for '{}'.", GetDebugName(), uiConstantBufferSizeInBytes, resource.m_sName.GetView());
              return XII_FAILURE;
            }

            D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferView = {};
            constantBufferView.BufferLocation                  = pConstantBufferD3D12->GetD3D12BufferGPUVirtualAddress();
            constantBufferView.SizeInBytes                     = static_cast<UINT>(uiConstantBufferSizeInBytes);

            if (constantBufferView.BufferLocation == 0ULL)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': constant-buffer GPU virtual address is invalid for '{}'.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            for (xiiUInt32 i = 0U; i < uiDescriptorCount; ++i)
            {
              const D3D12_CPU_DESCRIPTOR_HANDLE destinationCPUHandle = OffsetCPUDescriptorHandle(destinationCPUHandleBase, activeAllocation.m_uiDescriptorSize, i);
              pDeviceD3D12->GetD3D12Device()->CreateConstantBufferView(&constantBufferView, destinationCPUHandle);
            }
          }
          break;

          case xiiGALShaderResourceType::BufferSRV:
          {
            xiiGALBufferViewD3D12* pBufferViewD3D12 = resource.m_uiBindSlot < boundSetResources.m_pBoundBufferResourceViews.GetCount() ? boundSetResources.m_pBoundBufferResourceViews[resource.m_uiBindSlot] : nullptr;
            if (pBufferViewD3D12 == nullptr || pBufferViewD3D12->GetCPUDescriptorHandle().ptr == 0U)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no shader-resource buffer view bound at '{}'.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            xiiSharedPtr<xiiGALBufferD3D12> pBufferD3D12 = pBufferViewD3D12->GetBuffer().Downcast<xiiGALBufferD3D12>();
            if (pBufferD3D12 == nullptr || pBufferD3D12->GetD3D12Buffer() == nullptr)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': shader-resource buffer view '{}' references an invalid backing buffer.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            if (mode != xiiGALStateTransitionMode::None)
            {
              if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pBufferD3D12.Borrow(), pBufferD3D12->GetD3D12Buffer(), mode, xiiGALResourceStateFlags::ShaderResource, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::ShaderResource), resource.m_sName.GetView(), GetDebugName()))
                return XII_FAILURE;
            }

            for (xiiUInt32 i = 0U; i < uiDescriptorCount; ++i)
            {
              const D3D12_CPU_DESCRIPTOR_HANDLE destinationCPUHandle = OffsetCPUDescriptorHandle(destinationCPUHandleBase, activeAllocation.m_uiDescriptorSize, i);
              pDeviceD3D12->GetD3D12Device()->CopyDescriptorsSimple(1U, destinationCPUHandle, pBufferViewD3D12->GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
          }
          break;

          case xiiGALShaderResourceType::TextureSRV:
          case xiiGALShaderResourceType::TextureAndSampler:
          case xiiGALShaderResourceType::InputAttachment:
          {
            xiiGALTextureViewD3D12* pTextureViewD3D12 = resource.m_uiBindSlot < boundSetResources.m_pBoundTextureResourceViews.GetCount() ? boundSetResources.m_pBoundTextureResourceViews[resource.m_uiBindSlot] : nullptr;
            if (pTextureViewD3D12 == nullptr || pTextureViewD3D12->GetCPUDescriptorHandle().ptr == 0U)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no shader-resource texture view bound at '{}'.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            xiiSharedPtr<xiiGALTextureD3D12> pTextureD3D12 = pTextureViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
            if (pTextureD3D12 == nullptr || pTextureD3D12->GetD3D12Texture() == nullptr)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': shader-resource texture view '{}' references an invalid backing texture.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            const xiiBitflags<xiiGALResourceStateFlags> expectedTextureState = pTextureD3D12->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil) ? xiiGALResourceStateFlags::DepthRead : xiiGALResourceStateFlags::ShaderResource;

            if (mode != xiiGALStateTransitionMode::None)
            {
              if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTextureD3D12.Borrow(), pTextureD3D12->GetD3D12Texture(), mode, expectedTextureState, xiiD3D12TypeConversions::GetResourceState(expectedTextureState), resource.m_sName.GetView(), GetDebugName()))
                return XII_FAILURE;
            }

            for (xiiUInt32 i = 0U; i < uiDescriptorCount; ++i)
            {
              const D3D12_CPU_DESCRIPTOR_HANDLE destinationCPUHandle = OffsetCPUDescriptorHandle(destinationCPUHandleBase, activeAllocation.m_uiDescriptorSize, i);
              pDeviceD3D12->GetD3D12Device()->CopyDescriptorsSimple(1U, destinationCPUHandle, pTextureViewD3D12->GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
          }
          break;

          case xiiGALShaderResourceType::BufferUAV:
          {
            xiiGALBufferViewD3D12* pBufferViewD3D12 = resource.m_uiBindSlot < boundSetResources.m_pBoundUnorderedAccessBufferResourceViews.GetCount() ? boundSetResources.m_pBoundUnorderedAccessBufferResourceViews[resource.m_uiBindSlot] : nullptr;
            if (pBufferViewD3D12 == nullptr || pBufferViewD3D12->GetCPUDescriptorHandle().ptr == 0U)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no unordered-access buffer view bound at '{}'.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            xiiSharedPtr<xiiGALBufferD3D12> pBufferD3D12 = pBufferViewD3D12->GetBuffer().Downcast<xiiGALBufferD3D12>();
            if (pBufferD3D12 == nullptr || pBufferD3D12->GetD3D12Buffer() == nullptr)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': unordered-access buffer view '{}' references an invalid backing buffer.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            if (mode != xiiGALStateTransitionMode::None)
            {
              if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pBufferD3D12.Borrow(), pBufferD3D12->GetD3D12Buffer(), mode, xiiGALResourceStateFlags::UnorderedAccess, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::UnorderedAccess), resource.m_sName.GetView(), GetDebugName()))
                return XII_FAILURE;
            }

            for (xiiUInt32 i = 0U; i < uiDescriptorCount; ++i)
            {
              const D3D12_CPU_DESCRIPTOR_HANDLE destinationCPUHandle = OffsetCPUDescriptorHandle(destinationCPUHandleBase, activeAllocation.m_uiDescriptorSize, i);
              pDeviceD3D12->GetD3D12Device()->CopyDescriptorsSimple(1U, destinationCPUHandle, pBufferViewD3D12->GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
          }
          break;

          case xiiGALShaderResourceType::TextureUAV:
          {
            xiiGALTextureViewD3D12* pTextureViewD3D12 = resource.m_uiBindSlot < boundSetResources.m_pBoundUnorderedAccessTextureResourceViews.GetCount() ? boundSetResources.m_pBoundUnorderedAccessTextureResourceViews[resource.m_uiBindSlot] : nullptr;
            if (pTextureViewD3D12 == nullptr || pTextureViewD3D12->GetCPUDescriptorHandle().ptr == 0U)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no unordered-access texture view bound at '{}'.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            xiiSharedPtr<xiiGALTextureD3D12> pTextureD3D12 = pTextureViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
            if (pTextureD3D12 == nullptr || pTextureD3D12->GetD3D12Texture() == nullptr)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': unordered-access texture view '{}' references an invalid backing texture.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            if (mode != xiiGALStateTransitionMode::None)
            {
              if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTextureD3D12.Borrow(), pTextureD3D12->GetD3D12Texture(), mode, xiiGALResourceStateFlags::UnorderedAccess, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::UnorderedAccess), resource.m_sName.GetView(), GetDebugName()))
                return XII_FAILURE;
            }

            for (xiiUInt32 i = 0U; i < uiDescriptorCount; ++i)
            {
              const D3D12_CPU_DESCRIPTOR_HANDLE destinationCPUHandle = OffsetCPUDescriptorHandle(destinationCPUHandleBase, activeAllocation.m_uiDescriptorSize, i);
              pDeviceD3D12->GetD3D12Device()->CopyDescriptorsSimple(1U, destinationCPUHandle, pTextureViewD3D12->GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
          }
          break;

          case xiiGALShaderResourceType::Sampler:
          {
            xiiGALSamplerD3D12* pSamplerD3D12 = resource.m_uiBindSlot < boundSetResources.m_pBoundSamplerStates.GetCount() ? boundSetResources.m_pBoundSamplerStates[resource.m_uiBindSlot] : nullptr;
            if (pSamplerD3D12 == nullptr || pSamplerD3D12->GetCPUDescriptorHandle().ptr == 0U)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no sampler bound at '{}'.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            for (xiiUInt32 i = 0U; i < uiDescriptorCount; ++i)
            {
              const D3D12_CPU_DESCRIPTOR_HANDLE destinationCPUHandle = OffsetCPUDescriptorHandle(destinationCPUHandleBase, activeAllocation.m_uiDescriptorSize, i);
              pDeviceD3D12->GetD3D12Device()->CopyDescriptorsSimple(1U, destinationCPUHandle, pSamplerD3D12->GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            }
          }
          break;

          case xiiGALShaderResourceType::AccelerationStructure:
          {
            xiiGALTopLevelASD3D12* pTopLevelASD3D12 = resource.m_uiBindSlot < boundSetResources.m_pBoundAccelerationStructures.GetCount() ? boundSetResources.m_pBoundAccelerationStructures[resource.m_uiBindSlot] : nullptr;
            if (pTopLevelASD3D12 == nullptr || pTopLevelASD3D12->GetD3D12Resource() == nullptr)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': no acceleration structure bound at '{}'.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            if (mode != xiiGALStateTransitionMode::None)
            {
              if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTopLevelASD3D12, pTopLevelASD3D12->GetD3D12Resource(), mode, xiiGALResourceStateFlags::RayTracing, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::RayTracing), resource.m_sName.GetView(), GetDebugName()))
                return XII_FAILURE;
            }

            const D3D12_GPU_VIRTUAL_ADDRESS d3d12TopLevelASAddress = pTopLevelASD3D12->GetD3D12GPUVirtualAddress();
            if (d3d12TopLevelASAddress == 0ULL)
            {
              xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': acceleration structure '{}' has invalid GPU virtual address.", GetDebugName(), resource.m_sName.GetView());
              return XII_FAILURE;
            }

            D3D12_SHADER_RESOURCE_VIEW_DESC d3d12AccelerationStructureView          = {};
            d3d12AccelerationStructureView.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            d3d12AccelerationStructureView.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
            d3d12AccelerationStructureView.RaytracingAccelerationStructure.Location = d3d12TopLevelASAddress;

            for (xiiUInt32 i = 0U; i < uiDescriptorCount; ++i)
            {
              const D3D12_CPU_DESCRIPTOR_HANDLE destinationCPUHandle = OffsetCPUDescriptorHandle(destinationCPUHandleBase, activeAllocation.m_uiDescriptorSize, i);
              pDeviceD3D12->GetD3D12Device()->CreateShaderResourceView(nullptr, &d3d12AccelerationStructureView, destinationCPUHandle);
            }
          }
          break;

          default:
            xiiLog::Error("Failed to commit shader resources on D3D12 command list '{}': unsupported shader resource type '{}' for '{}'.", GetDebugName(), xiiArgEnum(resource.m_ResourceType), resource.m_sName.GetView());
            return XII_FAILURE;
        }

        BindDescriptorTable(uiRootParameterIndex, destinationGPUHandleBase);

        uiDescriptorBase += uiDescriptorCount;
        ++uiRootParameterIndex;
      }
    }

    m_CommandListData.m_bDescriptorsModified = false;
  }

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::ClearRenderTargetViewPlatform(xiiGALTextureView* pRenderTargetView, const xiiColor& clearColor)
{
  if (m_pD3D12CommandList == nullptr || pRenderTargetView == nullptr)
    return;

  xiiGALTextureViewD3D12* pRenderTargetViewD3D12 = xiiDynamicCast<xiiGALTextureViewD3D12*>(pRenderTargetView);
  if (pRenderTargetViewD3D12 == nullptr || pRenderTargetViewD3D12->GetCPUDescriptorHandle().ptr == 0U)
  {
    xiiLog::Error("Failed to clear render target on D3D12 command list '{}': incompatible render-target view backend type or descriptor handle.", GetDebugName());
    return;
  }

  xiiSharedPtr<xiiGALTextureD3D12> pTextureD3D12 = pRenderTargetViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
  if (pTextureD3D12 == nullptr || pTextureD3D12->GetD3D12Texture() == nullptr)
  {
    xiiLog::Error("Failed to clear render target on D3D12 command list '{}': backing texture is invalid.", GetDebugName());
    return;
  }

  // if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTextureD3D12.Borrow(), pTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::RenderTarget, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::RenderTarget), "render-target clear", GetDebugName()))
  //   return;

  const float clearColorRGBA[4] = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
  m_pD3D12CommandList->ClearRenderTargetView(pRenderTargetViewD3D12->GetCPUDescriptorHandle(), clearColorRGBA, 0U, nullptr);
}

void xiiGALCommandListD3D12::ClearDepthStencilViewPlatform(xiiGALTextureView* pDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear)
{
  if (m_pD3D12CommandList == nullptr || pDepthStencilView == nullptr || (!bClearDepth && !bClearStencil))
    return;

  xiiGALTextureViewD3D12* pDepthStencilViewD3D12 = xiiDynamicCast<xiiGALTextureViewD3D12*>(pDepthStencilView);
  if (pDepthStencilViewD3D12 == nullptr || pDepthStencilViewD3D12->GetCPUDescriptorHandle().ptr == 0U)
  {
    xiiLog::Error("Failed to clear depth-stencil view on D3D12 command list '{}': incompatible depth-stencil view backend type or descriptor handle.", GetDebugName());
    return;
  }

  xiiSharedPtr<xiiGALTextureD3D12> pTextureD3D12 = pDepthStencilViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
  if (pTextureD3D12 == nullptr || pTextureD3D12->GetD3D12Texture() == nullptr)
  {
    xiiLog::Error("Failed to clear depth-stencil on D3D12 command list '{}': backing texture is invalid.", GetDebugName());
    return;
  }

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTextureD3D12.Borrow(), pTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::DepthWrite, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::DepthWrite), "depth-stencil clear", GetDebugName()))
    return;

  D3D12_CLEAR_FLAGS d3d12ClearFlags = static_cast<D3D12_CLEAR_FLAGS>(0U);
  if (bClearDepth)
    d3d12ClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
  if (bClearStencil)
    d3d12ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;

  m_pD3D12CommandList->ClearDepthStencilView(pDepthStencilViewD3D12->GetCPUDescriptorHandle(), d3d12ClearFlags, fDepthClear, uiStencilClear, 0U, nullptr);
}

void xiiGALCommandListD3D12::BeginRenderPassPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues)
{
  if (m_pD3D12CommandList == nullptr || pRenderPass == nullptr || pFramebuffer == nullptr)
    return;

  xiiGALRenderPassD3D12*  pRenderPassD3D12  = xiiDynamicCast<xiiGALRenderPassD3D12*>(pRenderPass);
  xiiGALFramebufferD3D12* pFramebufferD3D12 = xiiDynamicCast<xiiGALFramebufferD3D12*>(pFramebuffer);
  if (pRenderPassD3D12 == nullptr || pFramebufferD3D12 == nullptr)
  {
    xiiLog::Error("Failed to begin render pass on D3D12 command list '{}': incompatible render pass/framebuffer backend types.", GetDebugName());
    return;
  }

  const xiiGALRenderPassCreationDescription&  renderPassDescription  = pRenderPassD3D12->GetDescription();
  const xiiGALFramebufferCreationDescription& framebufferDescription = pFramebufferD3D12->GetDescription();
  if (renderPassDescription.m_SubPasses.IsEmpty())
  {
    xiiLog::Error("Failed to begin render pass on D3D12 command list '{}': render pass contains no subpasses.", GetDebugName());
    return;
  }

  m_CommandListState.m_uiFramebufferWidth       = framebufferDescription.m_FramebufferSize.width;
  m_CommandListState.m_uiFramebufferHeight      = framebufferDescription.m_FramebufferSize.height;
  m_CommandListState.m_uiFramebufferArraySlices = framebufferDescription.m_uiArraySliceCount;
  m_CommandListState.m_bIsShadingRateSet        = false;
  m_CommandListFlags.Remove(CommandListFlags::ShadingRateSet);

  m_CommandListData.m_uiSubpassIndex = 0U;
  m_CommandListData.m_AttachmentClearValues.Clear();
  m_CommandListData.m_AttachmentClearValues.SetCount(renderPassDescription.m_Attachments.GetCount());

  for (xiiUInt32 uiAttachmentIndex = 0U; uiAttachmentIndex < renderPassDescription.m_Attachments.GetCount(); ++uiAttachmentIndex)
  {
    D3D12_CLEAR_VALUE& clearValue = m_CommandListData.m_AttachmentClearValues[uiAttachmentIndex];
    clearValue                    = {};
    clearValue.Format             = xiiD3D12TypeConversions::GetFormat(renderPassDescription.m_Attachments[uiAttachmentIndex].m_Format);

    if (uiAttachmentIndex < pOptimizedClearValues.GetCount())
    {
      const xiiGALOptimizedClearValue& optimizedClear = pOptimizedClearValues[uiAttachmentIndex];
      clearValue.Color[0]                             = optimizedClear.m_ClearColour.r;
      clearValue.Color[1]                             = optimizedClear.m_ClearColour.g;
      clearValue.Color[2]                             = optimizedClear.m_ClearColour.b;
      clearValue.Color[3]                             = optimizedClear.m_ClearColour.a;
      clearValue.DepthStencil.Depth                   = optimizedClear.m_DepthStencil.m_fDepth;
      clearValue.DepthStencil.Stencil                 = optimizedClear.m_DepthStencil.m_uiStencil;
    }
    else
    {
      clearValue.DepthStencil.Depth   = 1.0f;
      clearValue.DepthStencil.Stencil = 0U;
    }
  }

  for (xiiUInt32 uiAttachmentIndex = 0U; uiAttachmentIndex < renderPassDescription.m_Attachments.GetCount() && uiAttachmentIndex < framebufferDescription.m_Attachments.GetCount(); ++uiAttachmentIndex)
  {
    const xiiGALRenderPassAttachmentDescription& attachmentDescription = renderPassDescription.m_Attachments[uiAttachmentIndex];
    if (attachmentDescription.m_InitialStateFlags == xiiGALResourceStateFlags::Unknown || attachmentDescription.m_InitialStateFlags == xiiGALResourceStateFlags::Undefined)
      continue;

    xiiSharedPtr<xiiGALTextureViewD3D12> pAttachmentViewD3D12 = framebufferDescription.m_Attachments[uiAttachmentIndex].Downcast<xiiGALTextureViewD3D12>();
    xiiSharedPtr<xiiGALTextureD3D12>     pAttachmentTextureD3D12;
    if (pAttachmentViewD3D12 != nullptr)
    {
      pAttachmentTextureD3D12 = pAttachmentViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
    }

    if (pAttachmentTextureD3D12 == nullptr || pAttachmentTextureD3D12->GetD3D12Texture() == nullptr)
    {
      xiiLog::Error("Failed to begin render pass on D3D12 command list '{}': attachment {} does not reference a valid D3D12 texture.", GetDebugName(), uiAttachmentIndex);
      return;
    }

    if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pAttachmentTextureD3D12.Borrow(), pAttachmentTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, attachmentDescription.m_InitialStateFlags, xiiD3D12TypeConversions::GetResourceState(attachmentDescription.m_InitialStateFlags), "render-pass attachment initial state", GetDebugName()))
      return;
  }

  BindSubpassAttachments(pRenderPassD3D12, pFramebufferD3D12, 0U, pOptimizedClearValues);
}

void xiiGALCommandListD3D12::NextSubpassPlatform()
{
  if (m_pD3D12CommandList == nullptr)
    return;

  xiiGALRenderPassD3D12*  pRenderPassD3D12  = xiiDynamicCast<xiiGALRenderPassD3D12*>(m_pRenderPass);
  xiiGALFramebufferD3D12* pFramebufferD3D12 = xiiDynamicCast<xiiGALFramebufferD3D12*>(m_pFramebuffer);
  if (pRenderPassD3D12 == nullptr || pFramebufferD3D12 == nullptr)
  {
    xiiLog::Error("Failed to advance subpass on D3D12 command list '{}': render pass/framebuffer are not active or incompatible.", GetDebugName());
    return;
  }

  const xiiGALRenderPassCreationDescription& renderPassDescription = pRenderPassD3D12->GetDescription();
  const xiiUInt32                            uiNextSubpassIndex    = m_CommandListData.m_uiSubpassIndex + 1U;
  if (uiNextSubpassIndex >= renderPassDescription.m_SubPasses.GetCount())
  {
    xiiLog::Error("Failed to advance subpass on D3D12 command list '{}': subpass index {} is out of range (subpass count {}).", GetDebugName(), uiNextSubpassIndex, renderPassDescription.m_SubPasses.GetCount());
    return;
  }

  m_CommandListData.m_uiSubpassIndex = uiNextSubpassIndex;
  BindSubpassAttachments(pRenderPassD3D12, pFramebufferD3D12, uiNextSubpassIndex, xiiArrayPtr<const xiiGALOptimizedClearValue>());
}

void xiiGALCommandListD3D12::EndRenderPassPlatform()
{
  if (m_pD3D12CommandList == nullptr)
    return;

  xiiGALRenderPassD3D12*  pRenderPassD3D12  = xiiDynamicCast<xiiGALRenderPassD3D12*>(m_pRenderPass);
  xiiGALFramebufferD3D12* pFramebufferD3D12 = xiiDynamicCast<xiiGALFramebufferD3D12*>(m_pFramebuffer);
  if (pRenderPassD3D12 != nullptr && pFramebufferD3D12 != nullptr)
  {
    const xiiGALRenderPassCreationDescription&  renderPassDescription  = pRenderPassD3D12->GetDescription();
    const xiiGALFramebufferCreationDescription& framebufferDescription = pFramebufferD3D12->GetDescription();

    for (xiiUInt32 uiAttachmentIndex = 0U; uiAttachmentIndex < renderPassDescription.m_Attachments.GetCount() && uiAttachmentIndex < framebufferDescription.m_Attachments.GetCount(); ++uiAttachmentIndex)
    {
      const xiiGALRenderPassAttachmentDescription& attachmentDescription = renderPassDescription.m_Attachments[uiAttachmentIndex];
      if (attachmentDescription.m_FinalStateFlags == xiiGALResourceStateFlags::Unknown || attachmentDescription.m_FinalStateFlags == xiiGALResourceStateFlags::Undefined)
        continue;

      xiiSharedPtr<xiiGALTextureViewD3D12> pAttachmentViewD3D12 = framebufferDescription.m_Attachments[uiAttachmentIndex].Downcast<xiiGALTextureViewD3D12>();
      xiiSharedPtr<xiiGALTextureD3D12>     pAttachmentTextureD3D12;
      if (pAttachmentViewD3D12 != nullptr)
      {
        pAttachmentTextureD3D12 = pAttachmentViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
      }

      if (pAttachmentTextureD3D12 == nullptr || pAttachmentTextureD3D12->GetD3D12Texture() == nullptr)
      {
        xiiLog::Error("Failed to end render pass on D3D12 command list '{}': attachment {} does not reference a valid D3D12 texture.", GetDebugName(), uiAttachmentIndex);
        continue;
      }

      if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pAttachmentTextureD3D12.Borrow(), pAttachmentTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, attachmentDescription.m_FinalStateFlags, xiiD3D12TypeConversions::GetResourceState(attachmentDescription.m_FinalStateFlags), "render-pass attachment final state", GetDebugName()))
      {
        xiiLog::Error("Failed to transition attachment {} to final state at render-pass end on command list '{}'.", uiAttachmentIndex, GetDebugName());
      }
    }
  }

  m_CommandListData.m_pBoundRenderTargets.Clear();
  m_CommandListData.m_pBoundDepthStencilTarget = nullptr;
  m_CommandListData.m_uiBoundRenderTargetCount = 0U;
  m_CommandListData.m_uiSubpassIndex           = 0U;
  m_CommandListData.m_AttachmentClearValues.Clear();

  m_CommandListState.m_uiFramebufferWidth       = 0U;
  m_CommandListState.m_uiFramebufferHeight      = 0U;
  m_CommandListState.m_uiFramebufferArraySlices = 0U;
  m_CommandListState.m_bIsShadingRateSet        = false;
}

void xiiGALCommandListD3D12::DrawPlatform(const xiiGALDrawDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  PrepareForDraw();

  if (description.m_uiVertexCount == 0U || description.m_uiInstanceCount == 0U)
    return;

  m_pD3D12CommandList->DrawInstanced(description.m_uiVertexCount, description.m_uiInstanceCount, description.m_uiStartVertexLocation, description.m_uiFirstInstanceLocation);
}

void xiiGALCommandListD3D12::DrawIndexedPlatform(const xiiGALDrawIndexedDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  PrepareForIndexedDraw(description.m_IndexType);

  if (description.m_uiIndexCount == 0U || description.m_uiInstanceCount == 0U)
    return;

  m_pD3D12CommandList->DrawIndexedInstanced(description.m_uiIndexCount, description.m_uiInstanceCount, description.m_uiFirstIndexLocation, static_cast<INT>(description.m_uiBaseVertex), description.m_uiFirstInstanceLocation);
}

void xiiGALCommandListD3D12::DrawIndirectPlatform(const xiiGALDrawIndirectDescription& description)
{
  if (m_pD3D12CommandList == nullptr || description.m_pBuffer == nullptr || description.m_uiDrawCount == 0U)
    return;

  PrepareForDraw();

  xiiGALBufferD3D12* pArgumentBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pBuffer);
  if (pArgumentBufferD3D12 == nullptr || pArgumentBufferD3D12->GetD3D12Buffer() == nullptr)
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pArgumentBufferD3D12, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_BufferStateTransition, xiiGALResourceStateFlags::IndirectArgument, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndirectArgument), "indirect draw argument buffer", GetDebugName()))
    return;

  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12      = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  ID3D12CommandSignature*         pCommandSignature = CreateIndirectCommandSignature(pDeviceD3D12->GetD3D12Device(), D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, description.m_uiDrawArgumentStride);
  if (pCommandSignature == nullptr)
  {
    xiiLog::Error("Failed to issue DrawIndirect on D3D12 command list '{}': command signature creation failed.", GetDebugName());
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pCommandSignature);
    });

  ID3D12Resource* pCountBuffer = nullptr;
  if (description.m_pCounterBuffer != nullptr)
  {
    xiiGALBufferD3D12* pCounterBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pCounterBuffer);
    if (pCounterBufferD3D12 == nullptr || pCounterBufferD3D12->GetD3D12Buffer() == nullptr)
      return;

    if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pCounterBufferD3D12, pCounterBufferD3D12->GetD3D12Buffer(), description.m_CounterBufferStateTransition, xiiGALResourceStateFlags::IndirectArgument, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndirectArgument), "indirect draw count buffer", GetDebugName()))
      return;

    pCountBuffer = pCounterBufferD3D12->GetD3D12Buffer();
  }

  m_pD3D12CommandList->ExecuteIndirect(pCommandSignature, description.m_uiDrawCount, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_uiDrawArgumentOffset, pCountBuffer, description.m_uiCounterOffset);
}

void xiiGALCommandListD3D12::DrawIndexedIndirectPlatform(const xiiGALDrawIndexedIndirectDescription& description)
{
  if (m_pD3D12CommandList == nullptr || description.m_pBuffer == nullptr || description.m_uiDrawCount == 0U)
    return;

  PrepareForIndexedDraw(description.m_IndexType);

  xiiGALBufferD3D12* pArgumentBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pBuffer);
  if (pArgumentBufferD3D12 == nullptr || pArgumentBufferD3D12->GetD3D12Buffer() == nullptr)
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pArgumentBufferD3D12, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_BufferStateTransition, xiiGALResourceStateFlags::IndirectArgument, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndirectArgument), "indexed indirect draw argument buffer", GetDebugName()))
    return;

  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12      = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  ID3D12CommandSignature*         pCommandSignature = CreateIndirectCommandSignature(pDeviceD3D12->GetD3D12Device(), D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, description.m_uiDrawArgumentStride);
  if (pCommandSignature == nullptr)
  {
    xiiLog::Error("Failed to issue DrawIndexedIndirect on D3D12 command list '{}': command signature creation failed.", GetDebugName());
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pCommandSignature);
    });

  ID3D12Resource* pCountBuffer = nullptr;
  if (description.m_pCounterBuffer != nullptr)
  {
    xiiGALBufferD3D12* pCounterBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pCounterBuffer);
    if (pCounterBufferD3D12 == nullptr || pCounterBufferD3D12->GetD3D12Buffer() == nullptr)
      return;

    if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pCounterBufferD3D12, pCounterBufferD3D12->GetD3D12Buffer(), description.m_CounterBufferStateTransition, xiiGALResourceStateFlags::IndirectArgument, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndirectArgument), "indexed indirect draw count buffer", GetDebugName()))
      return;

    pCountBuffer = pCounterBufferD3D12->GetD3D12Buffer();
  }

  m_pD3D12CommandList->ExecuteIndirect(pCommandSignature, description.m_uiDrawCount, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_uiDrawArgumentOffset, pCountBuffer, description.m_uiCounterOffset);
}

void xiiGALCommandListD3D12::DrawMeshPlatform(const xiiGALDrawMeshDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  PrepareForDraw();

  if (description.m_uiThreadGroupCountX == 0U || description.m_uiThreadGroupCountY == 0U || description.m_uiThreadGroupCountZ == 0U)
    return;

  ID3D12GraphicsCommandList6* pD3D12CommandList6 = nullptr;
  if (FAILED(m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList6), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList6**>(&pD3D12CommandList6)))) || pD3D12CommandList6 == nullptr)
  {
    xiiLog::Error("Failed to issue DrawMesh on D3D12 command list '{}': ID3D12GraphicsCommandList6 is unavailable.", GetDebugName());
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList6);
    });

  pD3D12CommandList6->DispatchMesh(description.m_uiThreadGroupCountX, description.m_uiThreadGroupCountY, description.m_uiThreadGroupCountZ);
}

void xiiGALCommandListD3D12::DrawMeshIndirectPlatform(const xiiGALDrawMeshIndirectDescription& description)
{
  if (m_pD3D12CommandList == nullptr || description.m_pBuffer == nullptr || description.m_uiCommandCount == 0U)
    return;

  PrepareForDraw();

  xiiGALBufferD3D12* pArgumentBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pBuffer);
  if (pArgumentBufferD3D12 == nullptr || pArgumentBufferD3D12->GetD3D12Buffer() == nullptr)
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pArgumentBufferD3D12, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_BufferStateTransition, xiiGALResourceStateFlags::IndirectArgument, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndirectArgument), "indirect mesh draw argument buffer", GetDebugName()))
    return;

  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12      = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  ID3D12CommandSignature*         pCommandSignature = CreateIndirectCommandSignature(pDeviceD3D12->GetD3D12Device(), D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH, sizeof(D3D12_DISPATCH_ARGUMENTS));
  if (pCommandSignature == nullptr)
  {
    xiiLog::Error("Failed to issue DrawMeshIndirect on D3D12 command list '{}': command signature creation failed.", GetDebugName());
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pCommandSignature);
    });

  ID3D12Resource* pCountBuffer = nullptr;
  if (description.m_pCounterBuffer != nullptr)
  {
    xiiGALBufferD3D12* pCounterBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pCounterBuffer);
    if (pCounterBufferD3D12 == nullptr || pCounterBufferD3D12->GetD3D12Buffer() == nullptr)
      return;

    if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pCounterBufferD3D12, pCounterBufferD3D12->GetD3D12Buffer(), description.m_CounterBufferStateTransition, xiiGALResourceStateFlags::IndirectArgument, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndirectArgument), "indirect mesh draw count buffer", GetDebugName()))
      return;

    pCountBuffer = pCounterBufferD3D12->GetD3D12Buffer();
  }

  m_pD3D12CommandList->ExecuteIndirect(pCommandSignature, description.m_uiCommandCount, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_uiDrawArgumentOffset, pCountBuffer, description.m_uiCounterOffset);
}

void xiiGALCommandListD3D12::MultiDrawPlatform(const xiiGALMultiDrawDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  PrepareForDraw();

  if (description.m_uiInstanceCount == 0U)
    return;

  for (const xiiGALMultiDrawItem& drawItem : description.m_pDrawItems)
  {
    if (drawItem.m_uiVertexCount == 0U)
      continue;

    m_pD3D12CommandList->DrawInstanced(drawItem.m_uiVertexCount, description.m_uiInstanceCount, drawItem.m_uiStartVertexLocation, description.m_uiFirstInstanceLocation);
  }
}

void xiiGALCommandListD3D12::MultiDrawIndexedPlatform(const xiiGALMultiDrawIndexedDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  PrepareForIndexedDraw(description.m_IndexType);

  if (description.m_uiInstanceCount == 0U)
    return;

  for (const xiiGALMultiDrawIndexedItem& drawItem : description.m_pDrawItems)
  {
    if (drawItem.m_uiIndexCount == 0U)
      continue;

    m_pD3D12CommandList->DrawIndexedInstanced(drawItem.m_uiIndexCount, description.m_uiInstanceCount, drawItem.m_uiFirstIndexLocation, static_cast<INT>(drawItem.m_uiBaseVertex), description.m_uiFirstInstanceLocation);
  }
}

void xiiGALCommandListD3D12::DispatchComputePlatform(const xiiGALDispatchComputeDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  PrepareForDispatchCompute();

  if (description.m_uiThreadGroupCountX == 0U || description.m_uiThreadGroupCountY == 0U || description.m_uiThreadGroupCountZ == 0U)
    return;

  m_pD3D12CommandList->Dispatch(description.m_uiThreadGroupCountX, description.m_uiThreadGroupCountY, description.m_uiThreadGroupCountZ);
}

void xiiGALCommandListD3D12::DispatchComputeIndirectPlatform(const xiiGALDispatchComputeIndirectDescription& description)
{
  if (m_pD3D12CommandList == nullptr || description.m_pBuffer == nullptr)
    return;

  PrepareForDispatchCompute();

  xiiGALBufferD3D12* pArgumentBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pBuffer);
  if (pArgumentBufferD3D12 == nullptr || pArgumentBufferD3D12->GetD3D12Buffer() == nullptr)
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pArgumentBufferD3D12, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_BufferTransitionMode, xiiGALResourceStateFlags::IndirectArgument, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndirectArgument), "indirect dispatch argument buffer", GetDebugName()))
    return;

  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12      = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  ID3D12CommandSignature*         pCommandSignature = CreateIndirectCommandSignature(pDeviceD3D12->GetD3D12Device(), D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, sizeof(D3D12_DISPATCH_ARGUMENTS));
  if (pCommandSignature == nullptr)
  {
    xiiLog::Error("Failed to issue DispatchComputeIndirect on D3D12 command list '{}': command signature creation failed.", GetDebugName());
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pCommandSignature);
    });

  m_pD3D12CommandList->ExecuteIndirect(pCommandSignature, 1U, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_uiDispatchArgumentOffset, nullptr, 0U);
}

void xiiGALCommandListD3D12::TraceRaysPlatform(const xiiGALTraceRaysDescription& description)
{
  if (m_pD3D12CommandList == nullptr || description.m_pShaderBindingTable == nullptr)
    return;

  PrepareForRayTracing();

  xiiGALBufferD3D12* pSBTBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pShaderBindingTable);
  if (pSBTBufferD3D12 == nullptr || pSBTBufferD3D12->GetD3D12Buffer() == nullptr)
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pSBTBufferD3D12, pSBTBufferD3D12->GetD3D12Buffer(), description.m_ShaderBindingTableTransitionMode, xiiGALResourceStateFlags::RayTracing, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::RayTracing), "ray tracing SBT buffer", GetDebugName()))
    return;

  ID3D12GraphicsCommandList4* pD3D12CommandList4 = nullptr;
  if (FAILED(m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList4**>(&pD3D12CommandList4)))) || pD3D12CommandList4 == nullptr)
  {
    xiiLog::Error("Failed to issue TraceRays on D3D12 command list '{}': ID3D12GraphicsCommandList4 is unavailable.", GetDebugName());
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList4);
    });

  const D3D12_GPU_VIRTUAL_ADDRESS uiBaseAddress = pSBTBufferD3D12->GetD3D12BufferGPUVirtualAddress();
  auto                            BuildRegion   = [&](const xiiGALRayTracingSBTRegionDescription& region) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE d3d12Region = {};
    if (region.m_uiSize == 0U)
      return d3d12Region;
    d3d12Region.StartAddress  = uiBaseAddress + region.m_uiOffset;
    d3d12Region.SizeInBytes   = region.m_uiSize;
    d3d12Region.StrideInBytes = region.m_uiStride;
    return d3d12Region;
  };
  auto BuildRayGen = [&](const xiiGALRayTracingSBTRegionDescription& region) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE d3d12Region = {};
    if (region.m_uiSize == 0U)
      return d3d12Region;
    d3d12Region.StartAddress = uiBaseAddress + region.m_uiOffset;
    d3d12Region.SizeInBytes  = region.m_uiSize;
    return d3d12Region;
  };

  D3D12_DISPATCH_RAYS_DESC dispatchRaysDescription  = {};
  dispatchRaysDescription.RayGenerationShaderRecord = BuildRayGen(description.m_RayGenerationTable);
  dispatchRaysDescription.MissShaderTable           = BuildRegion(description.m_MissTable);
  dispatchRaysDescription.HitGroupTable             = BuildRegion(description.m_HitTable);
  dispatchRaysDescription.CallableShaderTable       = BuildRegion(description.m_CallableTable);
  dispatchRaysDescription.Width                     = description.m_uiWidth;
  dispatchRaysDescription.Height                    = description.m_uiHeight;
  dispatchRaysDescription.Depth                     = description.m_uiDepth;

  if (dispatchRaysDescription.Width > 0U && dispatchRaysDescription.Height > 0U && dispatchRaysDescription.Depth > 0U)
  {
    pD3D12CommandList4->DispatchRays(&dispatchRaysDescription);
  }
}

void xiiGALCommandListD3D12::TraceRaysIndirectPlatform(const xiiGALTraceRaysIndirectDescription& description)
{
  if (m_pD3D12CommandList == nullptr || description.m_pShaderBindingTable == nullptr || description.m_pArgumentBuffer == nullptr)
    return;

  PrepareForRayTracing();

  xiiGALBufferD3D12* pSBTBufferD3D12      = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pShaderBindingTable);
  xiiGALBufferD3D12* pArgumentBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pArgumentBuffer);
  if (pSBTBufferD3D12 == nullptr || pArgumentBufferD3D12 == nullptr || pSBTBufferD3D12->GetD3D12Buffer() == nullptr || pArgumentBufferD3D12->GetD3D12Buffer() == nullptr)
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pSBTBufferD3D12, pSBTBufferD3D12->GetD3D12Buffer(), description.m_ShaderBindingTableTransitionMode, xiiGALResourceStateFlags::RayTracing, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::RayTracing), "ray tracing SBT buffer", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pArgumentBufferD3D12, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_ArgumentBufferTransitionMode, xiiGALResourceStateFlags::IndirectArgument, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::IndirectArgument), "indirect ray tracing argument buffer", GetDebugName()))
    return;

  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12      = m_pDevice.Downcast<xiiGALDeviceD3D12>();
  ID3D12CommandSignature*         pCommandSignature = CreateIndirectCommandSignature(pDeviceD3D12->GetD3D12Device(), D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS, sizeof(D3D12_DISPATCH_RAYS_DESC));
  if (pCommandSignature == nullptr)
  {
    xiiLog::Error("Failed to issue TraceRaysIndirect on D3D12 command list '{}': command signature creation failed.", GetDebugName());
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pCommandSignature);
    });

  m_pD3D12CommandList->ExecuteIndirect(pCommandSignature, 1U, pArgumentBufferD3D12->GetD3D12Buffer(), description.m_uiArgumentOffset, nullptr, 0U);
}

void xiiGALCommandListD3D12::UpdateSBTPlatform(const xiiGALUpdateSBTDescription& description)
{
  if (m_pD3D12CommandList == nullptr || description.m_pShaderBindingTable == nullptr)
    return;

  xiiGALRayTracingPipelineState*      pPipelineState      = description.m_pPipelineState != nullptr ? description.m_pPipelineState : xiiDynamicCast<xiiGALRayTracingPipelineState*>(m_pPipelineState);
  xiiGALRayTracingPipelineStateD3D12* pPipelineStateD3D12 = xiiDynamicCast<xiiGALRayTracingPipelineStateD3D12*>(pPipelineState);
  if (pPipelineStateD3D12 == nullptr)
  {
    xiiLog::Error("Failed to update SBT on D3D12 command list '{}': no compatible ray tracing pipeline state is available.", GetDebugName());
    return;
  }

  xiiGALBufferD3D12* pSBTBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pShaderBindingTable);
  if (pSBTBufferD3D12 == nullptr || pSBTBufferD3D12->GetD3D12Buffer() == nullptr)
    return;

  const xiiUInt32             uiHandleSize       = m_pDevice.Downcast<xiiGALDeviceD3D12>()->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiShaderGroupHandleSize;
  xiiArrayPtr<const xiiUInt8> shaderGroupHandles = pPipelineStateD3D12->GetShaderGroupHandles();
  if (uiHandleSize == 0U || shaderGroupHandles.IsEmpty())
    return;

  struct RecordWrite
  {
    xiiUInt64 m_uiDestinationOffset = 0U;
    xiiUInt32 m_uiGroupIndex        = 0U;
  };

  xiiHybridArray<RecordWrite, 16U> writes;
  auto                             CollectWrites = [&](const xiiGALRayTracingSBTRegionDescription& region, xiiArrayPtr<const xiiUInt32> groupIndices, xiiUInt32 uiStartIndex) {
    if (region.m_uiSize == 0U || region.m_uiStride == 0U)
      return;

    const xiiUInt32 uiRecordCount = static_cast<xiiUInt32>(region.m_uiSize / region.m_uiStride);
    for (xiiUInt32 i = 0U; i < uiRecordCount && (uiStartIndex + i) < groupIndices.GetCount(); ++i)
    {
      RecordWrite& write          = writes.ExpandAndGetRef();
      write.m_uiDestinationOffset = region.m_uiOffset + static_cast<xiiUInt64>(i) * region.m_uiStride;
      write.m_uiGroupIndex        = groupIndices[uiStartIndex + i];
    }
  };

  CollectWrites(description.m_RayGenerationTable, pPipelineStateD3D12->GetRayGenerationGroupIndices(), description.m_uiRayGenerationShaderStartIndex);
  CollectWrites(description.m_MissTable, pPipelineStateD3D12->GetMissGroupIndices(), description.m_uiMissShaderStartIndex);
  CollectWrites(description.m_HitTable, pPipelineStateD3D12->GetHitGroupIndices(), description.m_uiHitGroupStartIndex);
  CollectWrites(description.m_CallableTable, pPipelineStateD3D12->GetCallableGroupIndices(), description.m_uiCallableShaderStartIndex);

  if (writes.IsEmpty())
    return;

  const xiiUInt64                    uiUploadSize      = static_cast<xiiUInt64>(writes.GetCount()) * uiHandleSize;
  xiiGALStagingBufferAllocationD3D12 stagingAllocation = m_CommandListData.m_pUploadStagingBufferPool->Allocate(static_cast<xiiUInt32>(uiUploadSize));
  if (stagingAllocation.m_pD3D12Buffer == nullptr || stagingAllocation.m_pMappedAddress == nullptr)
    return;

  for (xiiUInt32 i = 0U; i < writes.GetCount(); ++i)
  {
    const xiiUInt64 uiSourceOffset = static_cast<xiiUInt64>(writes[i].m_uiGroupIndex) * uiHandleSize;
    if ((uiSourceOffset + uiHandleSize) > shaderGroupHandles.GetCount())
      continue;

    xiiMemoryUtils::RawByteCopy(xiiMemoryUtils::AddByteOffset(stagingAllocation.m_pMappedAddress, static_cast<size_t>(i) * uiHandleSize), shaderGroupHandles.GetPtr() + uiSourceOffset, uiHandleSize);
  }

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pSBTBufferD3D12, pSBTBufferD3D12->GetD3D12Buffer(), description.m_ShaderBindingTableTransitionMode, xiiGALResourceStateFlags::CopyDestination, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::CopyDestination), "SBT destination buffer", GetDebugName()))
    return;

  for (xiiUInt32 i = 0U; i < writes.GetCount(); ++i)
  {
    m_pD3D12CommandList->CopyBufferRegion(pSBTBufferD3D12->GetD3D12Buffer(), writes[i].m_uiDestinationOffset, stagingAllocation.m_pD3D12Buffer, stagingAllocation.m_uiOffset + static_cast<xiiUInt64>(i) * uiHandleSize, uiHandleSize);
  }
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
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList4**>(&pD3D12CommandList4)));
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

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDescription = {};
    geometryDescription.Type                           = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    geometryDescription.Flags                          = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geometryDescription.AABBs.AABBCount                = uiBoxCount;
    geometryDescription.AABBs.AABBs.StartAddress       = pBoundingBoxBufferD3D12->GetD3D12BufferGPUVirtualAddress() + boxBuildData.m_uiBoundingBoxOffset;
    geometryDescription.AABBs.AABBs.StrideInBytes      = uiStride;

    d3d12Geometries.PushBack(geometryDescription);
  }

  xiiBitflags<xiiGALRayTracingBuildASFlags>           buildFlags      = description.m_BuildFlags.IsAnyFlagSet() ? description.m_BuildFlags : blasDescription.m_BuildASFlags;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS d3d12BuildFlags = xiiD3D12TypeConversions::GetAccelerationStructureBuildFlags(buildFlags);
  if (description.m_bUpdate)
  {
    d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3d12BuildDescription = {};
  d3d12BuildDescription.Inputs.Type                                        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  d3d12BuildDescription.Inputs.DescsLayout                                 = D3D12_ELEMENTS_LAYOUT_ARRAY;
  d3d12BuildDescription.Inputs.Flags                                       = d3d12BuildFlags;
  d3d12BuildDescription.Inputs.NumDescs                                    = d3d12Geometries.GetCount();
  d3d12BuildDescription.Inputs.pGeometryDescs                              = d3d12Geometries.GetData();
  d3d12BuildDescription.SourceAccelerationStructureData                    = description.m_bUpdate ? pBottomLevelASD3D12->GetD3D12GPUVirtualAddress() : 0ULL;
  d3d12BuildDescription.DestAccelerationStructureData                      = pBottomLevelASD3D12->GetD3D12GPUVirtualAddress();
  d3d12BuildDescription.ScratchAccelerationStructureData                   = pScratchBufferD3D12->GetD3D12BufferGPUVirtualAddress() + description.m_uiScratchBufferOffset;

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
  if (m_pD3D12CommandList == nullptr)
    return;

  xiiGALTopLevelASD3D12* pTopLevelASD3D12     = xiiDynamicCast<xiiGALTopLevelASD3D12*>(description.m_pTopLevelAS);
  xiiGALBufferD3D12*     pInstanceBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pInstanceBuffer);
  xiiGALBufferD3D12*     pScratchBufferD3D12  = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pScratchBuffer);
  if (pTopLevelASD3D12 == nullptr || pInstanceBufferD3D12 == nullptr || pScratchBufferD3D12 == nullptr)
  {
    xiiLog::Error("Failed to build TLAS on D3D12 command list '{}': incompatible TLAS/instance/scratch backend resource types.", GetDebugName());
    return;
  }

  ID3D12Resource* pD3D12TLASResource     = pTopLevelASD3D12->GetD3D12Resource();
  ID3D12Resource* pD3D12InstanceResource = pInstanceBufferD3D12->GetD3D12Buffer();
  ID3D12Resource* pD3D12ScratchResource  = pScratchBufferD3D12->GetD3D12Buffer();
  if (pD3D12TLASResource == nullptr || pD3D12InstanceResource == nullptr || pD3D12ScratchResource == nullptr)
  {
    xiiLog::Error("Failed to build TLAS on D3D12 command list '{}': TLAS/instance/scratch native resources are unavailable.", GetDebugName());
    return;
  }

  ID3D12GraphicsCommandList4* pD3D12CommandList4 = nullptr;
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList4**>(&pD3D12CommandList4)));
  if (FAILED(hResult) || pD3D12CommandList4 == nullptr)
  {
    xiiLog::Error("Failed to build TLAS on D3D12 command list '{}': ID3D12GraphicsCommandList4 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList4);
    });

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTopLevelASD3D12, pD3D12TLASResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASWrite, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "TLAS destination resource", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pInstanceBufferD3D12, pD3D12InstanceResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::ShaderResource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "TLAS instance buffer", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pScratchBufferD3D12, pD3D12ScratchResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASWrite, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "TLAS scratch buffer", GetDebugName()))
    return;

  xiiBitflags<xiiGALRayTracingBuildASFlags>           buildFlags      = description.m_BuildFlags.IsAnyFlagSet() ? description.m_BuildFlags : description.m_pTopLevelAS->GetDescription().m_Flags;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS d3d12BuildFlags = xiiD3D12TypeConversions::GetAccelerationStructureBuildFlags(buildFlags);
  if (description.m_bUpdate)
  {
    d3d12BuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3d12BuildDescription = {};
  d3d12BuildDescription.Inputs.Type                                        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  d3d12BuildDescription.Inputs.DescsLayout                                 = D3D12_ELEMENTS_LAYOUT_ARRAY;
  d3d12BuildDescription.Inputs.Flags                                       = d3d12BuildFlags;
  d3d12BuildDescription.Inputs.NumDescs                                    = description.m_uiInstanceCount;
  d3d12BuildDescription.Inputs.InstanceDescs                               = pInstanceBufferD3D12->GetD3D12BufferGPUVirtualAddress() + description.m_uiInstanceBufferOffset;
  d3d12BuildDescription.SourceAccelerationStructureData                    = description.m_bUpdate ? pTopLevelASD3D12->GetD3D12GPUVirtualAddress() : 0ULL;
  d3d12BuildDescription.DestAccelerationStructureData                      = pTopLevelASD3D12->GetD3D12GPUVirtualAddress();
  d3d12BuildDescription.ScratchAccelerationStructureData                   = pScratchBufferD3D12->GetD3D12BufferGPUVirtualAddress() + description.m_uiScratchBufferOffset;

  pD3D12CommandList4->BuildRaytracingAccelerationStructure(&d3d12BuildDescription, 0U, nullptr);

  D3D12_RESOURCE_BARRIER uavBarriers[2] = {};
  uavBarriers[0].Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  uavBarriers[0].UAV.pResource          = pD3D12TLASResource;
  uavBarriers[1].Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  uavBarriers[1].UAV.pResource          = pD3D12ScratchResource;
  m_pD3D12CommandList->ResourceBarrier(2U, uavBarriers);
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
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList4**>(&pD3D12CommandList4)));
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
  if (m_pD3D12CommandList == nullptr)
    return;

  xiiGALTopLevelASD3D12* pSourceTopLevelASD3D12      = xiiDynamicCast<xiiGALTopLevelASD3D12*>(description.m_pSourceTopLevelAS);
  xiiGALTopLevelASD3D12* pDestinationTopLevelASD3D12 = xiiDynamicCast<xiiGALTopLevelASD3D12*>(description.m_pDestinationTopLevelAS);
  if (pSourceTopLevelASD3D12 == nullptr || pDestinationTopLevelASD3D12 == nullptr)
  {
    xiiLog::Error("Failed to copy TLAS on D3D12 command list '{}': incompatible source or destination backend type.", GetDebugName());
    return;
  }

  ID3D12Resource* pD3D12SourceResource      = pSourceTopLevelASD3D12->GetD3D12Resource();
  ID3D12Resource* pD3D12DestinationResource = pDestinationTopLevelASD3D12->GetD3D12Resource();
  if (pD3D12SourceResource == nullptr || pD3D12DestinationResource == nullptr)
  {
    xiiLog::Error("Failed to copy TLAS on D3D12 command list '{}': source or destination native resource is unavailable.", GetDebugName());
    return;
  }

  ID3D12GraphicsCommandList4* pD3D12CommandList4 = nullptr;
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList4**>(&pD3D12CommandList4)));
  if (FAILED(hResult) || pD3D12CommandList4 == nullptr)
  {
    xiiLog::Error("Failed to copy TLAS on D3D12 command list '{}': ID3D12GraphicsCommandList4 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList4);
    });

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pSourceTopLevelASD3D12, pD3D12SourceResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "TLAS copy source", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDestinationTopLevelASD3D12, pD3D12DestinationResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASWrite, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "TLAS copy destination", GetDebugName()))
    return;

  const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE d3d12CopyMode = description.m_Mode == xiiGALASCopyMode::Compact ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;

  pD3D12CommandList4->CopyRaytracingAccelerationStructure(pDestinationTopLevelASD3D12->GetD3D12GPUVirtualAddress(), pSourceTopLevelASD3D12->GetD3D12GPUVirtualAddress(), d3d12CopyMode);
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
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList4**>(&pD3D12CommandList4)));
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
  postBuildDescription.DestBuffer                                                  = d3d12DestinationAddress;
  postBuildDescription.InfoType                                                    = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

  pD3D12CommandList4->EmitRaytracingAccelerationStructurePostbuildInfo(&postBuildDescription, 1U, &d3d12SourceAddress);
}

void xiiGALCommandListD3D12::WriteTLASCompactedSizePlatform(const xiiGALWriteTLASCompactedSizeDescription& description)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  xiiGALTopLevelASD3D12* pTopLevelASD3D12        = xiiDynamicCast<xiiGALTopLevelASD3D12*>(description.m_pTopLevelAS);
  xiiGALBufferD3D12*     pDestinationBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(description.m_pDestinationBuffer);
  if (pTopLevelASD3D12 == nullptr || pDestinationBufferD3D12 == nullptr)
  {
    xiiLog::Error("Failed to write TLAS compacted size on D3D12 command list '{}': incompatible TLAS/destination backend types.", GetDebugName());
    return;
  }

  ID3D12Resource* pD3D12TLASResource              = pTopLevelASD3D12->GetD3D12Resource();
  ID3D12Resource* pD3D12DestinationBufferResource = pDestinationBufferD3D12->GetD3D12Buffer();
  if (pD3D12TLASResource == nullptr || pD3D12DestinationBufferResource == nullptr)
  {
    xiiLog::Error("Failed to write TLAS compacted size on D3D12 command list '{}': TLAS/destination native resources are unavailable.", GetDebugName());
    return;
  }

  ID3D12GraphicsCommandList4* pD3D12CommandList4 = nullptr;
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList4**>(&pD3D12CommandList4)));
  if (FAILED(hResult) || pD3D12CommandList4 == nullptr)
  {
    xiiLog::Error("Failed to write TLAS compacted size on D3D12 command list '{}': ID3D12GraphicsCommandList4 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList4);
    });

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTopLevelASD3D12, pD3D12TLASResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::BuildASRead, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, "TLAS compacted-size source", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDestinationBufferD3D12, pD3D12DestinationBufferResource, description.m_ResourceStateTransitionMode, xiiGALResourceStateFlags::UnorderedAccess, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "TLAS compacted-size destination buffer", GetDebugName()))
    return;

  const D3D12_GPU_VIRTUAL_ADDRESS d3d12DestinationAddress = pDestinationBufferD3D12->GetD3D12BufferGPUVirtualAddress() + description.m_uiDestinationBufferOffset;
  const D3D12_GPU_VIRTUAL_ADDRESS d3d12SourceAddress      = pTopLevelASD3D12->GetD3D12GPUVirtualAddress();
  if (d3d12DestinationAddress == 0ULL || d3d12SourceAddress == 0ULL)
  {
    xiiLog::Error("Failed to write TLAS compacted size on D3D12 command list '{}': invalid source or destination GPU virtual address.", GetDebugName());
    return;
  }

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildDescription = {};
  postBuildDescription.DestBuffer                                                  = d3d12DestinationAddress;
  postBuildDescription.InfoType                                                    = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

  pD3D12CommandList4->EmitRaytracingAccelerationStructurePostbuildInfo(&postBuildDescription, 1U, &d3d12SourceAddress);
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

  const xiiGALQueryType::Enum queryType       = pQueryD3D12->GetDescription().m_Type;
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
  if (m_pD3D12CommandList == nullptr || pBuffer == nullptr || pSourceData.IsEmpty())
    return;

  xiiGALBufferD3D12* pBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(pBuffer);
  if (pBufferD3D12 == nullptr || pBufferD3D12->GetD3D12Buffer() == nullptr)
  {
    xiiLog::Error("Failed to update D3D12 buffer on command list '{}': incompatible backend buffer type.", GetDebugName());
    return;
  }

  const xiiUInt64 uiUpdateSize = pSourceData.GetCount();
  if (uiDestinationOffset + uiUpdateSize > pBufferD3D12->GetSize())
  {
    xiiLog::Error("Failed to update D3D12 buffer '{}': destination range [{}..{}) exceeds buffer size {}.", pBufferD3D12->GetDebugName(), uiDestinationOffset, uiDestinationOffset + uiUpdateSize, pBufferD3D12->GetSize());
    return;
  }

  const xiiGALBufferCreationDescription& bufferDescription = pBufferD3D12->GetDescription();
  if (bufferDescription.m_Usage == xiiGALResourceUsage::Dynamic || bufferDescription.m_Usage == xiiGALResourceUsage::Staging || bufferDescription.m_Usage == xiiGALResourceUsage::Unified)
  {
    void*       pMappedData = nullptr;
    D3D12_RANGE readRange   = {0U, 0U};
    if (FAILED(pBufferD3D12->GetD3D12Buffer()->Map(0U, &readRange, &pMappedData)) || pMappedData == nullptr)
    {
      xiiLog::Error("Failed to map host-visible D3D12 buffer '{}' for UpdateBuffer().", pBufferD3D12->GetDebugName());
      return;
    }

    xiiMemoryUtils::RawByteCopy(xiiMemoryUtils::AddByteOffset(pMappedData, static_cast<size_t>(uiDestinationOffset)), pSourceData.GetPtr(), static_cast<size_t>(uiUpdateSize));

    D3D12_RANGE writeRange = {static_cast<SIZE_T>(uiDestinationOffset), static_cast<SIZE_T>(uiDestinationOffset + uiUpdateSize)};
    pBufferD3D12->GetD3D12Buffer()->Unmap(0U, &writeRange);
    return;
  }

  xiiGALStagingBufferAllocationD3D12 stagingAllocation = m_CommandListData.m_pUploadStagingBufferPool->Allocate(static_cast<xiiUInt32>(uiUpdateSize));
  if (stagingAllocation.m_pD3D12Buffer == nullptr || stagingAllocation.m_pMappedAddress == nullptr)
  {
    xiiLog::Error("Failed to allocate upload staging memory for D3D12 UpdateBuffer().");
    return;
  }

  xiiMemoryUtils::RawByteCopy(stagingAllocation.m_pMappedAddress, pSourceData.GetPtr(), static_cast<size_t>(uiUpdateSize));

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pBufferD3D12, pBufferD3D12->GetD3D12Buffer(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::CopyDestination), "buffer update destination", GetDebugName()))
    return;

  m_pD3D12CommandList->CopyBufferRegion(pBufferD3D12->GetD3D12Buffer(), static_cast<UINT64>(uiDestinationOffset), stagingAllocation.m_pD3D12Buffer, stagingAllocation.m_uiOffset, static_cast<UINT64>(uiUpdateSize));
}

void xiiGALCommandListD3D12::CopyBufferPlatform(xiiGALBuffer* pSourceBuffer, xiiGALBuffer* pDestinationBuffer)
{
  if (pSourceBuffer == nullptr || pDestinationBuffer == nullptr)
    return;

  xiiGALBufferD3D12* pSourceBufferD3D12      = xiiDynamicCast<xiiGALBufferD3D12*>(pSourceBuffer);
  xiiGALBufferD3D12* pDestinationBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(pDestinationBuffer);
  if (pSourceBufferD3D12 == nullptr || pDestinationBufferD3D12 == nullptr || pSourceBufferD3D12->GetD3D12Buffer() == nullptr || pDestinationBufferD3D12->GetD3D12Buffer() == nullptr)
  {
    xiiLog::Error("Failed to copy D3D12 buffers on command list '{}': incompatible backend buffer types.", GetDebugName());
    return;
  }

  CopyBufferRegionPlatform(pSourceBufferD3D12, 0U, pDestinationBufferD3D12, 0U, pSourceBufferD3D12->GetSize());
}

void xiiGALCommandListD3D12::CopyBufferRegionPlatform(xiiGALBuffer* pSourceBuffer, xiiUInt64 uiSourceOffset, xiiGALBuffer* pDestinationBuffer, xiiUInt64 uiDestinationOffset, xiiUInt64 uiSize)
{
  if (m_pD3D12CommandList == nullptr || pSourceBuffer == nullptr || pDestinationBuffer == nullptr || uiSize == 0U)
    return;

  xiiGALBufferD3D12* pSourceBufferD3D12      = xiiDynamicCast<xiiGALBufferD3D12*>(pSourceBuffer);
  xiiGALBufferD3D12* pDestinationBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(pDestinationBuffer);
  if (pSourceBufferD3D12 == nullptr || pDestinationBufferD3D12 == nullptr || pSourceBufferD3D12->GetD3D12Buffer() == nullptr || pDestinationBufferD3D12->GetD3D12Buffer() == nullptr)
  {
    xiiLog::Error("Failed to copy D3D12 buffer region on command list '{}': incompatible backend buffer types.", GetDebugName());
    return;
  }

  if (uiSourceOffset + uiSize > pSourceBufferD3D12->GetSize() || uiDestinationOffset + uiSize > pDestinationBufferD3D12->GetSize())
  {
    xiiLog::Error("Failed to copy D3D12 buffer region on command list '{}': source/destination range exceeds buffer bounds.", GetDebugName());
    return;
  }

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pSourceBufferD3D12, pSourceBufferD3D12->GetD3D12Buffer(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopySource, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::CopySource), "copy source buffer", GetDebugName()))
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDestinationBufferD3D12, pDestinationBufferD3D12->GetD3D12Buffer(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::CopyDestination), "copy destination buffer", GetDebugName()))
    return;

  m_pD3D12CommandList->CopyBufferRegion(pDestinationBufferD3D12->GetD3D12Buffer(), uiDestinationOffset, pSourceBufferD3D12->GetD3D12Buffer(), uiSourceOffset, uiSize);
}

xiiResult xiiGALCommandListD3D12::MapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, void*& pMappedData)
{
  XII_IGNORE_UNUSED(mapFlags);

  pMappedData = nullptr;
  if (pBuffer == nullptr)
    return XII_FAILURE;

  xiiGALBufferD3D12* pBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(pBuffer);
  if (pBufferD3D12 == nullptr || pBufferD3D12->GetD3D12Buffer() == nullptr)
  {
    xiiLog::Error("Failed to map D3D12 buffer: incompatible backend buffer type.");
    return XII_FAILURE;
  }

  const xiiGALBufferCreationDescription& description = pBufferD3D12->GetDescription();
  if ((mapType == xiiGALMapType::Read || mapType == xiiGALMapType::ReadWrite) && !description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
  {
    xiiLog::Error("Failed to map D3D12 buffer '{}' for reading: CPU read access flag is missing.", pBufferD3D12->GetDebugName());
    return XII_FAILURE;
  }
  if ((mapType == xiiGALMapType::Write || mapType == xiiGALMapType::ReadWrite) && !description.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
  {
    xiiLog::Error("Failed to map D3D12 buffer '{}' for writing: CPU write access flag is missing.", pBufferD3D12->GetDebugName());
    return XII_FAILURE;
  }

  D3D12_RANGE readRange = {};
  if (mapType == xiiGALMapType::Read || mapType == xiiGALMapType::ReadWrite)
  {
    readRange.Begin = 0U;
    readRange.End   = static_cast<SIZE_T>(pBufferD3D12->GetSize());
  }

  if (FAILED(pBufferD3D12->GetD3D12Buffer()->Map(0U, &readRange, &pMappedData)) || pMappedData == nullptr)
  {
    xiiLog::Error("Failed to map D3D12 buffer '{}'.", pBufferD3D12->GetDebugName());
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::UnmapBufferPlatform(xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType)
{
  if (pBuffer == nullptr)
    return XII_FAILURE;

  xiiGALBufferD3D12* pBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(pBuffer);
  if (pBufferD3D12 == nullptr || pBufferD3D12->GetD3D12Buffer() == nullptr)
  {
    xiiLog::Error("Failed to unmap D3D12 buffer: incompatible backend buffer type.");
    return XII_FAILURE;
  }

  D3D12_RANGE writtenRange = {};
  if (mapType == xiiGALMapType::Write || mapType == xiiGALMapType::ReadWrite)
  {
    writtenRange.Begin = 0U;
    writtenRange.End   = static_cast<SIZE_T>(pBufferD3D12->GetSize());
  }
  pBufferD3D12->GetD3D12Buffer()->Unmap(0U, &writtenRange);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::UpdateTexturePlatform(xiiGALTexture* pTexture, const xiiGALTextureMipLevelData& textureMiplevelData, const xiiBoundingBoxU32& textureBox, const xiiGALTextureSubResourceData& subresourceData)
{
  if (m_pD3D12CommandList == nullptr || pTexture == nullptr || subresourceData.m_pData.IsEmpty())
    return;

  xiiGALTextureD3D12* pTextureD3D12 = xiiDynamicCast<xiiGALTextureD3D12*>(pTexture);
  if (pTextureD3D12 == nullptr || pTextureD3D12->GetD3D12Texture() == nullptr)
  {
    xiiLog::Error("Failed to update D3D12 texture on command list '{}': incompatible backend texture type.", GetDebugName());
    return;
  }

  const xiiGALTextureCreationDescription& textureDescription = pTextureD3D12->GetDescription();
  if (textureDescription.m_Usage == xiiGALResourceUsage::Staging)
  {
    xiiGALMappedTextureSubresource mappedSubresource = {};
    if (MapTextureSubresourcePlatform(pTextureD3D12, textureMiplevelData, xiiGALMapType::Write, xiiGALMapFlags::None, nullptr, mappedSubresource).Failed())
      return;

    const xiiGALResourceFormatDescription& formatProperties   = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);
    const xiiUInt32                        uiRowCount         = (textureBox.m_vMax.y - textureBox.m_vMin.y) / xiiMath::Max<xiiUInt32>(formatProperties.m_uiBlockHeight, 1U);
    const xiiUInt32                        uiDepth            = textureBox.m_vMax.z - textureBox.m_vMin.z;
    const xiiUInt32                        uiBoxWidthInBlocks = (textureBox.m_vMax.x - textureBox.m_vMin.x) / xiiMath::Max<xiiUInt32>(formatProperties.m_uiBlockWidth, 1U);
    const xiiUInt64                        uiRowSize          = static_cast<xiiUInt64>(uiBoxWidthInBlocks) * static_cast<xiiUInt64>(formatProperties.GetElementSize());
    xiiGALTextureUtilities::CopyTextureSubresource(subresourceData, uiRowCount, uiDepth, uiRowSize, mappedSubresource.m_pData, mappedSubresource.m_uiStride, mappedSubresource.m_uiDepthStride);
    XII_IGNORE_UNUSED(UnmapTextureSubresourcePlatform(pTextureD3D12, textureMiplevelData));
    return;
  }

  D3D12_RESOURCE_DESC d3d12TextureDescription = pTextureD3D12->GetD3D12Texture()->GetDesc();
  const UINT          uiSubresourceIndex      = xiiD3D12TypeConversions::CalculateSubResourceIndex(textureMiplevelData.m_uiMipLevel, textureMiplevelData.m_uiArraySlice, textureDescription.m_uiMipLevels);

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint = {};
  UINT                               uiRowsCount     = 0U;
  UINT64                             uiRowSize       = 0U;
  UINT64                             uiRequiredSize  = 0U;
  m_pDevice.Downcast<xiiGALDeviceD3D12>()->GetD3D12Device()->GetCopyableFootprints(&d3d12TextureDescription, uiSubresourceIndex, 1U, 0U, &placedFootprint, &uiRowsCount, &uiRowSize, &uiRequiredSize);

  xiiGALStagingBufferAllocationD3D12 stagingAllocation = m_CommandListData.m_pUploadStagingBufferPool->Allocate(static_cast<xiiUInt32>(uiRequiredSize));
  if (stagingAllocation.m_pD3D12Buffer == nullptr || stagingAllocation.m_pMappedAddress == nullptr)
  {
    xiiLog::Error("Failed to allocate D3D12 staging memory for UpdateTexture().");
    return;
  }

  const xiiUInt32 uiDepth = textureBox.m_vMax.z - textureBox.m_vMin.z;
  xiiGALTextureUtilities::CopyTextureSubresource(subresourceData, uiRowsCount, uiDepth, uiRowSize, stagingAllocation.m_pMappedAddress, placedFootprint.Footprint.RowPitch, placedFootprint.Footprint.RowPitch * uiRowsCount);

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pTextureD3D12, pTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::CopyDestination), "texture update destination", GetDebugName()))
    return;

  D3D12_TEXTURE_COPY_LOCATION destinationLocation = {};
  destinationLocation.pResource                   = pTextureD3D12->GetD3D12Texture();
  destinationLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  destinationLocation.SubresourceIndex            = uiSubresourceIndex;

  D3D12_TEXTURE_COPY_LOCATION sourceLocation = {};
  sourceLocation.pResource                   = stagingAllocation.m_pD3D12Buffer;
  sourceLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  sourceLocation.PlacedFootprint             = placedFootprint;
  sourceLocation.PlacedFootprint.Offset      = stagingAllocation.m_uiOffset;

  D3D12_BOX sourceBox = {};
  sourceBox.left      = 0U;
  sourceBox.top       = 0U;
  sourceBox.front     = 0U;
  sourceBox.right     = textureBox.m_vMax.x - textureBox.m_vMin.x;
  sourceBox.bottom    = textureBox.m_vMax.y - textureBox.m_vMin.y;
  sourceBox.back      = textureBox.m_vMax.z - textureBox.m_vMin.z;

  m_pD3D12CommandList->CopyTextureRegion(&destinationLocation, textureBox.m_vMin.x, textureBox.m_vMin.y, textureBox.m_vMin.z, &sourceLocation, &sourceBox);
}

void xiiGALCommandListD3D12::CopyTexturePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture)
{
  if (pSourceTexture == nullptr || pDestinationTexture == nullptr)
    return;

  xiiGALTextureD3D12* pSourceTextureD3D12      = xiiDynamicCast<xiiGALTextureD3D12*>(pSourceTexture);
  xiiGALTextureD3D12* pDestinationTextureD3D12 = xiiDynamicCast<xiiGALTextureD3D12*>(pDestinationTexture);
  if (pSourceTextureD3D12 == nullptr || pDestinationTextureD3D12 == nullptr)
    return;

  const xiiGALTextureCreationDescription& sourceDescription = pSourceTextureD3D12->GetDescription();

  xiiBoundingBoxU32              copyBox             = xiiBoundingBoxU32::MakeZero();
  const xiiGALMipLevelProperties sourceMipProperties = xiiGALTextureUtilities::GetMipLevelProperties(sourceDescription, 0U);
  copyBox.m_vMax                                     = xiiVec3U32(sourceMipProperties.m_LogicalSize.width, sourceMipProperties.m_LogicalSize.height, sourceMipProperties.m_uiDepth);

  xiiGALTextureMipLevelData sourceMipData      = {};
  xiiGALTextureMipLevelData destinationMipData = {};
  sourceMipData.m_uiMipLevel                   = 0U;
  sourceMipData.m_uiArraySlice                 = 0U;
  destinationMipData.m_uiMipLevel              = 0U;
  destinationMipData.m_uiArraySlice            = 0U;
  CopyTextureRegionPlatform(pSourceTextureD3D12, sourceMipData, copyBox, pDestinationTextureD3D12, destinationMipData, xiiVec3U32::MakeZero());
}

void xiiGALCommandListD3D12::CopyTextureRegionPlatform(xiiGALTexture* pSourceTexture, const xiiGALTextureMipLevelData& sourceMipLevelData, const xiiBoundingBoxU32& box, xiiGALTexture* pDestinationTexture, const xiiGALTextureMipLevelData& destinationMipLevelData, const xiiVec3U32& vDestinationPoint)
{
  if (m_pD3D12CommandList == nullptr || pSourceTexture == nullptr || pDestinationTexture == nullptr)
    return;

  xiiGALTextureD3D12* pSourceTextureD3D12      = xiiDynamicCast<xiiGALTextureD3D12*>(pSourceTexture);
  xiiGALTextureD3D12* pDestinationTextureD3D12 = xiiDynamicCast<xiiGALTextureD3D12*>(pDestinationTexture);
  if (pSourceTextureD3D12 == nullptr || pDestinationTextureD3D12 == nullptr || pSourceTextureD3D12->GetD3D12Texture() == nullptr || pDestinationTextureD3D12->GetD3D12Texture() == nullptr)
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pSourceTextureD3D12, pSourceTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopySource, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::CopySource), "texture copy source", GetDebugName()))
    return;
  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDestinationTextureD3D12, pDestinationTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::CopyDestination, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::CopyDestination), "texture copy destination", GetDebugName()))
    return;

  const xiiUInt32 uiSourceSubresourceIndex      = xiiD3D12TypeConversions::CalculateSubResourceIndex(sourceMipLevelData.m_uiMipLevel, sourceMipLevelData.m_uiArraySlice, pSourceTextureD3D12->GetDescription().m_uiMipLevels);
  const xiiUInt32 uiDestinationSubresourceIndex = xiiD3D12TypeConversions::CalculateSubResourceIndex(destinationMipLevelData.m_uiMipLevel, destinationMipLevelData.m_uiArraySlice, pDestinationTextureD3D12->GetDescription().m_uiMipLevels);

  D3D12_TEXTURE_COPY_LOCATION sourceLocation = {};
  sourceLocation.pResource                   = pSourceTextureD3D12->GetD3D12Texture();
  sourceLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  sourceLocation.SubresourceIndex            = uiSourceSubresourceIndex;

  D3D12_TEXTURE_COPY_LOCATION destinationLocation = {};
  destinationLocation.pResource                   = pDestinationTextureD3D12->GetD3D12Texture();
  destinationLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  destinationLocation.SubresourceIndex            = uiDestinationSubresourceIndex;

  D3D12_BOX sourceBox = {};
  sourceBox.left      = box.m_vMin.x;
  sourceBox.top       = box.m_vMin.y;
  sourceBox.front     = box.m_vMin.z;
  sourceBox.right     = box.m_vMax.x;
  sourceBox.bottom    = box.m_vMax.y;
  sourceBox.back      = box.m_vMax.z;

  m_pD3D12CommandList->CopyTextureRegion(&destinationLocation, vDestinationPoint.x, vDestinationPoint.y, vDestinationPoint.z, &sourceLocation, &sourceBox);
}

void xiiGALCommandListD3D12::ResolveTextureSubResourcePlatform(xiiGALTexture* pSourceTexture, xiiGALTexture* pDestinationTexture, const xiiGALResolveTextureSubresourceDescription& description)
{
  if (m_pD3D12CommandList == nullptr || pSourceTexture == nullptr || pDestinationTexture == nullptr)
    return;

  xiiGALTextureD3D12* pSourceTextureD3D12      = xiiDynamicCast<xiiGALTextureD3D12*>(pSourceTexture);
  xiiGALTextureD3D12* pDestinationTextureD3D12 = xiiDynamicCast<xiiGALTextureD3D12*>(pDestinationTexture);
  if (pSourceTextureD3D12 == nullptr || pDestinationTextureD3D12 == nullptr || pSourceTextureD3D12->GetD3D12Texture() == nullptr || pDestinationTextureD3D12->GetD3D12Texture() == nullptr)
    return;

  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pSourceTextureD3D12, pSourceTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::ResolveSource, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::ResolveSource), "resolve source texture", GetDebugName()))
    return;
  if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDestinationTextureD3D12, pDestinationTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::ResolveDestination, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::ResolveDestination), "resolve destination texture", GetDebugName()))
    return;

  const UINT        uiSourceSubresource      = xiiD3D12TypeConversions::CalculateSubResourceIndex(description.m_uiSourceMipLevel, description.m_uiSourceSlice, pSourceTextureD3D12->GetDescription().m_uiMipLevels);
  const UINT        uiDestinationSubresource = xiiD3D12TypeConversions::CalculateSubResourceIndex(description.m_uiDestinationMipLevel, description.m_uiDestinationSlice, pDestinationTextureD3D12->GetDescription().m_uiMipLevels);
  const DXGI_FORMAT dxgiFormat               = xiiD3D12TypeConversions::GetFormat(description.m_Format == xiiGALResourceFormat::Unknown ? pDestinationTextureD3D12->GetDescription().m_Format : description.m_Format);

  m_pD3D12CommandList->ResolveSubresource(pDestinationTextureD3D12->GetD3D12Texture(), uiDestinationSubresource, pSourceTextureD3D12->GetD3D12Texture(), uiSourceSubresource, dxgiFormat);
}

xiiResult xiiGALCommandListD3D12::MapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags, xiiBoundingBoxU32* pTextureBox, xiiGALMappedTextureSubresource& mappedData)
{
  XII_IGNORE_UNUSED(mapFlags);

  mappedData = {};
  if (pTexture == nullptr)
    return XII_FAILURE;

  xiiGALTextureD3D12* pTextureD3D12 = xiiDynamicCast<xiiGALTextureD3D12*>(pTexture);
  if (pTextureD3D12 == nullptr || pTextureD3D12->GetD3D12Texture() == nullptr)
    return XII_FAILURE;

  const xiiGALTextureCreationDescription& textureDescription = pTextureD3D12->GetDescription();
  if (textureDescription.m_Usage != xiiGALResourceUsage::Staging)
  {
    xiiLog::Error("Only staging textures can be mapped in the D3D12 backend.");
    return XII_FAILURE;
  }

  if ((mapType == xiiGALMapType::Read || mapType == xiiGALMapType::ReadWrite) && !textureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Read))
    return XII_FAILURE;
  if ((mapType == xiiGALMapType::Write || mapType == xiiGALMapType::ReadWrite) && !textureDescription.m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
    return XII_FAILURE;

  xiiBoundingBoxU32 resolvedBox = xiiBoundingBoxU32::MakeZero();
  if (pTextureBox == nullptr)
  {
    const xiiGALMipLevelProperties mipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(textureDescription, textureMipLevelData.m_uiMipLevel);
    resolvedBox.m_vMax                                = xiiVec3U32(mipLevelProperties.m_LogicalSize.width, mipLevelProperties.m_LogicalSize.height, mipLevelProperties.m_uiDepth);
    pTextureBox                                       = &resolvedBox;
  }

  const xiiGALResourceFormatDescription& formatProperties   = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);
  const xiiGALMipLevelProperties         mipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(textureDescription, textureMipLevelData.m_uiMipLevel);

  void*       pMappedMemory = nullptr;
  D3D12_RANGE readRange     = {};
  if (mapType == xiiGALMapType::Read || mapType == xiiGALMapType::ReadWrite)
  {
    readRange.Begin = 0U;
    readRange.End   = static_cast<SIZE_T>(pTextureD3D12->GetD3D12Texture()->GetDesc().Width);
  }

  if (FAILED(pTextureD3D12->GetD3D12Texture()->Map(0U, &readRange, &pMappedMemory)) || pMappedMemory == nullptr)
    return XII_FAILURE;

  const xiiUInt64 uiSubresourceOffset = xiiGALTextureUtilities::GetStagingTextureSubresourceOffset(textureDescription, textureMipLevelData.m_uiArraySlice, textureMipLevelData.m_uiMipLevel, 4U);
  const xiiUInt64 uiMapOffset =
    uiSubresourceOffset +
    ((pTextureBox->m_vMin.z * mipLevelProperties.m_StorageSize.height + pTextureBox->m_vMin.y) / xiiMath::Max<xiiUInt32>(formatProperties.m_uiBlockHeight, 1U)) * mipLevelProperties.m_uiRowSize +
    (pTextureBox->m_vMin.x / xiiMath::Max<xiiUInt32>(formatProperties.m_uiBlockWidth, 1U)) * static_cast<xiiUInt64>(formatProperties.GetElementSize());

  mappedData.m_pData         = xiiMemoryUtils::AddByteOffset(pMappedMemory, static_cast<size_t>(uiMapOffset));
  mappedData.m_uiStride      = mipLevelProperties.m_uiRowSize;
  mappedData.m_uiDepthStride = mipLevelProperties.m_uiDepthSliceSize;
  return XII_SUCCESS;
}

xiiResult xiiGALCommandListD3D12::UnmapTextureSubresourcePlatform(xiiGALTexture* pTexture, xiiGALTextureMipLevelData textureMipLevelData)
{
  XII_IGNORE_UNUSED(textureMipLevelData);

  if (pTexture == nullptr)
    return XII_FAILURE;

  xiiGALTextureD3D12* pTextureD3D12 = xiiDynamicCast<xiiGALTextureD3D12*>(pTexture);
  if (pTextureD3D12 == nullptr || pTextureD3D12->GetD3D12Texture() == nullptr)
    return XII_FAILURE;

  D3D12_RANGE writtenRange = {};
  if (pTextureD3D12->GetDescription().m_CPUAccessFlags.IsSet(xiiGALCPUAccessFlag::Write))
  {
    writtenRange.Begin = 0U;
    writtenRange.End   = static_cast<SIZE_T>(pTextureD3D12->GetD3D12Texture()->GetDesc().Width);
  }
  pTextureD3D12->GetD3D12Texture()->Unmap(0U, &writtenRange);

  return XII_SUCCESS;
}

void xiiGALCommandListD3D12::GenerateMipsPlatform(xiiGALTextureView* pTextureView)
{
  XII_IGNORE_UNUSED(pTextureView);

  xiiLog::Error("GenerateMips is currently unsupported in the D3D12 backend.");
}

void xiiGALCommandListD3D12::TransitionResourceStatesPlatform(xiiArrayPtr<xiiGALStateTransitionDescription> pResourceBarriers)
{
  // Batch aliasing barriers and flush in groups.
  xiiTemporaryHybridArray<D3D12_RESOURCE_BARRIER, 16U> aliasingBatch;

  for (xiiUInt32 i = 0; i < pResourceBarriers.GetCount(); ++i)
  {
    const xiiGALStateTransitionDescription& description = pResourceBarriers[i];

    // Aliasing transitions are handled as aliasing barriers.
    if (description.m_TransitionFlags.IsSet(xiiGALStateTransitionFlags::Aliasing))
    {
      D3D12_RESOURCE_BARRIER& batch  = aliasingBatch.ExpandAndGetRef();
      batch.Type                     = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
      batch.Flags                    = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      batch.Aliasing.pResourceBefore = xiiLocalStateTransitionHelper::GetD3D12Resource(description.m_pPreviousResource);
      batch.Aliasing.pResourceAfter  = xiiLocalStateTransitionHelper::GetD3D12Resource(description.m_pResource);

      // Flush in batches to avoid many small calls.
      if (aliasingBatch.GetCount() >= 16)
      {
        m_pD3D12CommandList->ResourceBarrier(aliasingBatch.GetCount(), aliasingBatch.GetData());

        aliasingBatch.Clear();
      }
      continue;
    }

    xiiLocalStateTransitionHelper helper(description, this);
    helper.Process();
  }

  // Flush remaining aliasing barriers.
  if (!aliasingBatch.IsEmpty())
  {
    m_pD3D12CommandList->ResourceBarrier(aliasingBatch.GetCount(), aliasingBatch.GetData());

    aliasingBatch.Clear();
  }
}

void xiiGALCommandListD3D12::SetShadingRatePlatform(xiiBitflags<xiiGALShadingRateFlags> baseRateFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> primitiveCombinerFlags, xiiBitflags<xiiGALShadingRateCombinerFlags> textureCombinerFlags)
{
  if (m_pD3D12CommandList == nullptr)
    return;

  ID3D12GraphicsCommandList5* pD3D12CommandList5 = nullptr;
  const HRESULT               hResult            = m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList5), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList5**>(&pD3D12CommandList5)));
  if (FAILED(hResult) || pD3D12CommandList5 == nullptr)
  {
    xiiLog::Error("Failed to set shading rate on D3D12 command list '{}': ID3D12GraphicsCommandList5 interface is unavailable ({}).", GetDebugName(), xiiHRESULTtoString(hResult));
    return;
  }

  XII_SCOPE_EXIT(
    {
      XII_GAL_D3D12_RELEASE(pD3D12CommandList5);
    });

  D3D12_SHADING_RATE_COMBINER d3d12Combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] = {};
  d3d12Combiners[0]                                                                    = xiiD3D12TypeConversions::GetShadingRateCombiner(primitiveCombinerFlags);
  d3d12Combiners[1]                                                                    = xiiD3D12TypeConversions::GetShadingRateCombiner(textureCombinerFlags);

  pD3D12CommandList5->RSSetShadingRate(xiiD3D12TypeConversions::GetShadingRate(baseRateFlags), d3d12Combiners);

  ID3D12Resource* pD3D12ShadingRateImage = nullptr;

  xiiGALRenderPassD3D12*  pRenderPassD3D12  = xiiDynamicCast<xiiGALRenderPassD3D12*>(m_pRenderPass);
  xiiGALFramebufferD3D12* pFramebufferD3D12 = xiiDynamicCast<xiiGALFramebufferD3D12*>(m_pFramebuffer);
  if (pRenderPassD3D12 != nullptr && pFramebufferD3D12 != nullptr)
  {
    const xiiGALRenderPassCreationDescription&  renderPassDescription  = pRenderPassD3D12->GetDescription();
    const xiiGALFramebufferCreationDescription& framebufferDescription = pFramebufferD3D12->GetDescription();
    const xiiUInt32                             uiSubpassIndex         = m_CommandListData.m_uiSubpassIndex;

    if (uiSubpassIndex < renderPassDescription.m_SubPasses.GetCount())
    {
      const xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses[uiSubpassIndex];
      if (!subpass.m_ShadingRateAttachment.IsEmpty())
      {
        const xiiGALAttachmentReferenceDescription& shadingAttachmentReference = subpass.m_ShadingRateAttachment[0].m_AttachmentReference;
        if (shadingAttachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && shadingAttachmentReference.m_uiAttachmentIndex < framebufferDescription.m_Attachments.GetCount())
        {
          xiiSharedPtr<xiiGALTextureViewD3D12> pShadingRateViewD3D12 = framebufferDescription.m_Attachments[shadingAttachmentReference.m_uiAttachmentIndex].Downcast<xiiGALTextureViewD3D12>();
          if (pShadingRateViewD3D12 != nullptr)
          {
            xiiSharedPtr<xiiGALTextureD3D12> pShadingRateTextureD3D12 = pShadingRateViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
            if (pShadingRateTextureD3D12 != nullptr && pShadingRateTextureD3D12->GetD3D12Texture() != nullptr)
            {
              if (TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pShadingRateTextureD3D12.Borrow(), pShadingRateTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::ShadingRate, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::ShadingRate), "subpass shading-rate attachment", GetDebugName()))
              {
                pD3D12ShadingRateImage = pShadingRateTextureD3D12->GetD3D12Texture();
              }
            }
          }
        }
      }
    }
  }

  pD3D12CommandList5->RSSetShadingRateImage(pD3D12ShadingRateImage);

  m_CommandListState.m_bIsShadingRateSet = true;
  m_CommandListFlags.Add(CommandListFlags::ShadingRateSet);
}

void xiiGALCommandListD3D12::EnqueueSignalPlatform(xiiGALFence* pFence, xiiUInt64 uiValue)
{
  xiiGALFenceD3D12* pFenceD3D12 = xiiDynamicCast<xiiGALFenceD3D12*>(pFence);
  FenceInfo         fenceInfo   = {.m_pFenceD3D12 = pFenceD3D12, .m_uiWaitValue = uiValue};

  m_SignalFences.PushBack(fenceInfo);
}

void xiiGALCommandListD3D12::DeviceWaitForFencePlatform(xiiGALFence* pFence, xiiUInt64 uiValue)
{
  xiiGALFenceD3D12* pFenceD3D12 = xiiDynamicCast<xiiGALFenceD3D12*>(pFence);
  FenceInfo         fenceInfo   = {.m_pFenceD3D12 = pFenceD3D12, .m_uiWaitValue = uiValue};

  m_WaitFences.PushBack(fenceInfo);
}

void xiiGALCommandListD3D12::BeginDebugGroupPlatform(xiiStringView sName, const xiiColor& color)
{
#if BUILDSYSTEM_ENABLE_PIX_EVENT_RUNTIME_SUPPORT
  xiiStringBuilder tmp;
  PIXBeginEvent(m_pD3D12CommandList, xiiColorToPixColor(color), sName.GetData(tmp));
#else
  XII_IGNORE_UNUSED(sName);
  XII_IGNORE_UNUSED(color);
#endif
}

void xiiGALCommandListD3D12::EndDebugGroupPlatform()
{
#if BUILDSYSTEM_ENABLE_PIX_EVENT_RUNTIME_SUPPORT
  PIXEndEvent(m_pD3D12CommandList);
#endif
}

void xiiGALCommandListD3D12::InsertDebugLabelPlatform(xiiStringView sName, const xiiColor& color)
{
#if BUILDSYSTEM_ENABLE_PIX_EVENT_RUNTIME_SUPPORT
  xiiStringBuilder tmp;
  PIXSetMarker(m_pD3D12CommandList, xiiColorToPixColor(color), sName.GetData(tmp));
#else
  XII_IGNORE_UNUSED(sName);
  XII_IGNORE_UNUSED(color);
#endif
}

void xiiGALCommandListD3D12::InvalidateStatePlatform()
{
  m_CommandListFlags = {};
  m_CommandListState = {};
  m_CommandListData.Invalidate();

  m_SignalFences.Clear();
  m_WaitFences.Clear();
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

void xiiGALCommandListD3D12::BindSubpassAttachments(xiiGALRenderPassD3D12* pRenderPassD3D12, xiiGALFramebufferD3D12* pFramebufferD3D12, xiiUInt32 uiSubpassIndex, xiiArrayPtr<const xiiGALOptimizedClearValue> pOptimizedClearValues)
{
  XII_IGNORE_UNUSED(pOptimizedClearValues);

  if (m_pD3D12CommandList == nullptr || pRenderPassD3D12 == nullptr || pFramebufferD3D12 == nullptr)
    return;

  const xiiGALRenderPassCreationDescription&  renderPassDescription  = pRenderPassD3D12->GetDescription();
  const xiiGALFramebufferCreationDescription& framebufferDescription = pFramebufferD3D12->GetDescription();
  if (uiSubpassIndex >= renderPassDescription.m_SubPasses.GetCount())
  {
    xiiLog::Error("Failed to bind D3D12 subpass attachments on command list '{}': subpass index {} exceeds subpass count {}.", GetDebugName(), uiSubpassIndex, renderPassDescription.m_SubPasses.GetCount());
    return;
  }

  const xiiGALSubPassDescription& subpass = renderPassDescription.m_SubPasses[uiSubpassIndex];

  if (!subpass.m_ResolveAttachments.IsEmpty())
  {
    xiiLog::Warning("D3D12 subpass {} on command list '{}' uses color resolve attachments, but automatic subpass resolves are not implemented yet.", uiSubpassIndex, GetDebugName());
  }
  if (!subpass.m_DepthResolveAttachment.IsEmpty())
  {
    xiiLog::Warning("D3D12 subpass {} on command list '{}' uses depth resolve attachments, but automatic subpass depth resolves are not implemented yet.", uiSubpassIndex, GetDebugName());
  }

  m_CommandListData.m_pBoundRenderTargets.Clear();
  m_CommandListData.m_pBoundDepthStencilTarget = nullptr;
  m_CommandListData.m_uiBoundRenderTargetCount = 0U;

  xiiTemporaryHybridArray<D3D12_CPU_DESCRIPTOR_HANDLE, 2U> d3d12RenderTargetHandles;
  d3d12RenderTargetHandles.Reserve(subpass.m_RenderTargetAttachments.GetCount());

  bool bHasRenderTargetAttachmentGap = false;
  for (xiiUInt32 uiColorAttachmentIndex = 0U; uiColorAttachmentIndex < subpass.m_RenderTargetAttachments.GetCount(); ++uiColorAttachmentIndex)
  {
    const xiiGALAttachmentReferenceDescription& attachmentReference = subpass.m_RenderTargetAttachments[uiColorAttachmentIndex];
    if (attachmentReference.m_uiAttachmentIndex == XII_GAL_ATTACHMENT_UNUSED)
    {
      if (!d3d12RenderTargetHandles.IsEmpty())
      {
        bHasRenderTargetAttachmentGap = true;
      }
      continue;
    }

    if (attachmentReference.m_uiAttachmentIndex >= framebufferDescription.m_Attachments.GetCount())
    {
      xiiLog::Error("Failed to bind D3D12 render target attachment {} on command list '{}': attachment index {} is out of framebuffer attachment bounds ({}).", uiColorAttachmentIndex, GetDebugName(), attachmentReference.m_uiAttachmentIndex, framebufferDescription.m_Attachments.GetCount());
      continue;
    }

    xiiSharedPtr<xiiGALTextureViewD3D12> pRenderTargetViewD3D12 = framebufferDescription.m_Attachments[attachmentReference.m_uiAttachmentIndex].Downcast<xiiGALTextureViewD3D12>();
    if (pRenderTargetViewD3D12 == nullptr || pRenderTargetViewD3D12->GetCPUDescriptorHandle().ptr == 0U)
    {
      xiiLog::Error("Failed to bind D3D12 render target attachment {} on command list '{}': attachment {} is not a valid D3D12 render-target view.", uiColorAttachmentIndex, GetDebugName(), attachmentReference.m_uiAttachmentIndex);
      continue;
    }

    xiiSharedPtr<xiiGALTextureD3D12> pRenderTargetTextureD3D12 = pRenderTargetViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
    if (pRenderTargetTextureD3D12 == nullptr || pRenderTargetTextureD3D12->GetD3D12Texture() == nullptr)
    {
      xiiLog::Error("Failed to bind D3D12 render target attachment {} on command list '{}': backing texture is invalid.", uiColorAttachmentIndex, GetDebugName());
      continue;
    }

    xiiBitflags<xiiGALResourceStateFlags> renderTargetState = attachmentReference.m_ResourceStateFlags;
    if (renderTargetState == xiiGALResourceStateFlags::Unknown || renderTargetState == xiiGALResourceStateFlags::Undefined)
    {
      renderTargetState = xiiGALResourceStateFlags::RenderTarget;
    }

    if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pRenderTargetTextureD3D12.Borrow(), pRenderTargetTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, renderTargetState, xiiD3D12TypeConversions::GetResourceState(renderTargetState), "subpass render-target attachment", GetDebugName()))
      continue;

    d3d12RenderTargetHandles.PushBack(pRenderTargetViewD3D12->GetCPUDescriptorHandle());
    m_CommandListData.m_pBoundRenderTargets.PushBack(pRenderTargetViewD3D12);

    const xiiGALRenderPassAttachmentDescription& attachmentDescription = renderPassDescription.m_Attachments[attachmentReference.m_uiAttachmentIndex];
    if (attachmentDescription.m_LoadOperation == xiiGALAttachmentLoadOperation::Clear && !WasAttachmentUsedInPreviousSubpass(renderPassDescription, attachmentReference.m_uiAttachmentIndex, uiSubpassIndex))
    {
      xiiColor clearColor = xiiColor::Black;
      if (attachmentReference.m_uiAttachmentIndex < m_CommandListData.m_AttachmentClearValues.GetCount())
      {
        const D3D12_CLEAR_VALUE& clearValue = m_CommandListData.m_AttachmentClearValues[attachmentReference.m_uiAttachmentIndex];
        clearColor                          = xiiColor(clearValue.Color[0], clearValue.Color[1], clearValue.Color[2], clearValue.Color[3]);
      }

      const float clearColorRGBA[4] = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
      m_pD3D12CommandList->ClearRenderTargetView(pRenderTargetViewD3D12->GetCPUDescriptorHandle(), clearColorRGBA, 0U, nullptr);
    }
  }

  if (bHasRenderTargetAttachmentGap)
  {
    xiiLog::Warning("D3D12 subpass {} on command list '{}' uses non-contiguous render-target attachment slots. Slots are compacted for OM binding.", uiSubpassIndex, GetDebugName());
  }

  D3D12_CPU_DESCRIPTOR_HANDLE                 d3d12DepthStencilHandle = {};
  xiiSharedPtr<xiiGALTextureViewD3D12>        pDepthStencilViewD3D12;
  xiiSharedPtr<xiiGALTextureD3D12>            pDepthStencilTextureD3D12;
  xiiBitflags<xiiGALResourceStateFlags>       depthStencilState         = xiiGALResourceStateFlags::DepthWrite;
  const xiiGALAttachmentReferenceDescription* pDepthAttachmentReference = nullptr;

  if (!subpass.m_DepthStencilAttachment.IsEmpty())
  {
    const xiiGALAttachmentReferenceDescription& depthAttachmentReference = subpass.m_DepthStencilAttachment[0];
    if (depthAttachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED)
    {
      pDepthAttachmentReference = &depthAttachmentReference;

      if (depthAttachmentReference.m_uiAttachmentIndex >= framebufferDescription.m_Attachments.GetCount())
      {
        xiiLog::Error("Failed to bind D3D12 depth-stencil attachment on command list '{}': attachment index {} is out of framebuffer attachment bounds ({}).", GetDebugName(), depthAttachmentReference.m_uiAttachmentIndex, framebufferDescription.m_Attachments.GetCount());
      }
      else
      {
        pDepthStencilViewD3D12 = framebufferDescription.m_Attachments[depthAttachmentReference.m_uiAttachmentIndex].Downcast<xiiGALTextureViewD3D12>();
        if (pDepthStencilViewD3D12 == nullptr || pDepthStencilViewD3D12->GetCPUDescriptorHandle().ptr == 0U)
        {
          xiiLog::Error("Failed to bind D3D12 depth-stencil attachment on command list '{}': attachment {} is not a valid D3D12 depth-stencil view.", GetDebugName(), depthAttachmentReference.m_uiAttachmentIndex);
        }
        else
        {
          pDepthStencilTextureD3D12 = pDepthStencilViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
          if (pDepthStencilTextureD3D12 == nullptr || pDepthStencilTextureD3D12->GetD3D12Texture() == nullptr)
          {
            xiiLog::Error("Failed to bind D3D12 depth-stencil attachment on command list '{}': backing texture is invalid.", GetDebugName());
          }
          else
          {
            depthStencilState = depthAttachmentReference.m_ResourceStateFlags;
            if (depthStencilState == xiiGALResourceStateFlags::Unknown || depthStencilState == xiiGALResourceStateFlags::Undefined)
            {
              depthStencilState = xiiGALResourceStateFlags::DepthWrite;
            }

            if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDepthStencilTextureD3D12.Borrow(), pDepthStencilTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, depthStencilState, xiiD3D12TypeConversions::GetResourceState(depthStencilState), "subpass depth-stencil attachment", GetDebugName()))
            {
              pDepthStencilTextureD3D12 = nullptr;
            }
            else
            {
              d3d12DepthStencilHandle = pDepthStencilViewD3D12->GetCPUDescriptorHandle();
            }
          }
        }
      }
    }
  }

  m_CommandListData.m_uiBoundRenderTargetCount = d3d12RenderTargetHandles.GetCount();
  m_CommandListData.m_pBoundDepthStencilTarget = pDepthStencilViewD3D12;

  m_pD3D12CommandList->OMSetRenderTargets(m_CommandListData.m_uiBoundRenderTargetCount, m_CommandListData.m_uiBoundRenderTargetCount > 0U ? d3d12RenderTargetHandles.GetData() : nullptr, FALSE, pDepthStencilViewD3D12 != nullptr ? &d3d12DepthStencilHandle : nullptr);

  if (pDepthAttachmentReference != nullptr && pDepthStencilViewD3D12 != nullptr && pDepthStencilTextureD3D12 != nullptr)
  {
    const xiiUInt32 uiDepthAttachmentIndex = pDepthAttachmentReference->m_uiAttachmentIndex;
    if (uiDepthAttachmentIndex < renderPassDescription.m_Attachments.GetCount())
    {
      const bool bFirstDepthUse = !WasAttachmentUsedInPreviousSubpass(renderPassDescription, uiDepthAttachmentIndex, uiSubpassIndex);

      if (bFirstDepthUse)
      {
        const xiiGALRenderPassAttachmentDescription& depthAttachmentDescription = renderPassDescription.m_Attachments[uiDepthAttachmentIndex];
        const xiiGALResourceFormatDescription&       formatProperties           = xiiGALTextureUtilities::GetResourceFormatProperties(depthAttachmentDescription.m_Format);

        D3D12_CLEAR_FLAGS d3d12ClearFlags = static_cast<D3D12_CLEAR_FLAGS>(0U);
        if (depthAttachmentDescription.m_LoadOperation == xiiGALAttachmentLoadOperation::Clear)
          d3d12ClearFlags |= D3D12_CLEAR_FLAG_DEPTH;

        if (HasStencilComponent(depthAttachmentDescription.m_Format) && depthAttachmentDescription.m_StencilLoadOperation == xiiGALAttachmentLoadOperation::Clear && formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil)
          d3d12ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;

        if (d3d12ClearFlags != static_cast<D3D12_CLEAR_FLAGS>(0U))
        {
          bool bReadyForClear = true;
          if (depthStencilState != xiiGALResourceStateFlags::DepthWrite)
          {
            if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDepthStencilTextureD3D12.Borrow(), pDepthStencilTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::DepthWrite, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::DepthWrite), "subpass depth-stencil clear", GetDebugName()))
            {
              bReadyForClear = false;
            }
          }

          if (bReadyForClear)
          {
            float    fDepthClearValue    = 1.0f;
            xiiUInt8 uiStencilClearValue = 0U;
            if (uiDepthAttachmentIndex < m_CommandListData.m_AttachmentClearValues.GetCount())
            {
              const D3D12_CLEAR_VALUE& clearValue = m_CommandListData.m_AttachmentClearValues[uiDepthAttachmentIndex];
              fDepthClearValue                    = clearValue.DepthStencil.Depth;
              uiStencilClearValue                 = static_cast<xiiUInt8>(clearValue.DepthStencil.Stencil);
            }

            m_pD3D12CommandList->ClearDepthStencilView(d3d12DepthStencilHandle, d3d12ClearFlags, fDepthClearValue, uiStencilClearValue, 0U, nullptr);

            if (depthStencilState != xiiGALResourceStateFlags::DepthWrite)
            {
              if (!TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pDepthStencilTextureD3D12.Borrow(), pDepthStencilTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, depthStencilState, xiiD3D12TypeConversions::GetResourceState(depthStencilState), "subpass depth-stencil restore", GetDebugName()))
              {
                xiiLog::Warning("Failed to restore depth-stencil state after clear in subpass {} on command list '{}'.", uiSubpassIndex, GetDebugName());
              }
            }
          }
        }
      }
    }
  }

  if (m_CommandListState.m_bIsShadingRateSet)
  {
    ID3D12GraphicsCommandList5* pD3D12CommandList5 = nullptr;
    if (SUCCEEDED(m_pD3D12CommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList5), reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList5**>(&pD3D12CommandList5)))) && pD3D12CommandList5 != nullptr)
    {
      XII_SCOPE_EXIT(
        {
          XII_GAL_D3D12_RELEASE(pD3D12CommandList5);
        });

      ID3D12Resource* pShadingRateResource = nullptr;
      if (!subpass.m_ShadingRateAttachment.IsEmpty())
      {
        const xiiGALAttachmentReferenceDescription& shadingAttachmentReference = subpass.m_ShadingRateAttachment[0].m_AttachmentReference;
        if (shadingAttachmentReference.m_uiAttachmentIndex != XII_GAL_ATTACHMENT_UNUSED && shadingAttachmentReference.m_uiAttachmentIndex < framebufferDescription.m_Attachments.GetCount())
        {
          xiiSharedPtr<xiiGALTextureViewD3D12> pShadingRateViewD3D12 = framebufferDescription.m_Attachments[shadingAttachmentReference.m_uiAttachmentIndex].Downcast<xiiGALTextureViewD3D12>();
          if (pShadingRateViewD3D12 != nullptr)
          {
            xiiSharedPtr<xiiGALTextureD3D12> pShadingRateTextureD3D12 = pShadingRateViewD3D12->GetTexture().Downcast<xiiGALTextureD3D12>();
            if (pShadingRateTextureD3D12 != nullptr && pShadingRateTextureD3D12->GetD3D12Texture() != nullptr)
            {
              if (TransitionOrVerifyResourceStateForRayTracing(m_pD3D12CommandList, pShadingRateTextureD3D12.Borrow(), pShadingRateTextureD3D12->GetD3D12Texture(), xiiGALStateTransitionMode::Transition, xiiGALResourceStateFlags::ShadingRate, xiiD3D12TypeConversions::GetResourceState(xiiGALResourceStateFlags::ShadingRate), "subpass shading-rate attachment", GetDebugName()))
              {
                pShadingRateResource = pShadingRateTextureD3D12->GetD3D12Texture();
              }
            }
          }
        }
      }

      pD3D12CommandList5->RSSetShadingRateImage(pShadingRateResource);
    }
  }
}

void xiiGALCommandListD3D12::PrepareForDraw()
{
  XII_ASSERT_DEBUG(m_pD3D12CommandList != nullptr, "Invalid command list.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  for (xiiUInt32 uiSlot = 0U; uiSlot < m_VertexStreams.GetCount(); ++uiSlot)
  {
    if (xiiGALBuffer* pBuffer = m_VertexStreams[uiSlot].m_pBuffer)
    {
      VerifyBufferState(pBuffer, xiiGALResourceStateFlags::VertexBuffer, "Using vertex buffers");
    }
  }
#endif

  xiiTemporaryHybridArray<D3D12_VERTEX_BUFFER_VIEW, 2U> d3d12VertexBufferViews;
  d3d12VertexBufferViews.SetCountUninitialized(m_VertexStreams.GetCount());

  for (xiiUInt32 uiSlot = 0U; uiSlot < m_VertexStreams.GetCount(); ++uiSlot)
  {
    const VertexStreamDescription& vertexStream = m_VertexStreams[uiSlot];

    if (xiiGALBufferD3D12* pBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(vertexStream.m_pBuffer))
    {
      XII_ASSERT_DEV(vertexStream.m_uiOffset < pBufferD3D12->GetSize(), "Vertex buffer offset {} exceeds buffer size {}.", vertexStream.m_uiOffset, pBufferD3D12->GetSize());

      d3d12VertexBufferViews[uiSlot].BufferLocation = pBufferD3D12->GetD3D12BufferGPUVirtualAddress() + vertexStream.m_uiOffset;
      d3d12VertexBufferViews[uiSlot].SizeInBytes    = static_cast<xiiUInt32>(pBufferD3D12->GetSize() - vertexStream.m_uiOffset);
      d3d12VertexBufferViews[uiSlot].StrideInBytes  = pBufferD3D12->GetDescription().m_uiElementByteStride;
    }
    else
    {
      d3d12VertexBufferViews[uiSlot].BufferLocation = 0ULL;
      d3d12VertexBufferViews[uiSlot].SizeInBytes    = 0U;
      d3d12VertexBufferViews[uiSlot].StrideInBytes  = 0U;
    }
  }

  m_pD3D12CommandList->IASetVertexBuffers(0U, d3d12VertexBufferViews.GetCount(), d3d12VertexBufferViews.GetData());
}

void xiiGALCommandListD3D12::PrepareForIndexedDraw(xiiEnum<xiiGALValueType> indexType)
{
  PrepareForDraw();

  if (m_pD3D12CommandList == nullptr || m_pIndexBuffer == nullptr)
    return;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  VerifyBufferState(m_pIndexBuffer, xiiGALResourceStateFlags::IndexBuffer, "Indexed draw call");
#endif

  xiiGALBufferD3D12* pIndexBufferD3D12 = xiiDynamicCast<xiiGALBufferD3D12*>(m_pIndexBuffer);
  if (pIndexBufferD3D12 == nullptr || pIndexBufferD3D12->GetD3D12Buffer() == nullptr)
    return;

  DXGI_FORMAT d3d12IndexFormat = DXGI_FORMAT_UNKNOWN;
  if (indexType == xiiGALValueType::UInt16)
  {
    d3d12IndexFormat = DXGI_FORMAT_R16_UINT;
  }
  else if (indexType == xiiGALValueType::UInt32)
  {
    d3d12IndexFormat = DXGI_FORMAT_R32_UINT;
  }
  else
  {
    xiiLog::Error("Failed to prepare indexed draw on D3D12 command list '{}': unsupported index type '{}'.", GetDebugName(), xiiArgEnum(indexType));
    return;
  }

  const xiiUInt64 uiIndexBufferSize = pIndexBufferD3D12->GetSize();
  if (m_uiIndexDataOffset >= uiIndexBufferSize)
  {
    xiiLog::Error("Failed to prepare indexed draw on D3D12 command list '{}': index-buffer offset {} exceeds buffer size {}.", GetDebugName(), m_uiIndexDataOffset, uiIndexBufferSize);
    return;
  }

  D3D12_INDEX_BUFFER_VIEW d3d12IndexBufferView = {};
  d3d12IndexBufferView.BufferLocation          = pIndexBufferD3D12->GetD3D12BufferGPUVirtualAddress() + m_uiIndexDataOffset;
  d3d12IndexBufferView.SizeInBytes             = static_cast<UINT>(xiiMath::Min<xiiUInt64>(uiIndexBufferSize - m_uiIndexDataOffset, static_cast<xiiUInt64>(0xFFFFFFFFULL)));
  d3d12IndexBufferView.Format                  = d3d12IndexFormat;

  m_pD3D12CommandList->IASetIndexBuffer(&d3d12IndexBufferView);
}

void xiiGALCommandListD3D12::PrepareForDispatchCompute()
{
  XII_VERIFY(CommitShaderResourcesPlatform(xiiGALStateTransitionMode::Transition).Succeeded(), "Failed to commit shader resources for compute dispatch.");
}

void xiiGALCommandListD3D12::PrepareForRayTracing()
{
  XII_VERIFY(CommitShaderResourcesPlatform(xiiGALStateTransitionMode::Transition).Succeeded(), "Failed to commit shader resources for ray tracing dispatch.");
}

[[nodiscard]] inline bool ResourceStateHasWriteAccess(xiiBitflags<xiiGALResourceStateFlags> flags)
{
  xiiBitflags<xiiGALResourceStateFlags> writeAccessStates = xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::CopyDestination | xiiGALResourceStateFlags::ResolveDestination | xiiGALResourceStateFlags::BuildASWrite;

  return writeAccessStates.IsAnySet(flags);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandListD3D12);
