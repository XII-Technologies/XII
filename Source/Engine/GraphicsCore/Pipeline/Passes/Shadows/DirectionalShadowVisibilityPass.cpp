/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/DirectionalShadowVisibilityPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>
#include <Shaders/Pipeline/Passes/Shadows/DirectionalShadowConstants.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalShadowVisibilityPass, 1, xiiRTTIDefaultAllocator<xiiDirectionalShadowVisibilityPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("CascadeSnapshot", m_Cascades)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("ShadowAtlas", m_Atlas)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Depth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Normal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("ObjectId", m_ObjectId)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Camera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Visibility", m_Visibility)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("DepthBias", m_fDepthBias)->AddAttributes(new xiiDefaultValueAttribute(0.0005f), new xiiClampValueAttribute(0.0f, 0.1f)),
      XII_MEMBER_PROPERTY("NormalBias", m_fNormalBias)->AddAttributes(new xiiDefaultValueAttribute(0.02f), new xiiClampValueAttribute(0.0f, 10.0f)),
      XII_MEMBER_PROPERTY("NormalBiasTexels", m_fNormalBiasTexels)->AddAttributes(new xiiDefaultValueAttribute(1.5f), new xiiClampValueAttribute(0.0f, 4.0f)),
    } XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {new xiiCategoryAttribute("Rendering/Shadows")} XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult xiiDirectionalShadowVisibilityPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fDepthBias) && m_fDepthBias >= 0 && m_fDepthBias <= 0.1f && xiiMath::IsFinite(m_fNormalBias) && m_fNormalBias >= 0 && m_fNormalBias <= 10 && xiiMath::IsFinite(m_fNormalBiasTexels) && m_fNormalBiasTexels >= 0 && m_fNormalBiasTexels <= 4 ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiDirectionalShadowVisibilityPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto input = context.GetInputData(m_Cascades);
  if (!input)
    return XII_SUCCESS;
  const auto* cascades = xiiDynamicCast<const xiiShadowCascadeSnapshot*>(input.Borrow());
  const auto  depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal), ids = context.GetInputTexture(m_ObjectId), atlas = context.GetInputTexture(m_Atlas);
  const auto  camera           = context.GetInputBuffer(m_Camera);
  const auto* surface          = xiiScreenSpacePassUtils::Image(context, depth, 1);
  const auto* atlasDescription = context.m_Graph.GetTextureDescription(atlas);
  const auto* identities       = context.m_Graph.GetTextureDescription(ids);
  if (!cascades || cascades->m_Cascades.IsEmpty() || !surface || !xiiScreenSpacePassUtils::Image(context, normal, 2, &surface->m_Size) || !identities || identities->m_Size != surface->m_Size || !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)) || !context.m_pView || !context.m_pView->GetGpuInstanceData() || !atlasDescription)
    return context.ReportFailure("Directional shadow visibility requires a completed atlas, matching surface images and authoritative receiver identities.");
  const auto resolution = cascades->m_uiResolution;
  if (!resolution || !xiiScreenSpacePassUtils::Uniform(context, cascades->m_hConstants, sizeof(xiiShadowCascadeConstants)))
    return context.ReportFailure("Directional shadow sampling requires exact cascade constants and a positive resolution.");
  const xiiSizeU32 atlasSize(resolution * (cascades->m_Cascades.GetCount() > 1 ? 2U : 1U), resolution * (cascades->m_Cascades.GetCount() > 2 ? 2U : 1U));
  if (atlasDescription->m_Size != atlasSize)
    return context.ReportFailure("Published directional atlas does not match the cascade snapshot.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Shadows/DirectionalShadowVisibility.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Depth, m_Normal, m_Ids, m_Atlas, m_Output;
    xiiRenderGraphBufferHandle               m_Camera, m_Cascades, m_Constants, m_Receivers;
    xiiDirectionalShadowConstants            m_Snapshot;
    xiiDynamicArray<xiiVec2U32>              m_Flags;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Ids = builder.ReadTexture(ids, xiiGALResourceStateFlags::ShaderResource);
    data.m_Atlas = builder.ReadTexture(atlas, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Cascades = builder.ReadBuffer(cascades->m_hConstants, xiiGALResourceStateFlags::ConstantBuffer);
    // GPU identities are stable values, not packed array indices. Publish a sorted,
    // immutable receiver table rather than searching the whole world for every pixel.
    for (const auto& instance : context.m_pView->GetGpuInstanceData()->GetInstances())
      data.m_Flags.PushBack(xiiVec2U32(instance.m_uiObjectId, instance.m_uiFlags));
    data.m_Flags.Sort([](const xiiVec2U32& a, const xiiVec2U32& b) { return a.x < b.x; });
    data.m_Snapshot.ShadowExtent = xiiVec4U32(surface->m_Size.width, surface->m_Size.height, resolution, data.m_Flags.GetCount());
    data.m_Snapshot.ShadowBias   = xiiVec4(m_fDepthBias, m_fNormalBias, m_fNormalBiasTexels, 0);
    if (data.m_Flags.IsEmpty()) data.m_Flags.PushBack(xiiVec2U32(0, 0));
    xiiGALBufferCreationDescription receiver;
    receiver.m_uiSize = data.m_Flags.GetCount() * sizeof(xiiVec2U32);
    receiver.m_uiElementByteStride = sizeof(xiiVec2U32);
    receiver.m_Mode = xiiGALBufferMode::Structured;
    receiver.m_BindFlags = xiiGALBindFlags::ShaderResource;
    data.m_Receivers = builder.WriteBuffer("Shadow Receiver Flags", receiver, xiiGALResourceStateFlags::ShaderResource);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Shadow Sampling Constants", sizeof(data.m_Snapshot));
    data.m_Output = xiiCompositionPassUtils::Output(builder, surface->m_Size, xiiGALResourceFormat::R32Float);
    data.m_Pipeline = m_Pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    xiiRenderPassUtils::UpdateBufferPreservingState(commands, execute.GetBuffer(data.m_Receivers), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Flags.GetData()), data.m_Flags.GetCount() * sizeof(xiiVec2U32)));
    { xiiGALMapHelper<xiiDirectionalShadowConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard); *mapped = data.m_Snapshot; }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", execute.GetBuffer(data.m_Cascades), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiDirectionalShadowConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_ShadowReceivers", execute.GetBuffer(data.m_Receivers)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    const auto bind = [&](const xiiTempHashedString& name, xiiRenderGraphTextureHandle texture) { commands.ResolveAndSetShaderResourceTextureView(name, execute.GetTexture(texture)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute); };
    bind("g_SceneDepth", data.m_Depth); bind("g_SceneNormal", data.m_Normal); bind("g_ObjectId", data.m_Ids); bind("g_ShadowAtlas", data.m_Atlas);
    commands.ResolveAndSetUnorderedAccessTextureView("g_ShadowVisibility", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
    commands.DispatchCompute({(data.m_Snapshot.ShadowExtent.x + 7) / 8, (data.m_Snapshot.ShadowExtent.y + 7) / 8, 1}); });
  return context.SetOutput(m_Visibility, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_DirectionalShadowVisibilityPass);
