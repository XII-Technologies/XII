/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Visibility/GpuSceneCommandBuildConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshDrawCommandsPass, 1, xiiRTTIDefaultAllocator<xiiMeshDrawCommandsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VisibleMeshlets", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshletCount", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommands", m_Commands)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommandCount", m_CommandCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshGroupCountX", m_uiMeshGroupCountX)->AddAttributes(new xiiDefaultValueAttribute(65535U), new xiiClampValueAttribute(1U, xiiMath::MaxValue<xiiUInt32>())),
    XII_MEMBER_PROPERTY("MeshGroupCountY", m_uiMeshGroupCountY)->AddAttributes(new xiiDefaultValueAttribute(1U), new xiiClampValueAttribute(1U, xiiMath::MaxValue<xiiUInt32>())),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Geometry") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiMeshDrawCommandsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiMeshGroupCountX && m_uiMeshGroupCountY && xiiUInt64(m_uiMeshGroupCountX) * m_uiMeshGroupCountY <= xiiMath::MaxValue<xiiUInt32>() ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiMeshDrawCommandsPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  visible = context.GetInputBuffer(m_Visible), count = context.GetInputBuffer(m_Count);
  const auto* stream  = context.m_Graph.GetBufferDescription(visible);
  const auto* counter = context.m_Graph.GetBufferDescription(count);
  if (!stream || stream->m_Mode != xiiGALBufferMode::Structured || stream->m_uiElementByteStride != sizeof(xiiVec2U32) || stream->m_uiSize < sizeof(xiiVec2U32) || stream->m_uiSize % sizeof(xiiVec2U32) != 0 || !counter || counter->m_Mode != xiiGALBufferMode::Structured || counter->m_uiElementByteStride != sizeof(xiiUInt32) || counter->m_uiSize < sizeof(xiiUInt32))
    return context.ReportFailure("Mesh draw preparation requires a compact uint2 stream and a structured uint counter.");
  if (!xiiGALDevice::HasDefaultDevice() || xiiGALDevice::GetDefaultDevice()->GetFeatures().m_MeshShaders != xiiGALDeviceFeatureState::Enabled)
    return XII_FAILURE;
  const auto&     limits   = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_MeshShaderProperties;
  const xiiUInt64 capacity = stream->m_uiSize / sizeof(xiiVec2U32);
  const xiiUInt64 width    = xiiMath::Min<xiiUInt64>(capacity, m_uiMeshGroupCountX);
  const xiiUInt64 rows     = (capacity + m_uiMeshGroupCountX - 1) / m_uiMeshGroupCountX;
  const xiiUInt64 height   = xiiMath::Min<xiiUInt64>(rows, m_uiMeshGroupCountY);
  const xiiUInt64 depth    = (rows + m_uiMeshGroupCountY - 1) / m_uiMeshGroupCountY;
  if (capacity > xiiMath::MaxValue<xiiUInt32>() || m_uiMeshGroupCountX > limits.m_uiMaxThreadGroupCountX || m_uiMeshGroupCountY > limits.m_uiMaxThreadGroupCountY || depth > limits.m_uiMaxThreadGroupCountZ || width * height * depth > limits.m_uiMaxThreadGroupTotalCount)
    return context.ReportFailure("Authored mesh command strides cannot cover the supplied stream within device limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Geometry/MeshDrawCommands.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle       m_Visible, m_Count, m_Commands, m_CommandCount, m_Constants;
    xiiGpuSceneCommandBuildConstants m_Snapshot;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Visible = builder.ReadBuffer(visible, xiiGALResourceStateFlags::ShaderResource);
      data.m_Count = builder.ReadBuffer(count, xiiGALResourceStateFlags::ShaderResource);
      xiiGALBufferCreationDescription description;
      description.m_uiSize = sizeof(xiiVec3U32);
      description.m_uiElementByteStride = sizeof(xiiVec3U32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
      data.m_Commands = builder.WriteBuffer("Mesh Draw Command", description, xiiGALResourceStateFlags::UnorderedAccess);
      description.m_uiSize = description.m_uiElementByteStride = sizeof(xiiUInt32);
      data.m_CommandCount = builder.WriteBuffer("Mesh Draw Command Count", description, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_Constants = xiiRenderPassUtils::Constants(builder, "Mesh Draw Strides", sizeof(data.m_Snapshot));
      data.m_Snapshot.MaxMeshGroupCountX = m_uiMeshGroupCountX;
      data.m_Snapshot.MaxMeshGroupCountY = m_uiMeshGroupCountY;
      data.m_Snapshot.MaxMeshGroupTotalCount = static_cast<xiiUInt32>(capacity);
      data.m_Snapshot.Padding = 0; }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      {
        xiiGALMapHelper<xiiGpuSceneCommandBuildConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_Snapshot;
      }
      commands.SetPipelineState(pipeline.Borrow());
      commands.ResolveAndSetConstantBuffer("xiiGpuSceneCommandBuildConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", execute.GetBuffer(data.m_Visible)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", execute.GetBuffer(data.m_Count)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_DrawCommands", execute.GetBuffer(data.m_Commands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.ResolveAndSetUnorderedAccessBufferView("g_DrawCommandCount", execute.GetBuffer(data.m_CommandCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
      commands.DispatchCompute({1, 1, 1}); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Commands, emitted.first->m_Commands));
  return context.SetOutput(m_CommandCount, emitted.first->m_CommandCount);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Geometry_MeshDrawCommandsPass);
