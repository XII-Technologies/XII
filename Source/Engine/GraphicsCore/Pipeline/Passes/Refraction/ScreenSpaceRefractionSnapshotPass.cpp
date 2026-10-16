/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Refraction/ScreenSpaceRefractionSnapshotPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

using namespace xiiRenderPassUtils;
struct xiiScreenSpaceRefractionSnapshotPass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hHDRSource; ///< CopySource in (completed scene color before refraction).
  xiiRenderGraphTextureHandle m_hSnapshot;  ///< CopyDestination out (immutable source for neighborhood sampling).
  xiiSizeU32                  m_Size;
};


xiiScreenSpaceRefractionSnapshotPass::xiiScreenSpaceRefractionSnapshotPass()  = default;
xiiScreenSpaceRefractionSnapshotPass::~xiiScreenSpaceRefractionSnapshotPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenSpaceRefractionSnapshotPass, 1, xiiRTTIDefaultAllocator<xiiScreenSpaceRefractionSnapshotPass>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Refraction")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


void xiiScreenSpaceRefractionSnapshotPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  ResourcePins(pins);
  pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, false, xiiGALResourceFormat::RGBA16Float});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::None;
  pins.PushBack({"RefractionSceneColorInput", xiiRenderPipelinePin::Texture, false, false, xiiGALResourceFormat::RGBA16Float});
  for (auto& pin : pins)
    if (pin.m_Type == xiiRenderPipelinePin::Texture)
    {
      pin.m_Constraints.m_Dimension = xiiGALResourceDimension::Texture2D;
      pin.m_Constraints.m_uiSamples = 1;
    }
}

xiiResult xiiScreenSpaceRefractionSnapshotPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  xiiSizeU32                 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture("HDRSceneColor"), 0, 0, size));
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hHDRSource = context.GetInputTexture("HDRSceneColor");
    Setup(data, builder); }, xiiMakeDelegate(&xiiScreenSpaceRefractionSnapshotPass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput("RefractionSceneColorInput", emitted.first->m_hSnapshot));
  return XII_SUCCESS;
}

void xiiScreenSpaceRefractionSnapshotPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSource = builder.ReadTexture(data.m_hHDRSource, xiiGALResourceStateFlags::CopySource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = data.m_Size.width;
  description.m_Size.height = data.m_Size.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSnapshot          = builder.WriteTexture("RefractionSceneColorInput", description, xiiGALResourceStateFlags::CopyDestination);

  builder.SetPassAllowMerge(false);
}

void xiiScreenSpaceRefractionSnapshotPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("SSRefractionSnapshot");
  {
    cmd.CopyTexture(context.GetTexture(data.m_hHDRSource), context.GetTexture(data.m_hSnapshot));
  }
  cmd.EndDebugGroup();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Refraction_ScreenSpaceRefractionSnapshotPass);
