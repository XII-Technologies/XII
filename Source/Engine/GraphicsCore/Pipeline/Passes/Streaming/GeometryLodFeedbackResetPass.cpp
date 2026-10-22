/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackTypes.h>
#include <GraphicsFoundation/Device/Device.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGeometryLodFeedbackResetPass, 1, xiiRTTIDefaultAllocator<xiiGeometryLodFeedbackResetPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Requests", m_Requests)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Capacity", m_uiCapacity)->AddAttributes(new xiiDefaultValueAttribute(4096U), new xiiClampValueAttribute(1U, 65536U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Streaming") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiGeometryLodFeedbackResetPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiCapacity && m_uiCapacity <= 65536 && xiiMath::IsPowerOf2(m_uiCapacity) ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiGeometryLodFeedbackResetPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto  groups = (m_uiCapacity + 1ULL + 63) / 64;
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (limits.m_uiMaxThreadGroupSizeX < 64 || limits.m_uiMaxThreadGroupInvocations < 64 || groups > limits.m_uiMaxThreadGroupCountX)
    return context.ReportFailure("Geometry feedback initialization exceeds compute limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Streaming/GeometryLodFeedbackReset.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Requests;
    xiiUInt32                  m_uiGroups;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    xiiGALBufferCreationDescription description;
    description.m_uiSize = (m_uiCapacity + 1ULL) * sizeof(xiiGpuGeometryLodRequest); description.m_uiElementByteStride = sizeof(xiiGpuGeometryLodRequest);
    description.m_Mode = xiiGALBufferMode::Structured; description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    data.m_Requests = builder.WriteBuffer("Initialized Geometry Requests", description, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_uiGroups = static_cast<xiiUInt32>(groups); }, [pipeline = m_Pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList(); commands.SetPipelineState(pipeline.Borrow());
    commands.ResolveAndSetUnorderedAccessBufferView("g_Requests", execute.GetBuffer(data.m_Requests)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess(); commands.DispatchCompute({data.m_uiGroups, 1, 1}); });
  return context.SetOutput(m_Requests, emitted.first->m_Requests);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Streaming_GeometryLodFeedbackResetPass);
