/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowFeedbackResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowFeedbackTypes.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVirtualShadowFeedback, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVirtualShadowFeedback>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VirtualKeyLow", m_uiVirtualKeyLow),
    XII_MEMBER_PROPERTY("VirtualKeyHigh", m_uiVirtualKeyHigh),
    XII_MEMBER_PROPERTY("Priority", m_uiPriority),
    XII_MEMBER_PROPERTY("Flags", m_uiFlags),
  } XII_END_PROPERTIES;
} XII_END_STATIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualShadowFeedbackResetPass, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowFeedbackResetPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Requests", m_Requests)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Capacity", m_uiCapacity)->AddAttributes(new xiiDefaultValueAttribute(8192U), new xiiClampValueAttribute(1U, 1048576U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Shadows") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiVirtualShadowFeedbackResetPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiCapacity && m_uiCapacity <= 1048576 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiVirtualShadowFeedbackResetPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!m_uiCapacity || m_uiCapacity > 1048576)
    return context.ReportFailure("Virtual shadow feedback capacity must be within 1..1048576 requests.");
  xiiRenderPassResourceScope scope(context);
  const auto                 pass = context.m_Graph.AddPass<xiiRenderGraphBufferHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](auto& data, xiiRenderGraphBuilder& builder) {
    xiiGALBufferCreationDescription description;
    description.m_uiSize = (m_uiCapacity + 1ULL) * sizeof(xiiGpuVirtualShadowFeedback);
    description.m_uiElementByteStride = sizeof(xiiGpuVirtualShadowFeedback);
    description.m_Mode = xiiGALBufferMode::Structured;
    description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    data = builder.WriteBuffer("Initialized Shadow Requests", description, xiiGALResourceStateFlags::CopyDestination); }, [](const auto& data, xiiRenderGraphPassContext& execute) {
    const xiiGpuVirtualShadowFeedback header;
    execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&header), sizeof(header))); });
  return context.SetOutput(m_Requests, *pass.first);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_VirtualShadowFeedbackResetPass);
