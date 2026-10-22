/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletCullPass.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletCullPass, 1, xiiRTTIDefaultAllocator<xiiMeshletCullPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SceneInstances", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleInstances", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleInstanceCount", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LodHistory", m_Lod)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DispatchArguments", m_Arguments)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialCount", m_InitialCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialOverflow", m_InitialOverflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshlets", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshletCount", m_OutputCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshletOverflow", m_Overflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets)->AddAttributes(new xiiDefaultValueAttribute(1024U * 1024U), new xiiClampValueAttribute(1U, 16777216U)),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visibility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiMeshletCullPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiMaxVisibleMeshlets > 0 && m_uiMaxVisibleMeshlets <= 16777216U ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiMeshletCullPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto scene = context.GetInputBuffer(m_Scene), view = context.GetInputBuffer(m_View), geometry = context.GetInputBuffer(m_Geometry), meshlets = context.GetInputBuffer(m_Meshlets);
  const auto visible = context.GetInputBuffer(m_Visible), count = context.GetInputBuffer(m_Count), lod = context.GetInputBuffer(m_Lod), arguments = context.GetInputBuffer(m_Arguments);
  const auto initialCount = context.GetInputBuffer(m_InitialCount), initialOverflow = context.GetInputBuffer(m_InitialOverflow);
  const auto valid = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride && description->m_uiSize >= stride && description->m_uiSize % stride == 0 && description->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  if (!valid(scene, sizeof(xiiGpuSceneInstance)) || !valid(view, sizeof(xiiGpuVisibilityView)) || !valid(geometry, sizeof(xiiGpuGeometryRecord)) || !valid(meshlets, sizeof(xiiMeshlet)) || !valid(visible, sizeof(xiiUInt32)) || !valid(count, sizeof(xiiUInt32)) || !valid(lod, sizeof(xiiVec4U32)) || !valid(arguments, sizeof(xiiUInt32)) || context.m_Graph.GetBufferDescription(arguments)->m_uiSize < 3 * sizeof(xiiUInt32) || !valid(initialCount, sizeof(xiiUInt32)) || !valid(initialOverflow, sizeof(xiiUInt32)) || initialCount.GetResourceId() == initialOverflow.GetResourceId() || initialCount.GetResourceId() == count.GetResourceId() || initialOverflow.GetResourceId() == count.GetResourceId() || initialCount.GetResourceId() == visible.GetResourceId() || initialOverflow.GetResourceId() == visible.GetResourceId() || initialCount.GetResourceId() == arguments.GetResourceId() || initialOverflow.GetResourceId() == arguments.GetResourceId())
    return context.ReportFailure("Meshlet culling requires canonical inputs and independently reset output counters.");
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (limits.m_uiMaxThreadGroupSizeX < 64 || limits.m_uiMaxThreadGroupInvocations < 64)
    return context.ReportFailure("Meshlet culling requires 64-thread compute groups.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Visibility/MeshletCull.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Scene, m_View, m_Geometry, m_Meshlets, m_Visible, m_Count, m_Lod, m_Arguments, m_Output, m_OutputCount, m_Overflow;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Scene = builder.ReadBuffer(scene, xiiGALResourceStateFlags::ShaderResource);
      data.m_View = builder.ReadBuffer(view, xiiGALResourceStateFlags::ShaderResource);
      data.m_Geometry = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
      data.m_Meshlets = builder.ReadBuffer(meshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_Visible = builder.ReadBuffer(visible, xiiGALResourceStateFlags::ShaderResource);
      data.m_Count = builder.ReadBuffer(count, xiiGALResourceStateFlags::ShaderResource);
      data.m_Lod = builder.ReadBuffer(lod, xiiGALResourceStateFlags::ShaderResource);
      data.m_Arguments = builder.ReadBuffer(arguments, xiiGALResourceStateFlags::IndirectArgument);
      data.m_OutputCount = builder.ReadWriteBuffer(initialCount, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_Overflow = builder.ReadWriteBuffer(initialOverflow, xiiGALResourceStateFlags::UnorderedAccess);
      xiiGALBufferCreationDescription description;
      description.m_uiSize = xiiUInt64(m_uiMaxVisibleMeshlets) * sizeof(xiiVec2U32);
      description.m_uiElementByteStride = sizeof(xiiVec2U32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data.m_Output = builder.WriteBuffer("Visible Meshlet Pairs", description, xiiGALResourceStateFlags::UnorderedAccess); }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.SetPipelineState(pipeline.Borrow());
      commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execute.GetBuffer(data.m_Scene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibilityView", execute.GetBuffer(data.m_View)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_Geometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_Meshlets", execute.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleInstances", execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleInstanceCount", execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_CurrentLodHistory", execute.GetBuffer(data.m_Lod)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_VisibleMeshlets", execute.GetBuffer(data.m_Output)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_VisibleMeshletCount", execute.GetBuffer(data.m_OutputCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_VisibleMeshletOverflow", execute.GetBuffer(data.m_Overflow)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
      commands.DispatchComputeIndirect({execute.GetBuffer(data.m_Arguments), xiiGALStateTransitionMode::None}); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Output, emitted.first->m_Output));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputCount, emitted.first->m_OutputCount));
  return context.SetOutput(m_Overflow, emitted.first->m_Overflow);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_MeshletCullPass);
