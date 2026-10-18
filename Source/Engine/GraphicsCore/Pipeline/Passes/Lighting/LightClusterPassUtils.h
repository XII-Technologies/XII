/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

namespace xiiLightClusterPassUtils
{
  inline const xiiLightClusterFrameData* Layout(const xiiRenderPipelinePassContext& context, const xiiRenderPipelineDataInputPin& pin)
  {
    return xiiDynamicCast<const xiiLightClusterFrameData*>(context.GetInputData(pin).Borrow());
  }
  inline bool Buffer(const xiiRenderPipelinePassContext& context, xiiRenderGraphBufferHandle handle, xiiUInt32 stride, xiiUInt64 count, xiiBitflags<xiiGALBindFlags> flags)
  {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride && description->m_uiSize == count * stride && description->m_BindFlags.AreAllSet(flags);
  }
  inline xiiGALBufferCreationDescription Description(xiiUInt32 stride, xiiUInt32 count)
  {
    xiiGALBufferCreationDescription result;
    result.m_uiSize              = xiiUInt64(stride) * count;
    result.m_uiElementByteStride = stride;
    result.m_Mode                = xiiGALBufferMode::Structured;
    result.m_Usage               = xiiGALResourceUsage::Default;
    result.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    return result;
  }
  inline xiiResult Dispatch(xiiLightClusterConstants& snapshot, xiiUInt32 count, xiiGALDispatchComputeDescription& out_dispatch)
  {
    if (!xiiGALDevice::HasDefaultDevice() || !count)
      return XII_FAILURE;
    const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
    if (limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 64 || !limits.m_uiMaxThreadGroupCountX)
      return XII_FAILURE;
    const xiiUInt32 groups             = (count + 63U) / 64U;
    out_dispatch.m_uiThreadGroupCountX = xiiMath::Min(groups, xiiMath::Min(limits.m_uiMaxThreadGroupCountX, xiiMath::MaxValue<xiiUInt32>() / 64U));
    out_dispatch.m_uiThreadGroupCountY = (groups + out_dispatch.m_uiThreadGroupCountX - 1U) / out_dispatch.m_uiThreadGroupCountX;
    out_dispatch.m_uiThreadGroupCountZ = 1;
    snapshot.Work.z                    = out_dispatch.m_uiThreadGroupCountX * 64U;
    return out_dispatch.m_uiThreadGroupCountY <= limits.m_uiMaxThreadGroupCountY ? XII_SUCCESS : XII_FAILURE;
  }
  inline void Bind(xiiRenderGraphPassContext& execute, xiiRenderGraphBufferHandle handle, const xiiLightClusterConstants& snapshot, xiiGALComputePipelineState* pipeline)
  {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiLightClusterConstants> mapped(commands, execute.GetBuffer(handle), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = snapshot;
    }
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = execute.GetBuffer(handle);
    transition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
    commands.SetPipelineState(pipeline);
    commands.ResolveAndSetConstantBuffer("xiiLightClusterConstants", execute.GetBuffer(handle), xiiGALShaderType::Compute);
  }
} // namespace xiiLightClusterPassUtils
