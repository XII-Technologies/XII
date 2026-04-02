#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/AccelerationStructurePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/RayTracing/AccelerationStructure.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAccelerationStructurePass, 1, xiiRTTIDefaultAllocator<xiiAccelerationStructurePass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("AccelerationStructurePass")), XII_MEMBER_PROPERTY("AllowRefit", m_bAllowRefit)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("EnableCompaction", m_bEnableCompaction)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiASAutoReg { xiiASAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiAccelerationStructurePass)); } }; static xiiASAutoReg s_AutoReg; }

xiiAccelerationStructurePass::xiiAccelerationStructurePass() : xiiRenderPipelinePass("AccelerationStructurePass") {}
xiiAccelerationStructurePass::~xiiAccelerationStructurePass() = default;

namespace { struct ASPassData { bool bHasRT = false; }; }

void xiiAccelerationStructurePass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // Check hardware RT support via device feature query.
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  const bool bRTSupported = pDevice->GetFeatures().m_bRayTracing;

  if (!bRTSupported)
    return; // No-op — downstream RT passes will also bail out when they find no TLAS handle.

  auto [pData, hPass] = graph.AddPass<ASPassData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [](ASPassData& data, xiiRGBuilder& builder)
    {
      data.bHasRT = true;
      builder.SetPassSideEffects(true);   // BLAS/TLAS build writes to persistent GPU memory.
      builder.SetPassAllowMerge(false);   // Must not be merged — explicit barrier semantics.
    },
    [this](const ASPassData& data, xiiRGPassContext& context)
    {
      if (!data.bHasRT) return;
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Acceleration Structure Build");

      // Pass 32: BLAS scheduling — CPU-side policy (refit vs full-rebuild per mesh).
      // Pass 33: BLAS build/refit — issued as batched BuildAccelerationStructure commands.
      cmd.BuildAccelerationStructures(); // Submits all pending BLAS builds enqueued this frame.

      // Pass 34: TLAS build — single top-level instance buffer covering all BLASes.
      cmd.BuildTopLevelAccelerationStructure();

      // Pass 35: SBT update — only re-records hit groups that changed this frame.
      cmd.UpdateShaderBindingTable();

      // Pass 45: Compaction deferred query — issued here if any BLAS flagged for compaction.
      if (m_bEnableCompaction)
        cmd.CompactAccelerationStructures();

      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);
}
