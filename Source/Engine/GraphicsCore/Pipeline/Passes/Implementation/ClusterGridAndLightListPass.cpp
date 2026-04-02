#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ClusterGridAndLightListPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusterGridAndLightListPass, 1, xiiRTTIDefaultAllocator<xiiClusterGridAndLightListPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ClusterGridAndLightListPass")), XII_MEMBER_PROPERTY("ClusterX", m_uiClusterCountX)->AddAttributes(new xiiDefaultValueAttribute(16u)), XII_MEMBER_PROPERTY("ClusterY", m_uiClusterCountY)->AddAttributes(new xiiDefaultValueAttribute(8u)), XII_MEMBER_PROPERTY("ClusterZ", m_uiClusterCountZ)->AddAttributes(new xiiDefaultValueAttribute(24u)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiClusterAutoReg { xiiClusterAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiClusterGridAndLightListPass)); } }; static xiiClusterAutoReg s_AutoReg; }

xiiClusterGridAndLightListPass::xiiClusterGridAndLightListPass() : xiiRenderPipelinePass("ClusterGridAndLightListPass") {}
xiiClusterGridAndLightListPass::~xiiClusterGridAndLightListPass() = default;

namespace { struct ClusterData { xiiRGTextureHandle hDepth; xiiRGBufferHandle hClusterDescs; xiiRGBufferHandle hLightGrid; xiiRGBufferHandle hLightIndex; xiiUInt32 cX = 16u, cY = 8u, cZ = 24u; }; }

void xiiClusterGridAndLightListPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* p = xiiGALDevice::GetDefaultDevice();
  xiiRGTextureHandle hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  const xiiUInt32 totalClusters = m_uiClusterCountX * m_uiClusterCountY * m_uiClusterCountZ;
  xiiGALBufferCreationDescription descDescs;
  descDescs.m_uiSize        = sizeof(float) * 8u * totalClusters; // AABB min+max per cluster
  descDescs.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  descDescs.m_ResourceUsage = xiiGALResourceUsage::Default;

  xiiGALBufferCreationDescription gridDesc;
  gridDesc.m_uiSize        = sizeof(xiiUInt32) * 2u * totalClusters; // (offset, count) per cluster
  gridDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  gridDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  xiiGALBufferCreationDescription idxDesc;
  idxDesc.m_uiSize        = sizeof(xiiUInt32) * totalClusters * m_uiMaxLightsPerCluster;
  idxDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  idxDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  auto [pData, hPass] = graph.AddPass<ClusterData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, descDescs, gridDesc, idxDesc, this](ClusterData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      data.hClusterDescs = builder.WriteBuffer("ClusterDescriptors", descDescs, xiiGALResourceStateFlags::UnorderedAccess);
      data.hLightGrid    = builder.WriteBuffer("LightGrid",           gridDesc,  xiiGALResourceStateFlags::UnorderedAccess);
      data.hLightIndex   = builder.WriteBuffer("LightIndexBuffer",    idxDesc,   xiiGALResourceStateFlags::UnorderedAccess);
      data.cX = m_uiClusterCountX; data.cY = m_uiClusterCountY; data.cZ = m_uiClusterCountZ;
    },
    [](const ClusterData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Cluster Grid + Light List");
      // ClusterBuild.xiiShader: pass 21 — builds frustum-space cluster AABB descriptors.
      cmd.Dispatch(data.cX, data.cY, data.cZ);
      // LightAssignment.xiiShader: pass 22 — assigns active lights to overlapping clusters.
      cmd.Dispatch((data.cX * data.cY * data.cZ + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_ClusterDescriptors), pData->hClusterDescs);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightGridBuffer),    pData->hLightGrid);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightIndexBuffer),   pData->hLightIndex);
}
