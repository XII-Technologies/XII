#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/InstanceTransformPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInstanceTransformPass, 1, xiiRTTIDefaultAllocator<xiiInstanceTransformPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("InstanceTransformPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiInstanceTransformPass::xiiInstanceTransformPass() : xiiRenderPipelinePass("InstanceTransformPass") {}
xiiInstanceTransformPass::~xiiInstanceTransformPass() = default;

namespace { struct InstanceTransformPassData { xiiRGBufferHandle hSceneTransforms; xiiRGBufferHandle hWorldMatrices; xiiRGBufferHandle hBounds; xiiUInt32 uiInstanceCount = 0u; }; }

void xiiInstanceTransformPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!m_pWorldMatrixBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 12u * k_uiMaxInstances; // float4x3 per instance
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pWorldMatrixBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pBoundsBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 8u * k_uiMaxInstances; // center(float4) + extents(float4)
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pBoundsBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pSceneTransformBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 12u * k_uiMaxInstances;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pSceneTransformBuffer = pDevice->CreateBuffer(desc);
  }

  auto [pData, hPass] = graph.AddPass<InstanceTransformPassData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [this](InstanceTransformPassData& data, xiiRGBuilder& builder)
    {
      data.hSceneTransforms = builder.ImportBuffer("SceneTransforms", m_pSceneTransformBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hSceneTransforms = builder.ReadBuffer(data.hSceneTransforms, xiiGALResourceStateFlags::ShaderResource);
      data.hWorldMatrices   = builder.ImportBuffer("WorldMatrices", m_pWorldMatrixBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hWorldMatrices   = builder.WriteBuffer(data.hWorldMatrices, xiiGALResourceStateFlags::UnorderedAccess);
      data.hBounds          = builder.ImportBuffer("InstanceBoundsRW", m_pBoundsBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hBounds          = builder.WriteBuffer(data.hBounds, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount  = k_uiMaxInstances;
    },
    [](const InstanceTransformPassData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Instance Transform + Bounds");
      // InstanceUpdate.xiiShader: [numthreads(64,1,1)]
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceWorldMatrixBuffer), pData->hWorldMatrices);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer),      pData->hBounds);
}
