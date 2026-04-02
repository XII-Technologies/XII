#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/CoarseFrustumCullPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCoarseFrustumCullPass, 1, xiiRTTIDefaultAllocator<xiiCoarseFrustumCullPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("CoarseFrustumCullPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace { struct xiiCoarseFrustumAutoReg { xiiCoarseFrustumAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiCoarseFrustumCullPass)); } }; static xiiCoarseFrustumAutoReg s_AutoReg; }

xiiCoarseFrustumCullPass::xiiCoarseFrustumCullPass() : xiiRenderPipelinePass("CoarseFrustumCullPass") {}
xiiCoarseFrustumCullPass::~xiiCoarseFrustumCullPass() = default;

namespace { struct FrustumCullPassData { xiiRGBufferHandle hBounds; xiiRGBufferHandle hLOD; xiiRGBufferHandle hVisible; xiiUInt32 uiInstanceCount = 0u; }; }

void xiiCoarseFrustumCullPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!m_pVisibleCandidateBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiUInt32) * k_uiMaxInstances;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pVisibleCandidateBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pFrustumPlanesBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 4u * 6u; // 6 planes x float4
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pFrustumPlanesBuffer = pDevice->CreateBuffer(desc);
  }

  // Extract frustum planes from the view's culling camera.
  xiiFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  xiiVec4 planes[6];
  for (xiiUInt32 i = 0; i < 6u; ++i)
  {
    const xiiPlane& p = frustum.GetPlane(i);
    planes[i] = xiiVec4(p.m_vNormal, p.m_fNegDistance);
  }

  xiiRGBufferHandle hBounds, hLOD;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer), hBounds);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceLODBuffer),    hLOD);

  auto [pData, hPass] = graph.AddPass<FrustumCullPassData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [this, hBounds, hLOD](FrustumCullPassData& data, xiiRGBuilder& builder)
    {
      if (hBounds.IsValid()) data.hBounds  = builder.ReadBuffer(hBounds, xiiGALResourceStateFlags::ShaderResource);
      if (hLOD.IsValid())    data.hLOD     = builder.ReadBuffer(hLOD,    xiiGALResourceStateFlags::ShaderResource);
      data.hVisible          = builder.ImportBuffer("VisibleCandidates", m_pVisibleCandidateBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hVisible          = builder.WriteBuffer(data.hVisible, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount   = k_uiMaxInstances;
    },
    [this, planes](const FrustumCullPassData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Coarse Frustum Cull");
      cmd.UpdateBuffer(m_pFrustumPlanesBuffer.Borrow(), 0u, planes, sizeof(planes));
      // CoarseFrustumCulling.xiiShader: [numthreads(64,1,1)]
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), pData->hVisible);
}
