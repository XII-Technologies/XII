/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

/// View-local pass objects and bindings. Synchronize only at a frame boundary, after previous CPU
/// graph execution. Resource changes replace immutable compilations; existing snapshots stay valid.
/// A failed replacement drops stale executable state. No graph traversal occurs during Build.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineInstance
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderPipelineInstance);

public:
  xiiRenderPipelineInstance() = default;
  ~xiiRenderPipelineInstance();
  xiiResult                    Synchronize(const xiiRenderPipelineResourceHandle& resource, bool* out_changed = nullptr);
  xiiResult                    Initialize(xiiSharedPtr<const xiiRenderPipeline> pipeline);
  xiiResult                    Build(xiiView* view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiVariantDictionary& overrides = {});
  void                         Clear();
  void                         InvalidateTemporalHistory();
  const xiiRenderPipeline*     GetPipeline() const { return m_pPipeline.Borrow(); }
  const xiiRenderPipelinePass* GetPass(xiiUInt32 index) const { return index < m_Passes.GetCount() ? m_Passes[index] : nullptr; }

private:
  xiiDynamicArray<xiiRenderPipelineBinding>                       m_HistoryBindings;
  xiiSharedPtr<const xiiRenderPipeline>                           m_pPipeline;
  xiiDynamicArray<xiiRenderPipelinePass*>                         m_Passes;
  xiiDynamicArray<xiiRenderPipelineBinding>                       m_Bindings;
  xiiVariantDictionary                                            m_Parameters;
  xiiVariantDictionary                                            m_Overrides;
  xiiDynamicArray<xiiVariantDictionary>                           m_PassParameters;
  xiiHashTable<xiiString, xiiUniquePtr<xiiRenderGraphBlackboard>> m_Blackboards;
  xiiDynamicArray<xiiRenderGraphBlackboard*>                      m_PassBlackboards;
};
