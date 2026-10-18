/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/LodHistoryResetPass.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLodHistoryResetPass, 1, xiiRTTIDefaultAllocator<xiiLodHistoryResetPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SceneInstances", m_Instances)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LodHistory", m_History)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Visibility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiLodHistoryResetPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto* input = context.m_Graph.GetBufferDescription(context.GetInputBuffer(m_Instances));
  if (!input || input->m_Mode != xiiGALBufferMode::Structured || input->m_uiElementByteStride != sizeof(xiiGpuSceneInstance) || input->m_uiSize == 0 || input->m_uiSize % sizeof(xiiGpuSceneInstance) != 0 || input->m_uiSize / sizeof(xiiGpuSceneInstance) > xiiMath::MaxValue<xiiUInt32>())
    return context.ReportFailure("LOD initialization requires a canonical GPU instance buffer.");
  const auto count = static_cast<xiiUInt32>(input->m_uiSize / sizeof(xiiGpuSceneInstance));
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  const auto  groups = (xiiUInt64(count) + 63U) / 64U;
  if (limits.m_uiMaxThreadGroupSizeX < 64 || limits.m_uiMaxThreadGroupInvocations < 64 || groups > limits.m_uiMaxThreadGroupCountX)
    return context.ReportFailure("LOD initialization exceeds compute dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Visibility/LodHistoryReset.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_History;
    xiiUInt32                  m_uiGroups;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = xiiUInt64(count) * sizeof(xiiVec4U32);
      description.m_uiElementByteStride = sizeof(xiiVec4U32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data.m_History = builder.WriteBuffer("Invalid LOD Entries", description, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_uiGroups = static_cast<xiiUInt32>(groups); }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.SetPipelineState(pipeline.Borrow());
      commands.ResolveAndSetUnorderedAccessBufferView("g_LodHistory", execute.GetBuffer(data.m_History)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
      commands.DispatchCompute({data.m_uiGroups, 1, 1}); });
  return context.SetOutput(m_History, emitted.first->m_History);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Visibility_LodHistoryResetPass);
