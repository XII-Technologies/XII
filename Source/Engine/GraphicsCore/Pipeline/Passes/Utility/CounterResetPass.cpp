/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CounterResetPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCounterResetPass, 1, xiiRTTIDefaultAllocator<xiiCounterResetPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Counter", m_Counter)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Value", m_uiValue)->AddAttributes(new xiiDefaultValueAttribute(0U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Utility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiCounterResetPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Counter;
    xiiUInt32                  m_uiValue;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = sizeof(xiiUInt32);
      description.m_uiElementByteStride = sizeof(xiiUInt32);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
      data.m_Counter = builder.WriteBuffer("Counter", description, xiiGALResourceStateFlags::CopyDestination);
      data.m_uiValue = m_uiValue; }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Counter), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_uiValue), sizeof(data.m_uiValue))); });
  return context.SetOutput(m_Counter, emitted.first->m_Counter);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_CounterResetPass);
