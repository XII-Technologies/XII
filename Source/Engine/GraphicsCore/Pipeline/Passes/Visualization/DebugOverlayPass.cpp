/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/DebugOverlayPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDebugOverlayPass, 1, xiiRTTIDefaultAllocator<xiiDebugOverlayPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DisplayLinearColorInput", m_InputDisplayLinearColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("DisplayLinearColor")),
    XII_MEMBER_PROPERTY("SceneDepthInput", m_InputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("SceneDepth")),
    XII_MEMBER_PROPERTY("DisplayLinearColorOutput", m_OutputDisplayLinearColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("DisplayLinearColor")),
    XII_MEMBER_PROPERTY("SceneDepthOutput", m_OutputSceneDepth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("SceneDepth")),
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visualization") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiDebugOverlayPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  auto                       color = context.GetInputTexture(m_InputDisplayLinearColor);
  auto                       depth = context.GetInputTexture(m_InputSceneDepth);
  if (m_bEnabled)
    XII_SUCCEED_OR_RETURN(xiiDebugRenderer::AddRenderGraphPasses(context.m_Graph, color, depth));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputDisplayLinearColor, color));
  return context.SetOutput(m_OutputSceneDepth, depth);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visualization_DebugOverlayPass);
