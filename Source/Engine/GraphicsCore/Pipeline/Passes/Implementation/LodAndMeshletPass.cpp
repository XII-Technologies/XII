#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/LodAndMeshletPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLodAndMeshletPass, 1, xiiRTTIDefaultAllocator<xiiLodAndMeshletPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("LodAndMeshletPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiLodAndMeshletPass::xiiLodAndMeshletPass() : xiiRenderPipelinePass("LodAndMeshletPass") {}
xiiLodAndMeshletPass::~xiiLodAndMeshletPass() = default;

namespace
{
  struct alignas(16) LodConstants { float LODDistances[4]; };
  struct LodPassData { xiiRGBufferHandle hBounds; xiiRGBufferHandle hLODOutput; xiiUInt32 uiInstanceCount = 0u; };
}

void xiiLodAndMeshletPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!m_pLODMetadataBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiUInt32) * k_uiMaxInstances; // packed uint: (lod:8, bin:8, flags:16)
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pLODMetadataBuffer = pDevice->CreateBuffer(desc);
  }

  xiiRGBufferHandle hBounds;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer), hBounds);

  const LodConstants lodConsts = { { m_fLOD0Distance, m_fLOD1Distance, m_fLOD2Distance, m_fLOD3Distance } };

  auto [pData, hPass] = graph.AddPass<LodPassData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [this, hBounds](LodPassData& data, xiiRGBuilder& builder)
    {
      if (hBounds.IsValid()) data.hBounds = builder.ReadBuffer(hBounds, xiiGALResourceStateFlags::ShaderResource);
      data.hLODOutput     = builder.ImportBuffer("LODMetadata", m_pLODMetadataBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hLODOutput     = builder.WriteBuffer(data.hLODOutput, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount = k_uiMaxInstances;
    },
    [lodConsts](const LodPassData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("LOD Selection + Meshlet");
      // LodSelection.xiiShader: [numthreads(64,1,1)]
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceLODBuffer), pData->hLODOutput);
}
