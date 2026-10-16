/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/TextureSelectionPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureSelectionPass, 1, xiiRTTIDefaultAllocator<xiiTextureSelectionPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PreferPrimary", m_bPreferPrimary)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Utility")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
void xiiTextureSelectionPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  xiiRenderPassUtils::ResourcePins(pins);
  pins.PushBack({"Primary", xiiRenderPipelinePin::Texture, true, true});
  pins.PushBack({"Fallback", xiiRenderPipelinePin::Texture, true, false});
  pins.PushBack({"Texture", xiiRenderPipelinePin::Texture, false, false});
  pins.PeekBack().m_Access = xiiRenderPipelinePinAccess::Read;
}
xiiResult xiiTextureSelectionPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto primary = context.GetInputTexture("Primary");
  return context.SetOutput("Texture", m_bPreferPrimary && primary.IsValid() ? primary : context.GetInputTexture("Fallback"));
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_TextureSelectionPass);
