/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/InstanceCullPass.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInstanceCullPass, 1, xiiRTTIDefaultAllocator<xiiInstanceCullPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_Instances)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Counter", m_Counter)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitializedLodHistory", m_InitializedLod)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousLodHistory", m_PreviousLod)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleInstances", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleInstanceCount", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LodHistory", m_Lod)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visibility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiInstanceCullPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto scene = context.GetInputBuffer(m_Instances), view = context.GetInputBuffer(m_View), geometry = context.GetInputBuffer(m_Geometry), candidates = context.GetInputBuffer(m_Candidates);
  const auto counter = context.GetInputBuffer(m_Counter), current = context.GetInputBuffer(m_InitializedLod);
  auto       previous = context.GetInputBuffer(m_PreviousLod);
  const auto valid    = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride && description->m_uiSize >= stride && description->m_uiSize % stride == 0;
  };
  if (!valid(scene, sizeof(xiiGpuSceneInstance)) || !valid(view, sizeof(xiiGpuVisibilityView)) || !valid(geometry, sizeof(xiiGpuGeometryRecord)) || !valid(candidates, sizeof(xiiUInt32)) || !valid(counter, sizeof(xiiUInt32)) || !valid(current, sizeof(xiiVec4U32)) || (previous.IsValid() && (!valid(previous, sizeof(xiiVec4U32)) || previous.m_Id == current.m_Id)))
    return context.ReportFailure("Instance culling buffer layouts or previous/current LOD ownership are invalid.");
  const auto capacity = context.m_Graph.GetBufferDescription(scene)->m_uiSize / sizeof(xiiGpuSceneInstance);
  if (capacity > xiiMath::MaxValue<xiiUInt32>() || context.m_Graph.GetBufferDescription(current)->m_uiSize / sizeof(xiiVec4U32) < capacity)
    return context.ReportFailure("LOD storage does not cover the instance capacity.");
  const auto work   = xiiMath::Max(capacity, context.m_Graph.GetBufferDescription(candidates)->m_uiSize / sizeof(xiiUInt32));
  const auto groups = (work + 63U) / 64U;
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 64 || groups > limits.m_uiMaxThreadGroupCountX)
    return context.ReportFailure("Instance culling exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Visibility/GpuSceneInstanceCull.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  if (!previous.IsValid())
  {
    const auto initialized = context.m_Graph.AddPass<xiiRenderGraphBufferHandle>("Absent LOD History", xiiGALCommandQueueFlags::Transfer, [](auto& data, xiiRenderGraphBuilder& builder) {
        xiiGALBufferCreationDescription description;
        description.m_uiSize = sizeof(xiiVec4U32);
        description.m_uiElementByteStride = sizeof(xiiVec4U32);
        description.m_Mode = xiiGALBufferMode::Structured;
        description.m_BindFlags = xiiGALBindFlags::ShaderResource;
        data = builder.WriteBuffer("Absent Previous LOD", description, xiiGALResourceStateFlags::CopyDestination); }, [](const auto& data, xiiRenderGraphPassContext& execute) {
        const xiiVec4U32 invalid(xiiInvalidIndex);
        execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&invalid), sizeof(invalid))); });
    previous               = *initialized.first;
  }
  struct Data
  {
    xiiRenderGraphBufferHandle m_Scene, m_View, m_Geometry, m_Candidates, m_Previous, m_Current, m_Visible, m_Count;
    xiiUInt32                  m_uiGroups;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Scene = builder.ReadBuffer(scene, xiiGALResourceStateFlags::ShaderResource);
      data.m_View = builder.ReadBuffer(view, xiiGALResourceStateFlags::ShaderResource);
      data.m_Geometry = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
      data.m_Candidates = builder.ReadBuffer(candidates, xiiGALResourceStateFlags::ShaderResource);
      data.m_Previous = builder.ReadBuffer(previous, xiiGALResourceStateFlags::ShaderResource);
      data.m_Current = builder.ReadWriteBuffer(current, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_Count = builder.ReadWriteBuffer(counter, xiiGALResourceStateFlags::UnorderedAccess);
      xiiGALBufferCreationDescription description;
      description.m_uiSize = capacity * sizeof(xiiUInt32);
      description.m_uiElementByteStride = sizeof(xiiUInt32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data.m_Visible = builder.WriteBuffer("Visible Instances", description, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_uiGroups = static_cast<xiiUInt32>(groups); }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.SetPipelineState(pipeline.Borrow());
      commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execute.GetBuffer(data.m_Scene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibilityView", execute.GetBuffer(data.m_View)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_Geometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_CandidateInstances", execute.GetBuffer(data.m_Candidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_PreviousLodHistory", execute.GetBuffer(data.m_Previous)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_CurrentLodHistory", execute.GetBuffer(data.m_Current)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_VisibleInstances", execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_VisibleInstanceCount", execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
      commands.DispatchCompute({data.m_uiGroups, 1, 1}); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Visible, emitted.first->m_Visible));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Count, emitted.first->m_Count));
  return context.SetOutput(m_Lod, emitted.first->m_Current);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_InstanceCullPass);
