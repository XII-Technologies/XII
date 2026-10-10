/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>

xiiRenderPipelineInstance::~xiiRenderPipelineInstance() { Clear(); }
void xiiRenderPipelineInstance::Clear()
{
  for (auto* pass : m_Passes) pass->GetDynamicRTTI()->GetAllocator()->Deallocate(pass);
  m_Passes.Clear();
  m_Bindings.Clear();
  m_pPipeline = nullptr;
  m_Parameters.Clear();
  m_Overrides.Clear();
}
xiiResult xiiRenderPipelineInstance::Initialize(xiiSharedPtr<const xiiRenderPipeline> pipeline)
{
  if (pipeline == m_pPipeline && pipeline) return XII_SUCCESS;
  Clear();
  if (!pipeline) return XII_FAILURE;
  m_pPipeline = std::move(pipeline);
  for (xiiUInt32 i = 0; i < m_pPipeline->m_Passes.GetCount(); ++i)
  {
    xiiRenderPipelinePass* pass = m_pPipeline->m_Passes[i].m_pType->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    m_Passes.PushBack(pass);
    if (pass->Configure(m_pPipeline->m_Descriptor.m_Passes[i].m_Properties).Failed())
    {
      Clear();
      return XII_FAILURE;
    }
  }
  m_Bindings.SetCount(m_pPipeline->m_uiBindingCount);
  m_Parameters = m_pPipeline->m_Descriptor.m_Parameters;
  return XII_SUCCESS;
}
xiiResult xiiRenderPipelineInstance::Synchronize(const xiiRenderPipelineResourceHandle& resource, bool* out_changed)
{
  if (out_changed) *out_changed = false;
  if (!resource.IsValid())
  {
    if (out_changed) *out_changed = m_pPipeline != nullptr;
    Clear();
    return XII_FAILURE;
  }
  xiiResourceLock<xiiRenderPipelineResource> acquired(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  auto                                       pipeline = acquired.IsValid() ? acquired->GetPipeline() : xiiSharedPtr<const xiiRenderPipeline>();
  if (out_changed) *out_changed = pipeline != m_pPipeline;
  return Initialize(std::move(pipeline));
}
xiiResult xiiRenderPipelineInstance::Build(xiiView* view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiVariantDictionary& overrides)
{
  if (!m_pPipeline) return XII_FAILURE;
  if (m_Overrides != overrides)
  {
    m_Overrides  = overrides;
    m_Parameters = m_pPipeline->m_Descriptor.m_Parameters;
    for (auto it = overrides.GetIterator(); it.IsValid(); ++it) m_Parameters.Insert(it.Key(), it.Value());
  }
  for (auto& binding : m_Bindings) binding = {};
  for (auto index : m_pPipeline->m_Order)
  {
    const auto& pass = m_pPipeline->m_Passes[index];
    for (xiiUInt32 i = 0; i < pass.m_uiConnectionCount; ++i)
    {
      const auto& connection                   = m_pPipeline->m_Connections[pass.m_uiConnectionOffset + i];
      m_Bindings[connection.m_uiTargetBinding] = m_Bindings[connection.m_uiSourceBinding];
    }
    xiiRenderPipelinePassContext context{view, graph, blackboard, m_Parameters, pass.m_Pins, m_Bindings.GetArrayPtr().GetSubArray(pass.m_uiBindingOffset, pass.m_Pins.GetCount()), m_pPipeline->m_Descriptor.m_Passes[index].m_sName};
    auto                         validateBindings = [&](bool inputs) -> xiiResult {
      for (xiiUInt32 i = 0; i < pass.m_Pins.GetCount(); ++i)
      {
        const auto& pin = pass.m_Pins[i];
        if (pin.m_bInput != inputs) continue;
        const auto& binding = context.m_Bindings[i];
        if (pin.m_Type == xiiRenderPipelinePin::Texture)
        {
          const auto* description = graph.GetTextureDescription(binding.m_hTexture);
          if ((binding.m_hTexture.IsValid() || !pin.m_bOptional) &&
              (!description || (pin.m_Format != xiiGALResourceFormat::Unknown && description->m_Format != pin.m_Format))) return XII_FAILURE;
        }
        else if (pin.m_Type == xiiRenderPipelinePin::Buffer &&
                 (binding.m_hBuffer.IsValid() || !pin.m_bOptional) && !graph.GetBufferDescription(binding.m_hBuffer))
          return XII_FAILURE;
      }
      return XII_SUCCESS;
    };
    XII_SUCCEED_OR_RETURN(validateBindings(true));
    XII_SUCCEED_OR_RETURN(m_Passes[index]->Build(context));
    XII_SUCCEED_OR_RETURN(validateBindings(false));
  }
  return XII_SUCCESS;
}
