#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/HiZOcclusionCullPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHiZOcclusionCullPass, 1, xiiRTTIDefaultAllocator<xiiHiZOcclusionCullPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("HiZOcclusionCullPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiHiZOcclusionCullPass::xiiHiZOcclusionCullPass() : xiiRenderPipelinePass("HiZOcclusionCullPass") {}
xiiHiZOcclusionCullPass::~xiiHiZOcclusionCullPass() = default;

namespace { struct HiZOcclData { xiiRGBufferHandle hCandidates; xiiRGBufferHandle hBounds; xiiRGTextureHandle hHiZ; xiiRGBufferHandle hSurviving; xiiUInt32 uiInstanceCount = 0u; }; }

void xiiHiZOcclusionCullPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!m_pSurvivingInstanceBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiUInt32) * k_uiMaxInstances;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pSurvivingInstanceBuffer = pDevice->CreateBuffer(desc);
  }

  xiiRGBufferHandle hCandidates, hBounds;
  xiiRGTextureHandle hHiZ;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), hCandidates);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer),   hBounds);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid),             hHiZ);

  auto [pData, hPass] = graph.AddPass<HiZOcclData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [this, hCandidates, hBounds, hHiZ](HiZOcclData& data, xiiRGBuilder& builder)
    {
      if (hCandidates.IsValid()) data.hCandidates = builder.ReadBuffer(hCandidates, xiiGALResourceStateFlags::ShaderResource);
      if (hBounds.IsValid())     data.hBounds     = builder.ReadBuffer(hBounds,     xiiGALResourceStateFlags::ShaderResource);
      if (hHiZ.IsValid())        data.hHiZ        = builder.ReadTexture(hHiZ,       xiiGALResourceStateFlags::ShaderResource);
      data.hSurviving     = builder.ImportBuffer("SurvivingInstances", m_pSurvivingInstanceBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hSurviving     = builder.WriteBuffer(data.hSurviving, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount = k_uiMaxInstances;
    },
    [](const HiZOcclData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Hi-Z Occlusion Cull");
      // HiZOcclusionCulling.xiiShader: projects AABB to screen, samples Hi-Z pyramid.
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer), pData->hSurviving);
}
