/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Utility/BufferExportPass.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBufferExportPass, 1, xiiRTTIDefaultAllocator<xiiBufferExportPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Utility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiBufferExportPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  input       = context.GetInputBuffer(m_Input);
  const auto* description = context.m_Graph.GetBufferDescription(input);
  if (!description || !description->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Buffer export requires an explicit shader-readable buffer.");
  const auto emitted = context.m_Graph.AddPass<xiiRenderGraphBufferHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
      data = builder.ReadBuffer(input, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportBuffer(data, xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
  return context.SetOutput(m_Output, *emitted.first);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_BufferExportPass);
