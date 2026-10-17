/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletDispatchPass.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshletDispatchPass, 1, xiiRTTIDefaultAllocator<xiiMeshletDispatchPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VisibleInstances", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleInstanceCount", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MaximumMeshletGroups", m_Maximum)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DispatchArguments", m_Arguments)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visibility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiMeshletDispatchPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto visible = context.GetInputBuffer(m_Visible), count = context.GetInputBuffer(m_Count), meshlets = context.GetInputBuffer(m_Meshlets), maximum = context.GetInputBuffer(m_Maximum);
  const auto valid = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* description = context.m_Graph.GetBufferDescription(handle);
    return description && description->m_Mode == xiiGALBufferMode::Structured && description->m_uiElementByteStride == stride && description->m_uiSize >= stride && description->m_uiSize % stride == 0 && description->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  if (!valid(visible, sizeof(xiiUInt32)) || !valid(count, sizeof(xiiUInt32)) || !valid(meshlets, sizeof(xiiMeshlet)) || !valid(maximum, sizeof(xiiUInt32)))
    return context.ReportFailure("Meshlet dispatch requires structured visibility, metadata and bound buffers.");
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto& limits          = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  const auto  visibleCapacity = context.m_Graph.GetBufferDescription(visible)->m_uiSize / sizeof(xiiUInt32);
  const auto  meshletCapacity = context.m_Graph.GetBufferDescription(meshlets)->m_uiSize / sizeof(xiiMeshlet);
  if ((meshletCapacity + 63U) / 64U > limits.m_uiMaxThreadGroupCountX || limits.m_uiMaxThreadGroupCountY < 65535U ||
      visibleCapacity > xiiMath::MaxValue<xiiUInt32>() - 65534U || (visibleCapacity + 65534U) / 65535U > limits.m_uiMaxThreadGroupCountZ)
    return context.ReportFailure("Meshlet indirect dimensions exceed the device's compute limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Visibility/MeshletDispatch.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Visible, m_Count, m_Meshlets, m_Maximum, m_Arguments;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Visible = builder.ReadBuffer(visible, xiiGALResourceStateFlags::ShaderResource);
      data.m_Count = builder.ReadBuffer(count, xiiGALResourceStateFlags::ShaderResource);
      data.m_Meshlets = builder.ReadBuffer(meshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_Maximum = builder.ReadBuffer(maximum, xiiGALResourceStateFlags::ShaderResource);
      xiiGALBufferCreationDescription description;
      description.m_uiSize = 3 * sizeof(xiiUInt32);
      description.m_uiElementByteStride = sizeof(xiiUInt32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
      data.m_Arguments = builder.WriteBuffer("Meshlet Work Dimensions", description, xiiGALResourceStateFlags::UnorderedAccess); }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.SetPipelineState(pipeline.Borrow());
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleInstances", execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleInstanceCount", execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_Meshlets", execute.GetBuffer(data.m_Meshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_MaximumMeshletGroups", execute.GetBuffer(data.m_Maximum)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_MeshletDispatchArguments", execute.GetBuffer(data.m_Arguments)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
      commands.DispatchCompute({1, 1, 1}); });
  return context.SetOutput(m_Arguments, emitted.first->m_Arguments);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_MeshletDispatchPass);
