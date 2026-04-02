#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ColorGradingPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiColorGradingPass, 1, xiiRTTIDefaultAllocator<xiiColorGradingPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ColorGradingPass")), XII_MEMBER_PROPERTY("VignetteStrength", m_fVignetteStrength)->AddAttributes(new xiiDefaultValueAttribute(0.3f)), XII_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new xiiDefaultValueAttribute(1.0f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiColorGradAutoReg { xiiColorGradAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiColorGradingPass)); } }; static xiiColorGradAutoReg s_AutoReg; }

xiiColorGradingPass::xiiColorGradingPass() : xiiRenderPipelinePass("ColorGradingPass") {}
xiiColorGradingPass::~xiiColorGradingPass() = default;

namespace { struct ColorGradData { xiiRGTextureHandle hLDR, hLUT, hGraded; xiiUInt32 uiW = 1u, uiH = 1u; float fVignette = 0.3f, fGrain = 0.02f, fSat = 1.0f, fContrast = 1.0f; }; }

void xiiColorGradingPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hLDR;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_LDRSceneColor), hLDR);

  xiiGALTextureCreationDescription gradedDesc;
  gradedDesc.m_uiWidth = uiW; gradedDesc.m_uiHeight = uiH; gradedDesc.m_uiMipLevels = 1u;
  gradedDesc.m_Format  = xiiGALTextureFormat::R8G8B8A8UNorm;
  gradedDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<ColorGradData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hLDR, gradedDesc, uiW, uiH, this](ColorGradData& data, xiiRGBuilder& builder)
    {
      if (hLDR.IsValid()) data.hLDR = builder.ReadTexture(hLDR, xiiGALResourceStateFlags::ShaderResource);
      if (m_pColorLUT) {
        data.hLUT = builder.ImportTexture("ColorLUT", m_pColorLUT, xiiGALResourceStateFlags::ShaderResource);
        data.hLUT = builder.ReadTexture(data.hLUT, xiiGALResourceStateFlags::ShaderResource);
      }
      data.hGraded   = builder.WriteTexture("GradedColor", gradedDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH;
      data.fVignette = m_fVignetteStrength; data.fGrain = m_fGrainStrength;
      data.fSat = m_fSaturation; data.fContrast = m_fContrast;
    },
    [](const ColorGradData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Color Grading");
      // ColorGrading.xiiShader: 33^3 LUT sample + vignette + temporal grain + gamut map.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GradedColor), pData->hGraded);
}
