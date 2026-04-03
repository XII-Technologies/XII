#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ShadowCasterCullPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShadowCasterCullPass, 1, xiiRTTIDefaultAllocator<xiiShadowCasterCullPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ShadowCasterCullPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiShadowCasterCullPass::xiiShadowCasterCullPass() : xiiRenderPipelinePass("ShadowCasterCullPass") {}
xiiShadowCasterCullPass::~xiiShadowCasterCullPass() = default;

namespace { struct ShadowCullData { xiiRGBufferHandle hCascadeMatrices; xiiRGBufferHandle hInstanceBounds; xiiRGBufferHandle hShadowCasters; xiiUInt32 uiCascadeCount = 0u; }; }

void xiiShadowCasterCullPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!m_pShadowCasterBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiUInt32) * k_uiMaxInstances * 4u; // 4 cascades * max instances
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pShadowCasterBuffer = pDevice->CreateBuffer(desc);
  }

  xiiRGBufferHandle hCascMat, hBounds;
  xiiUInt32 uiCascadeCount = 4u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeMatrices), hCascMat);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer),   hBounds);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeCount),     uiCascadeCount);

  auto [pData, hPass] = graph.AddPass<ShadowCullData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [this, hCascMat, hBounds, uiCascadeCount](ShadowCullData& data, xiiRGBuilder& builder)
    {
      if (hCascMat.IsValid()) data.hCascadeMatrices = builder.ReadBuffer(hCascMat, xiiGALResourceStateFlags::ConstantBuffer);
      if (hBounds.IsValid())  data.hInstanceBounds  = builder.ReadBuffer(hBounds,  xiiGALResourceStateFlags::ShaderResource);
      data.hShadowCasters  = builder.ImportBuffer("ShadowCasters", m_pShadowCasterBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hShadowCasters  = builder.WriteBuffer(data.hShadowCasters, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiCascadeCount  = uiCascadeCount;
    },
    [](const ShadowCullData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Shadow Caster Cull");
      // ShadowCasterCulling.xiiShader: test each instance AABB against each cascade volume.
      cmd.Dispatch((65536u + 63u) / 64u, data.uiCascadeCount, 1u);
      cmd.PopDebugGroup();
    }
  );
}
