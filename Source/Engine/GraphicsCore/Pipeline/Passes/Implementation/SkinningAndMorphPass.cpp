#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/SkinningAndMorphPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkinningAndMorphPass, 1, xiiRTTIDefaultAllocator<xiiSkinningAndMorphPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("SkinningAndMorphPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace { struct xiiSkinningAutoReg { xiiSkinningAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiSkinningAndMorphPass)); } }; static xiiSkinningAutoReg s_AutoReg; }

xiiSkinningAndMorphPass::xiiSkinningAndMorphPass() : xiiRenderPipelinePass("SkinningAndMorphPass") {}
xiiSkinningAndMorphPass::~xiiSkinningAndMorphPass() = default;

namespace { struct SkinPassData { xiiRGBufferHandle hSkinInput; xiiRGBufferHandle hBonePalette; xiiRGBufferHandle hMorphWeights; xiiRGBufferHandle hSkinnedOutput; xiiUInt32 uiVertexCount = 0u; }; }

void xiiSkinningAndMorphPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create persistent GPU buffers.  These grow as scene vertex counts increase.
  constexpr xiiUInt32 k_uiMaxVertices = 1u << 20u; // 1M vertices initial allocation
  if (!m_pSkinnedVertexBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 4u * k_uiMaxVertices;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pSkinnedVertexBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pSkinningInputBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 4u * k_uiMaxVertices;
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Default;
    m_pSkinningInputBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pBonePaletteBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 12u * 1024u; // 1024 bones max
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pBonePaletteBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pMorphWeightsBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(float) * 4u * 256u; // 256 morph targets max
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pMorphWeightsBuffer = pDevice->CreateBuffer(desc);
  }

  auto [pData, hPass] = graph.AddPass<SkinPassData>(
    GetName(),
    xiiGALCommandQueueFlags::Compute,
    [this](SkinPassData& data, xiiRGBuilder& builder)
    {
      data.hSkinInput    = builder.ImportBuffer("SkinInput",    m_pSkinningInputBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hSkinInput    = builder.ReadBuffer(data.hSkinInput,  xiiGALResourceStateFlags::ShaderResource);
      data.hBonePalette  = builder.ImportBuffer("BonePalette",  m_pBonePaletteBuffer,   xiiGALResourceStateFlags::ShaderResource);
      data.hBonePalette  = builder.ReadBuffer(data.hBonePalette, xiiGALResourceStateFlags::ShaderResource);
      data.hMorphWeights = builder.ImportBuffer("MorphWeights", m_pMorphWeightsBuffer,  xiiGALResourceStateFlags::ShaderResource);
      data.hMorphWeights = builder.ReadBuffer(data.hMorphWeights, xiiGALResourceStateFlags::ShaderResource);
      data.hSkinnedOutput = builder.ImportBuffer("SkinnedVertices", m_pSkinnedVertexBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hSkinnedOutput = builder.WriteBuffer(data.hSkinnedOutput, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiVertexCount  = k_uiMaxVertices;
    },
    [](const SkinPassData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Skinning + Morph");
      // Dispatch Skinning.xiiShader: [numthreads(64,1,1)], one thread per vertex.
      cmd.Dispatch((data.uiVertexCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SkinnedVertexBuffer), pData->hSkinnedOutput);
}
