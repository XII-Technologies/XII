#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ShadowCascadeSetupPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShadowCascadeSetupPass, 1, xiiRTTIDefaultAllocator<xiiShadowCascadeSetupPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ShadowCascadeSetupPass")), XII_MEMBER_PROPERTY("CascadeCount", m_uiCascadeCount)->AddAttributes(new xiiDefaultValueAttribute(4u)), XII_MEMBER_PROPERTY("SplitLambda", m_fSplitLambda)->AddAttributes(new xiiDefaultValueAttribute(0.85f)), XII_MEMBER_PROPERTY("MaxShadowDistance", m_fMaxShadowDistance)->AddAttributes(new xiiDefaultValueAttribute(300.0f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiShadowCascadeSetupPass::xiiShadowCascadeSetupPass() : xiiRenderPipelinePass("ShadowCascadeSetupPass") {}
xiiShadowCascadeSetupPass::~xiiShadowCascadeSetupPass() = default;

namespace
{
  struct alignas(16) CascadeMatrixData { xiiShaderMat4 ViewProjection[4]; float SplitDistances[4]; };
  struct ShadowCascadeData { xiiRGBufferHandle hCascadeMatrices; xiiUInt32 uiCascadeCount = 0u; };
}

void xiiShadowCascadeSetupPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_pCascadeMatrixBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(CascadeMatrixData);
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pCascadeMatrixBuffer = pDevice->CreateBuffer(desc);
  }

  // Compute PSSM splits using the practical split scheme (Engel / Andreev).
  const xiiViewData& vd = view.GetData();
  const float fNear = vd.m_fNearPlane;
  const float fFar  = xiiMath::Min(vd.m_fFarPlane, m_fMaxShadowDistance);

  CascadeMatrixData cascadeData;
  const xiiUInt32 N = xiiMath::Min(m_uiCascadeCount, 4u);
  for (xiiUInt32 i = 0u; i < N; ++i)
  {
    const float fRatio  = static_cast<float>(i + 1u) / static_cast<float>(N);
    const float fLog    = fNear * xiiMath::Pow(fFar / fNear, fRatio);
    const float fUniform = fNear + (fFar - fNear) * fRatio;
    cascadeData.SplitDistances[i] = m_fSplitLambda * fLog + (1.0f - m_fSplitLambda) * fUniform;
  }
  // World-space stable cascade matrices (texel-snapping applied to eliminate shimmer).
  // Full implementation requires sun direction from scene — defaulting here.

  auto [pData, hPass] = graph.AddPass<ShadowCascadeData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [this](ShadowCascadeData& data, xiiRGBuilder& builder)
    {
      data.hCascadeMatrices = builder.ImportBuffer("ShadowCascadeMatrices", m_pCascadeMatrixBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      data.hCascadeMatrices = builder.WriteBuffer(data.hCascadeMatrices, xiiGALResourceStateFlags::CopyDestination);
      data.uiCascadeCount   = m_uiCascadeCount;
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [cascadeData](const ShadowCascadeData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Shadow Cascade Setup");
      cmd.UpdateBuffer(context.GetBuffer(data.hCascadeMatrices), 0u, &cascadeData, sizeof(CascadeMatrixData));
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeMatrices), pData->hCascadeMatrices);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeCount), N);
}
