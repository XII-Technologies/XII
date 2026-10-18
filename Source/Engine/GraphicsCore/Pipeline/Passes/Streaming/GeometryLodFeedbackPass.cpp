/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackPass.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackTypes.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Streaming/GeometryLodFeedbackConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGeometryLodFeedbackPass, 1, xiiRTTIDefaultAllocator<xiiGeometryLodFeedbackPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitializedRequests", m_Initial)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Requests", m_Requests)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PriorityTier", m_uiPriorityTier)->AddAttributes(new xiiDefaultValueAttribute(3U), new xiiClampValueAttribute(0U, 3U)),
    XII_MEMBER_PROPERTY("PurposeMask", m_uiPurposeMask)->AddAttributes(new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Streaming") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiGeometryLodFeedbackPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiPriorityTier <= 3 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiGeometryLodFeedbackPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto scene = context.GetInputBuffer(m_Scene), view = context.GetInputBuffer(m_View), geometry = context.GetInputBuffer(m_Geometry), candidates = context.GetInputBuffer(m_Candidates), initial = context.GetInputBuffer(m_Initial);
  const auto valid = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride && description->m_uiSize >= stride && description->m_uiSize % stride == 0 && description->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  if (!valid(scene, sizeof(xiiGpuSceneInstance)) || !valid(view, sizeof(xiiGpuVisibilityView)) || !valid(geometry, sizeof(xiiGpuGeometryRecord)) || !valid(candidates, sizeof(xiiUInt32)) || !valid(initial, sizeof(xiiGpuGeometryLodRequest)))
    return context.ReportFailure("Geometry feedback requires canonical scene/view/residency records, uint candidates and an initialized request table.");
  const auto entries = context.m_Graph.GetBufferDescription(initial)->m_uiSize / sizeof(xiiGpuGeometryLodRequest);
  if (entries < 2 || entries > 65537 || !xiiMath::IsPowerOf2(static_cast<xiiUInt32>(entries - 1)))
    return context.ReportFailure("Geometry feedback table capacity must be a power of two within 1..65536, plus its header.");
  const auto work   = xiiMath::Max(context.m_Graph.GetBufferDescription(scene)->m_uiSize / sizeof(xiiGpuSceneInstance), context.m_Graph.GetBufferDescription(candidates)->m_uiSize / sizeof(xiiUInt32));
  const auto groups = (work + 63) / 64;
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (limits.m_uiMaxThreadGroupSizeX < 64 || limits.m_uiMaxThreadGroupInvocations < 64 || groups > limits.m_uiMaxThreadGroupCountX)
    return context.ReportFailure("Geometry feedback exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Streaming/GeometryLodFeedback.xiiShader"));
  auto tier = m_uiPriorityTier;
  if (const auto* value = context.m_Parameters.GetValue("StreamingPriorityTier"))
  {
    if (!value->IsA<xiiUInt32>() || value->Get<xiiUInt32>() > 3)
      return context.ReportFailure("StreamingPriorityTier must be a uint within 0..3.");
    tier = value->Get<xiiUInt32>();
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Scene, m_View, m_Geometry, m_Candidates, m_Requests, m_Constants;
    xiiUInt32                  m_uiGroups, m_uiTier, m_uiPurpose;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Scene = builder.ReadBuffer(scene, xiiGALResourceStateFlags::ShaderResource); data.m_View = builder.ReadBuffer(view, xiiGALResourceStateFlags::ShaderResource);
    data.m_Geometry = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource); data.m_Candidates = builder.ReadBuffer(candidates, xiiGALResourceStateFlags::ShaderResource);
    data.m_Requests = builder.ReadWriteBuffer(initial, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Geometry Request Parameters", sizeof(xiiGeometryLodFeedbackConstants));
    data.m_uiGroups = static_cast<xiiUInt32>(groups); data.m_uiTier = tier; data.m_uiPurpose = m_uiPurposeMask; }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    { xiiGALMapHelper<xiiGeometryLodFeedbackConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard); mapped->PriorityTier = data.m_uiTier; mapped->PurposeMask = data.m_uiPurpose; }
    xiiGALStateTransitionDescription transition; transition.m_pResource = execute.GetBuffer(data.m_Constants); transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1)); commands.SetPipelineState(pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiGeometryLodFeedbackConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execute.GetBuffer(data.m_Scene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_VisibilityView", execute.GetBuffer(data.m_View)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Geometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Candidates", execute.GetBuffer(data.m_Candidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_Requests", execute.GetBuffer(data.m_Requests)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess(); commands.DispatchCompute({data.m_uiGroups, 1, 1}); });
  return context.SetOutput(m_Requests, emitted.first->m_Requests);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Streaming_GeometryLodFeedbackPass);
