#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/DrawCommandBuildPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDrawCommandBuildPass, 1, xiiRTTIDefaultAllocator<xiiDrawCommandBuildPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("DrawCommandBuildPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDrawCommandBuildPass::xiiDrawCommandBuildPass() : xiiRenderPipelinePass("DrawCommandBuildPass") {}
xiiDrawCommandBuildPass::~xiiDrawCommandBuildPass() = default;

namespace { struct DrawCmdBuildData { xiiRGBufferHandle hSurviving; xiiRGBufferHandle hLOD; xiiRGBufferHandle hDrawArgs; xiiRGBufferHandle hDrawCount; xiiUInt32 uiInstanceCount = 0u; }; }

void xiiDrawCommandBuildPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // DrawIndexedIndirect struct: (indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation)
  if (!m_pDrawIndirectArgsBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiUInt32) * 5u * m_uiMaxDrawCommands;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::IndirectDrawArgs | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pDrawIndirectArgsBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pDrawCountBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiUInt32) * m_uiMaxMaterialBins;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pDrawCountBuffer = pDevice->CreateBuffer(desc);
  }

  xiiRGBufferHandle hSurviving, hLOD;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer), hSurviving);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceLODBuffer),       hLOD);

  auto [pData, hPass] = graph.AddPass<DrawCmdBuildData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [this, hSurviving, hLOD](DrawCmdBuildData& data, xiiRGBuilder& builder)
    {
      if (hSurviving.IsValid()) data.hSurviving = builder.ReadBuffer(hSurviving, xiiGALResourceStateFlags::ShaderResource);
      if (hLOD.IsValid())       data.hLOD       = builder.ReadBuffer(hLOD,       xiiGALResourceStateFlags::ShaderResource);
      data.hDrawArgs  = builder.ImportBuffer("DrawIndirectArgs",  m_pDrawIndirectArgsBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hDrawArgs  = builder.WriteBuffer(data.hDrawArgs, xiiGALResourceStateFlags::UnorderedAccess);
      data.hDrawCount = builder.ImportBuffer("DrawCounts",        m_pDrawCountBuffer,        xiiGALResourceStateFlags::UnorderedAccess);
      data.hDrawCount = builder.WriteBuffer(data.hDrawCount, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount = 65536u;
    },
    [](const DrawCmdBuildData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Draw Command Build + Compact");
      // DrawCommandBuild.xiiShader: bins surviving instances by material, writes DrawIndexedIndirect args.
      // Uses prefix sum for compact packing — no CPU readback needed.
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), pData->hDrawArgs);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawCountBuffer),      pData->hDrawCount);
}
