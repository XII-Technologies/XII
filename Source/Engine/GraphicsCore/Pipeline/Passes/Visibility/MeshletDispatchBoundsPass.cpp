/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletDispatchBoundsPass.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletDispatchBoundsPass, 1, xiiRTTIDefaultAllocator<xiiMeshletDispatchBoundsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleInstances", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleInstanceCount", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LodHistory", m_Lod)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitializedMaximum", m_Initial)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MaximumMeshletGroups", m_Maximum)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visibility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiMeshletDispatchBoundsPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto scene = context.GetInputBuffer(m_Scene), view = context.GetInputBuffer(m_View), geometry = context.GetInputBuffer(m_Geometry), meshlets = context.GetInputBuffer(m_Meshlets);
  const auto visible = context.GetInputBuffer(m_Visible), count = context.GetInputBuffer(m_Count), lod = context.GetInputBuffer(m_Lod), initial = context.GetInputBuffer(m_Initial);
  const auto valid = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride && description->m_uiSize >= stride && description->m_uiSize % stride == 0 && description->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  if (!valid(scene, sizeof(xiiGpuSceneInstance)) || !valid(view, sizeof(xiiGpuVisibilityView)) || !valid(geometry, sizeof(xiiGpuGeometryRecord)) || !valid(meshlets, sizeof(xiiMeshlet)) ||
      !valid(visible, sizeof(xiiUInt32)) || !valid(count, sizeof(xiiUInt32)) || !valid(lod, sizeof(xiiVec4U32)) || !valid(initial, sizeof(xiiUInt32)) ||
      initial.m_Id == count.m_Id || initial.m_Id == visible.m_Id)
    return context.ReportFailure("Meshlet dispatch bounds require canonical, independently initialized buffers.");
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto  capacity = context.m_Graph.GetBufferDescription(visible)->m_uiSize / sizeof(xiiUInt32);
  const auto  groups   = (capacity + 63U) / 64U;
  const auto& limits   = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (limits.m_uiMaxThreadGroupSizeX < 64 || limits.m_uiMaxThreadGroupInvocations < 64 || groups > limits.m_uiMaxThreadGroupCountX)
    return context.ReportFailure("Meshlet dispatch reduction exceeds compute limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Visibility/MeshletDispatchBounds.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Scene, m_View, m_Geometry, m_Meshlets, m_Visible, m_Count, m_Lod, m_Maximum;
    xiiUInt32                  m_uiGroups;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Scene = builder.ReadBuffer(scene, xiiGALResourceStateFlags::ShaderResource);
      data.m_View = builder.ReadBuffer(view, xiiGALResourceStateFlags::ShaderResource);
      data.m_Geometry = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
      data.m_Meshlets = builder.ReadBuffer(meshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_Visible = builder.ReadBuffer(visible, xiiGALResourceStateFlags::ShaderResource);
      data.m_Count = builder.ReadBuffer(count, xiiGALResourceStateFlags::ShaderResource);
      data.m_Lod = builder.ReadBuffer(lod, xiiGALResourceStateFlags::ShaderResource);
      data.m_Maximum = builder.ReadWriteBuffer(initial, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_uiGroups = static_cast<xiiUInt32>(groups); }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.SetPipelineState(pipeline.Borrow());
      commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execute.GetBuffer(data.m_Scene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibilityView", execute.GetBuffer(data.m_View)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_Geometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_Meshlets", execute.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleInstances", execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleInstanceCount", execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_CurrentLodHistory", execute.GetBuffer(data.m_Lod)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_MaximumMeshletGroups", execute.GetBuffer(data.m_Maximum)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
      commands.DispatchCompute({data.m_uiGroups, 1, 1}); });
  return context.SetOutput(m_Maximum, emitted.first->m_Maximum);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_MeshletDispatchBoundsPass);
