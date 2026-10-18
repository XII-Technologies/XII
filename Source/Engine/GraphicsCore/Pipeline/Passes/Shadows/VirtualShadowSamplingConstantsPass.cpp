/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowSamplingConstantsPass.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualShadowSamplingConstantsPass,1,xiiRTTIDefaultAllocator<xiiVirtualShadowSamplingConstantsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Residency",m_Residency)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Constants",m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightId",m_uiLightId)->AddAttributes(new xiiDefaultValueAttribute(1U),new xiiClampValueAttribute(0U,0xFFFFFFU)),
    XII_MEMBER_PROPERTY("Enabled",m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Shadows") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiVirtualShadowSamplingConstantsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiLightId < (1U << 24) ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiVirtualShadowSamplingConstantsPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto source    = context.GetInputData(m_Residency);
  const auto residency = source ? source.Downcast<const xiiVirtualShadowResidencySnapshot>() : nullptr;
  if (!residency || !residency->m_Upload || !residency->m_Upload->m_Atlas)
    return context.ReportFailure("Virtual shadow sampling requires explicit captured device residency.");
  auto light   = m_uiLightId;
  auto enabled = m_bEnabled;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowSamplingLightId", light));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowSamplingEnabled", enabled));
  const auto& upload = *residency->m_Upload;
  const auto& layout = residency->m_Bindings;
  const auto& image  = upload.m_Atlas->GetDescription();
  if (light >= (1U << 24) || !xiiMath::IsPowerOf2(upload.m_uiVirtualResolution) || !xiiMath::IsPowerOf2(upload.m_uiPageSize) || upload.m_uiVirtualResolution < upload.m_uiPageSize || !layout.m_uiVirtualTableCapacity || !image.m_Size.HasNonZeroArea())
    return context.ReportFailure("Invalid captured virtual shadow sampling dimensions or addressing.");
  xiiVirtualShadowSamplingConstants snapshot{};
  snapshot.VirtualShadowPageTableBaseIndex  = layout.m_uiVirtualTableBaseIndex;
  snapshot.VirtualShadowPageTableCapacity   = layout.m_uiVirtualTableCapacity;
  snapshot.VirtualShadowResolution          = upload.m_uiVirtualResolution;
  snapshot.VirtualShadowPageSize            = upload.m_uiPageSize;
  snapshot.VirtualShadowPhysicalAtlasWidth  = image.m_Size.width;
  snapshot.VirtualShadowPhysicalAtlasHeight = image.m_Size.height;
  snapshot.VirtualShadowDirectionalLightId  = light;
  snapshot.VirtualShadowEnabled             = enabled ? 1U : 0U;
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle        m_Buffer;
    xiiVirtualShadowSamplingConstants m_Snapshot;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Buffer=xiiRenderPassUtils::Constants(builder,"Virtual Shadow Sampling",sizeof(snapshot)); data.m_Snapshot=snapshot; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    {
      xiiGALMapHelper<xiiVirtualShadowSamplingConstants> mapped(commands,execute.GetBuffer(data.m_Buffer),xiiGALMapType::Write,xiiGALMapFlags::Discard);
      *mapped=data.m_Snapshot;
    }
    // Dynamic mapping can stage a transfer; restore the state declared by this operation.
    xiiGALStateTransitionDescription transition;
    transition.m_pResource=execute.GetBuffer(data.m_Buffer);
    transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer;
    transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1)); });
  return context.SetOutput(m_Constants, pass.first->m_Buffer);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_VirtualShadowSamplingConstantsPass);
