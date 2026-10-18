/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

/// An explicitly supplied root resource input. Resolve its interface index again after a compilation changes.
/// Graph handles are frame-local values and are never serialized identities.
struct xiiRenderPipelineInputBinding
{
  xiiRenderPipelineInputBinding(const xiiRenderGraph& graph, xiiUInt32 interfaceIndex, xiiRenderGraphTextureHandle texture) :
    m_uiInterface(interfaceIndex), m_uiSetupSerial(graph.GetSetupSerial()) { m_Binding.m_hTexture = texture; }
  xiiRenderPipelineInputBinding(const xiiRenderGraph& graph, xiiUInt32 interfaceIndex, xiiRenderGraphBufferHandle buffer) :
    m_uiInterface(interfaceIndex), m_uiSetupSerial(graph.GetSetupSerial()) { m_Binding.m_hBuffer = buffer; }
  xiiRenderPipelineInputBinding(const xiiRenderGraph& graph, xiiUInt32 interfaceIndex, const xiiRenderPipelineBinding& binding) :
    m_uiInterface(interfaceIndex), m_Binding(binding), m_uiSetupSerial(graph.GetSetupSerial()) {}
  xiiUInt32                m_uiInterface;
  xiiRenderPipelineBinding m_Binding;
  xiiUInt64                m_uiSetupSerial;
};

/// View-local pass objects and bindings. Synchronize only at a frame boundary, after previous CPU
/// graph execution. Resource changes replace immutable compilations; existing snapshots stay valid.
/// A failed replacement drops stale executable state. No graph traversal occurs during Build.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineInstance
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderPipelineInstance);

public:
  xiiRenderPipelineInstance();
  ~xiiRenderPipelineInstance();
  xiiResult Synchronize(const xiiRenderPipelineResourceHandle& resource, bool* out_changed = nullptr);
  xiiResult Initialize(xiiSharedPtr<const xiiRenderPipeline> pipeline);
  /// Publish explicit previous-frame resources before current-frame inputs are available.
  /// Repeated calls within one setup are idempotent; changes to context/configuration require a new setup.
  xiiResult                       BeginFrame(xiiView* view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiVariantDictionary& overrides = {});
  xiiResult                       Build(xiiView* view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiVariantDictionary& overrides = {}, xiiArrayPtr<const xiiRenderPipelineInputBinding> inputs = {});
  const xiiRenderPipelineBinding* GetInterfaceBinding(xiiUInt32 index) const;
  void                            Clear();
  void                            InvalidateTemporalHistory();
  /// Frame-local binding inspection. Handles expire when the graph starts its next setup.
  const xiiRenderPipelineBinding* GetOutputBinding(xiiUInt32 passIndex, xiiStringView pin) const;
  const xiiRenderPipeline*        GetPipeline() const { return m_pPipeline.Borrow(); }
  xiiStringView                   GetLastBuildError() const { return m_sBuildError; }
  const xiiRenderPipelinePass*    GetPass(xiiUInt32 index) const { return index < m_Passes.GetCount() ? m_Passes[index] : nullptr; }

private:
  xiiUInt64                                                       m_uiInstanceScope = 0; // Runtime namespace only, never a serialized graph/document identity.
  xiiString                                                       m_sScopeParent;
  xiiDynamicArray<xiiString>                                      m_ExecutionScopes;
  xiiUInt64                                                       m_uiPreparedSetup = 0, m_uiBuiltSetup = 0;
  const xiiView*                                                  m_pPreparedView       = nullptr; // Compared only during the same setup, never owned.
  const xiiRenderGraphBlackboard*                                 m_pPreparedBlackboard = nullptr;
  xiiDynamicArray<xiiRenderPipelineBinding>                       m_HistoryBindings;
  xiiString                                                       m_sBuildError;
  xiiSharedPtr<const xiiRenderPipeline>                           m_pPipeline;
  xiiDynamicArray<xiiRenderPipelinePass*>                         m_Passes;
  xiiDynamicArray<xiiRenderPipelineBinding>                       m_Bindings;
  xiiVariantDictionary                                            m_Parameters;
  xiiVariantDictionary                                            m_Overrides;
  xiiDynamicArray<xiiVariantDictionary>                           m_PassParameters;
  xiiHashTable<xiiString, xiiUniquePtr<xiiRenderGraphBlackboard>> m_Blackboards;
  xiiDynamicArray<xiiRenderGraphBlackboard*>                      m_PassBlackboards;
};
