/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIAtlasPreparePass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbePassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGIAtlasPreparePass,1,xiiRTTIDefaultAllocator<xiiDDGIAtlasPreparePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Schedule",m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousIrradiance",m_PreviousIrradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousDistance",m_PreviousDistance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Irradiance",m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Distance",m_Distance)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDDGIAtlasPreparePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto snapshot = xiiDDGIProbePassUtils::Snapshot(context, m_Schedule);
  if (!snapshot || !xiiGALDevice::HasDefaultDevice() || snapshot->m_AtlasSize.width > xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_uiMaxTexture2DDimension || snapshot->m_AtlasSize.height > xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_uiMaxTexture2DDimension)
    return context.ReportFailure("DDGI atlas extent exceeds the device or scheduling domain.");
  const auto previousIrradiance = context.GetInputTexture(m_PreviousIrradiance), previousDistance = context.GetInputTexture(m_PreviousDistance);
  if (snapshot->m_bHistoryValid)
  {
    const auto* a = context.m_Graph.GetTextureDescription(previousIrradiance);
    const auto* b = context.m_Graph.GetTextureDescription(previousDistance);
    if (!a || !b || a->m_Size != snapshot->m_AtlasSize || b->m_Size != snapshot->m_AtlasSize)
      return context.ReportFailure("DDGI atlas preparation must receive the same history pair as scheduling.");
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle m_Irradiance, m_Distance, m_PreviousIrradiance, m_PreviousDistance;
    xiiSizeU32                  m_Size;
    bool                        m_History;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size=snapshot->m_AtlasSize; data.m_History=snapshot->m_bHistoryValid;
    if(data.m_History) { data.m_PreviousIrradiance=builder.ReadTexture(previousIrradiance,xiiGALResourceStateFlags::CopySource); data.m_PreviousDistance=builder.ReadTexture(previousDistance,xiiGALResourceStateFlags::CopySource); }
    xiiGALTextureCreationDescription d; d.m_Type=xiiGALResourceDimension::Texture2D; d.m_Size=data.m_Size; d.m_Format=xiiGALResourceFormat::RGBA16Float; d.m_BindFlags=xiiGALBindFlags::ShaderResource|xiiGALBindFlags::UnorderedAccess|xiiGALBindFlags::RenderTarget;
    const auto state=data.m_History ? xiiGALResourceStateFlags::CopyDestination : xiiGALResourceStateFlags::RenderTarget;
    data.m_Irradiance=builder.WriteTexture("Current Irradiance",d,state); d.m_Format=xiiGALResourceFormat::RG32Float;
    data.m_Distance=builder.WriteTexture("Current Distance",d,state); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    if(data.m_History)
    {
      const auto box=xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(),{data.m_Size.width,data.m_Size.height,1});
      commands.CopyTextureRegion(execute.GetTexture(data.m_PreviousIrradiance),{},box,execute.GetTexture(data.m_Irradiance),{},xiiVec3U32::MakeZero());
      commands.CopyTextureRegion(execute.GetTexture(data.m_PreviousDistance),{},box,execute.GetTexture(data.m_Distance),{},xiiVec3U32::MakeZero());
    }
    else
    {
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands,execute.GetTexture(data.m_Irradiance)->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor::MakeZero());
      xiiRenderPassUtils::ClearRenderTargetPreservingState(commands,execute.GetTexture(data.m_Distance)->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor::MakeZero());
    } });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Irradiance, pass.first->m_Irradiance));
  return context.SetOutput(m_Distance, pass.first->m_Distance);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_DDGIAtlasPreparePass);
