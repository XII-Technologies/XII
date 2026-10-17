/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/VertexPullDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVertexPullDrawCommandsPass, 1, xiiRTTIDefaultAllocator<xiiVertexPullDrawCommandsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshlets", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshletCount", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommands", m_Commands)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommandCount", m_CommandCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Geometry") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiVertexPullDrawCommandsPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto scene = context.GetInputBuffer(m_Scene), view = context.GetInputBuffer(m_View), geometry = context.GetInputBuffer(m_Geometry);
  const auto meshlets = context.GetInputBuffer(m_Meshlets), visible = context.GetInputBuffer(m_Visible), count = context.GetInputBuffer(m_Count);
  const auto valid = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride &&
      description->m_uiSize >= stride && description->m_uiSize % stride == 0 && description->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  if (!valid(scene, sizeof(xiiGpuSceneInstance)) || !valid(view, sizeof(xiiGpuVisibilityView)) || !valid(geometry, sizeof(xiiGpuGeometryRecord)) ||
      !valid(meshlets, sizeof(xiiMeshlet)) || !valid(visible, sizeof(xiiVec2U32)) || !valid(count, sizeof(xiiUInt32)))
    return context.ReportFailure("Vertex-pull draw preparation requires canonical structured inputs.");
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto  capacity = context.m_Graph.GetBufferDescription(visible)->m_uiSize / sizeof(xiiVec2U32);
  const auto  groups   = (capacity + 63U) / 64U;
  const auto& limits   = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (limits.m_uiMaxThreadGroupSizeX < 64 || limits.m_uiMaxThreadGroupInvocations < 64 || groups > limits.m_uiMaxThreadGroupCountX)
    return context.ReportFailure("Vertex-pull draw preparation exceeds compute limits; reduce the supplied meshlet capacity.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Geometry/VertexPullDrawCommands.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Scene, m_View, m_Geometry, m_Meshlets, m_Visible, m_Count, m_Commands, m_CommandCount;
    xiiUInt32                  m_uiGroups;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Scene = builder.ReadBuffer(scene, xiiGALResourceStateFlags::ShaderResource);
      data.m_View = builder.ReadBuffer(view, xiiGALResourceStateFlags::ShaderResource);
      data.m_Geometry = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
      data.m_Meshlets = builder.ReadBuffer(meshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_Visible = builder.ReadBuffer(visible, xiiGALResourceStateFlags::ShaderResource);
      data.m_Count = builder.ReadBuffer(count, xiiGALResourceStateFlags::ShaderResource);
      xiiGALBufferCreationDescription description;
      description.m_uiSize = capacity * sizeof(xiiVec4U32);
      description.m_uiElementByteStride = sizeof(xiiVec4U32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
      data.m_Commands = builder.WriteBuffer("Vertex-pull Draw Commands", description, xiiGALResourceStateFlags::UnorderedAccess);
      description.m_uiSize = sizeof(xiiUInt32);
      description.m_uiElementByteStride = sizeof(xiiUInt32);
      data.m_CommandCount = builder.WriteBuffer("Vertex-pull Draw Count", description, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_uiGroups = static_cast<xiiUInt32>(groups); }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.SetPipelineState(pipeline.Borrow());
      commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances", execute.GetBuffer(data.m_Scene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibilityView", execute.GetBuffer(data.m_View)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_Geometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_Meshlets", execute.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_DrawCommands", execute.GetBuffer(data.m_Commands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_DrawCommandCount", execute.GetBuffer(data.m_CommandCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
      commands.DispatchCompute({data.m_uiGroups, 1, 1}); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Commands, emitted.first->m_Commands));
  return context.SetOutput(m_CommandCount, emitted.first->m_CommandCount);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Geometry_VertexPullDrawCommandsPass);
