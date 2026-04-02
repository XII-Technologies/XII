#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/LocalLightShadowPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLocalLightShadowPass, 1, xiiRTTIDefaultAllocator<xiiLocalLightShadowPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("LocalLightShadowPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiLocalShadowAutoReg { xiiLocalShadowAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiLocalLightShadowPass)); } }; static xiiLocalShadowAutoReg s_AutoReg; }

xiiLocalLightShadowPass::xiiLocalLightShadowPass() : xiiRenderPipelinePass("LocalLightShadowPass") {}
xiiLocalLightShadowPass::~xiiLocalLightShadowPass() = default;

namespace { struct LocalShadowData { xiiRGTextureHandle hShadowAtlas; xiiRGBufferHandle hAtlasDescs; xiiUInt32 uiAtlasSize = 4096u; }; }

void xiiLocalLightShadowPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALTextureCreationDescription atlasDesc;
  atlasDesc.m_uiWidth = m_uiAtlasSize; atlasDesc.m_uiHeight = m_uiAtlasSize; atlasDesc.m_uiMipLevels = 1u;
  atlasDesc.m_Format    = xiiGALTextureFormat::D16UNorm;
  atlasDesc.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  xiiGALBufferCreationDescription descsDesc;
  descsDesc.m_uiSize        = sizeof(xiiUInt32) * 4u * m_uiMaxShadowedLights; // (x,y,size,flags) per light
  descsDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  descsDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  auto [pData, hPass] = graph.AddPass<LocalShadowData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [atlasDesc, descsDesc, this](LocalShadowData& data, xiiRGBuilder& builder)
    {
      data.hShadowAtlas = builder.WriteTexture("LocalShadowAtlas", atlasDesc, xiiGALResourceStateFlags::DepthWrite);
      data.hAtlasDescs  = builder.WriteBuffer("LocalShadowAtlasDescs", descsDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiAtlasSize  = m_uiAtlasSize;
      builder.SetPassAllowMerge(false);
    },
    [](const LocalShadowData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Local Light Shadows");
      // Pass 18: LocalLightShadowAtlasAllocation.xiiShader — deterministic atlas tile assignment.
      cmd.Dispatch(1u, 1u, 1u);
      // Pass 19: ShadowDepth.xiiShader — indirect draws per shadowed light into allocated tiles.
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LocalShadowAtlas),      pData->hShadowAtlas);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs), pData->hAtlasDescs);
}
