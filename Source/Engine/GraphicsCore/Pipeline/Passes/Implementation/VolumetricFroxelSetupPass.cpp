#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/VolumetricFroxelSetupPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVolumetricFroxelSetupPass, 1, xiiRTTIDefaultAllocator<xiiVolumetricFroxelSetupPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("VolumetricFroxelSetupPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiVolumetricFroxelSetupPass::xiiVolumetricFroxelSetupPass() : xiiRenderPipelinePass("VolumetricFroxelSetupPass") {}
xiiVolumetricFroxelSetupPass::~xiiVolumetricFroxelSetupPass() = default;

namespace { struct FroxelData { xiiRGBufferHandle hFroxelMeta; xiiRGTextureHandle hFroxelScattering; xiiUInt32 cX = 160u, cY = 90u, cZ = 64u; }; }

void xiiVolumetricFroxelSetupPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALBufferCreationDescription metaDesc;
  metaDesc.m_uiSize        = sizeof(float) * 4u * m_uiFroxelCountX * m_uiFroxelCountY * m_uiFroxelCountZ;
  metaDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  metaDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  xiiGALTextureCreationDescription scatterDesc;
  scatterDesc.m_uiWidth = m_uiFroxelCountX; scatterDesc.m_uiHeight = m_uiFroxelCountY;
  scatterDesc.m_uiDepth = m_uiFroxelCountZ; scatterDesc.m_uiMipLevels = 1u;
  scatterDesc.m_TextureType = xiiGALTextureType::Texture3D;
  scatterDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  scatterDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<FroxelData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [metaDesc, scatterDesc, this](FroxelData& data, xiiRGBuilder& builder)
    {
      data.hFroxelMeta       = builder.WriteBuffer("FroxelMetadata",   metaDesc,   xiiGALResourceStateFlags::UnorderedAccess);
      data.hFroxelScattering = builder.WriteTexture("FroxelScattering", scatterDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.cX = m_uiFroxelCountX; data.cY = m_uiFroxelCountY; data.cZ = m_uiFroxelCountZ;
    },
    [](const FroxelData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Volumetric Froxel Setup");
      // FroxelSetup.xiiShader: [numthreads(8,8,1)] — computes per-froxel bounds+phase terms.
      cmd.Dispatch((data.cX + 7u) / 8u, (data.cY + 7u) / 8u, data.cZ);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelMetadataBuffer),  pData->hFroxelMeta);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelScatteringBuffer), pData->hFroxelScattering);
}
