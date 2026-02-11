#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/RenderPipelineNode.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>
#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Fence.h>

class xiiView;
class xiiFrustum;
class xiiDGMLGraph;
class xiiRasterizerView;
class xiiFrameDataProviderBase;
class xiiRenderPipelinePassBase;

class XII_GRAPHICSCORE_DLL xiiRenderPipeline : public xiiRefCounted
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderPipeline);

public:
  enum class PipelineState
  {
    Uninitialized,
    RebuildError,
    Initialized
  };

  xiiRenderPipeline();
  ~xiiRenderPipeline();

  void                       AddPass(xiiUniquePtr<xiiRenderPipelinePassBase>&& pPass);
  void                       RemovePass(xiiRenderPipelinePassBase* pPass);
  void                       GetPasses(xiiDynamicArray<const xiiRenderPipelinePassBase*>& ref_passes) const;
  void                       GetPasses(xiiDynamicArray<xiiRenderPipelinePassBase*>& ref_passes);
  xiiRenderPipelinePassBase* GetPassByName(const xiiStringView& sPassName);

  bool Connect(xiiRenderPipelinePassBase* pOutputNode, xiiStringView sOutputPinName, xiiRenderPipelinePassBase* pInputNode, xiiStringView sInputPinName);
  bool Connect(xiiRenderPipelinePassBase* pOutputNode, xiiHashedString sOutputPinName, xiiRenderPipelinePassBase* pInputNode, xiiHashedString sInputPinName);
  bool Disconnect(xiiRenderPipelinePassBase* pOutputNode, xiiHashedString sOutputPinName, xiiRenderPipelinePassBase* pInputNode, xiiHashedString sInputPinName);

  const xiiRenderPipelinePassConnection* GetInputConnection(const xiiRenderPipelinePassBase* pPass, xiiHashedString sInputPinName) const;
  const xiiRenderPipelinePassConnection* GetOutputConnection(const xiiRenderPipelinePassBase* pPass, xiiHashedString sOutputPinName) const;

  void          AddExtractor(xiiUniquePtr<xiiExtractor>&& pExtractor);
  void          RemoveExtractor(xiiExtractor* pExtractor);
  void          GetExtractors(xiiDynamicArray<const xiiExtractor*>& ref_extractors) const;
  void          GetExtractors(xiiDynamicArray<xiiExtractor*>& ref_extractors);
  xiiExtractor* GetExtractorByName(const xiiStringView& sExtractorName);

  const xiiExtractedRenderData& GetRenderData() const;
  xiiRenderDataBatchList        GetRenderDataBatchesWithCategory(xiiRenderData::Category category) const;

  using RenderDataProcessor = xiiDelegate<void(xiiExtractedRenderData&)>;
  xiiUInt32 AddRenderDataProcessor(RenderDataProcessor processor);

  /// \brief Creates a DGML graph of all passes and resources. Can be used to verify that no accidental temporary resources are created due to poorly constructed pipelines or errors in code.
  void CreateDgmlGraph(xiiDGMLGraph& ref_graph);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  static xiiCVarBool cvar_SpatialCullingVis;
#endif

public:
  XII_ALWAYS_INLINE xiiHashedString GetViewName() const { return m_sName; }

  template <typename T>
  XII_ALWAYS_INLINE T* GetFrameDataProvider() const { return static_cast<T*>(GetFrameDataProvider(xiiGetStaticRTTI<T>())); }

private:
  friend class xiiRenderWorld;
  friend class xiiView;

  // \brief Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild(const xiiView& view);
  xiiResult     RebuildInternal(const xiiView& view);
  xiiResult     SortPasses();
  xiiResult     InitializePassResourceDescriptions(const xiiView& view);
  xiiResult     CreatePassResourceUsage(const xiiView& view);
  xiiResult     InitializeRenderPipelinePasses(const xiiView& view);
  xiiResult     BuildExecutionPlan(const xiiView& view);
  void          SortExtractors();
  void          UpdateViewData(const xiiView& view, xiiUInt32 uiDataIndex);

  // Modern in-place render-graph authoring API (replaces node-pin authoring)
public:
  using RGResourceId = xiiUInt32;
  using RGPassId = xiiUInt32;

  RGResourceId CreateResource(xiiStringView sName);
  RGPassId     CreatePass(xiiStringView sName, xiiEnum<xiiGALCommandQueueFlags> queue = xiiGALCommandQueueFlags::Graphics);
  void         AddPassInput(RGPassId pass, RGResourceId resource);
  void         AddPassOutput(RGPassId pass, RGResourceId resource);

  // Callback type that mirrors existing pass Execute signature. Optional: pass implementations can still derive xiiRenderPipelinePassBase.
  using RenderPassCallback = xiiDelegate<void(const xiiRenderViewContext&, const xiiArrayPtr<xiiRenderPipelinePassConnection* const>, const xiiArrayPtr<xiiRenderPipelinePassConnection* const>)>;
  void SetPassCallback(RGPassId pass, RenderPassCallback callback);

private:
  struct RGResourceDesc
  {
    xiiString m_sName;
  };

  struct RGPassDesc
  {
    xiiString                                    m_sName;
    xiiEnum<xiiGALCommandQueueFlags>             m_Queue = xiiGALCommandQueueFlags::Graphics;
    xiiDynamicArray<RGResourceId>                m_Inputs;
    xiiDynamicArray<RGResourceId>                m_Outputs;
    RenderPassCallback                           m_Callback;
    xiiRenderPipelinePassBase*                   m_pLegacyImpl = nullptr; // optional bridge
  };

  xiiDynamicArray<RGResourceDesc> m_RGResources;
  xiiDynamicArray<RGPassDesc>     m_RGPasses;

  void RemoveConnections(xiiRenderPipelinePassBase* pPass);
  void ClearRenderPassGraphResources();
  bool AreInputDescriptionsAvailable(const xiiRenderPipelinePassBase* pPass, const xiiHybridArray<xiiRenderPipelinePassBase*, 32>& done) const;
  bool ArePassThroughInputsDone(const xiiRenderPipelinePassBase* pPass, const xiiHybridArray<xiiRenderPipelinePassBase*, 32>& done) const;

  xiiFrameDataProviderBase* GetFrameDataProvider(const xiiRTTI* pRtti) const;

  void ExtractData(const xiiView& view);
  void FindVisibleObjects(const xiiView& view);

  void Render(xiiRenderContext* pRenderContext);

  xiiRasterizerView* PrepareOcclusionCulling(const xiiFrustum& frustum, const xiiView& view);
  void               PreviewOcclusionBuffer(const xiiRasterizerView& rasterizer, const xiiView& view);

private: // Member data
  // Thread data
  xiiThreadID m_CurrentExtractThread;
  xiiThreadID m_CurrentRenderThread;

  // Pipeline render data
  xiiExtractedRenderData                m_Data[2];
  xiiDynamicArray<const xiiGameObject*> m_VisibleObjects;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiTime m_AverageCullingTime;
#endif

  xiiHashedString m_sName;
  xiiUInt64       m_uiLastExtractionFrame;
  xiiUInt64       m_uiLastRenderFrame;

  // Render pass graph data
  PipelineState m_PipelineState = PipelineState::Uninitialized;

  struct ConnectionData
  {
    // Inputs / outputs match the node pin indices. Value at index is nullptr if not connected.
    xiiDynamicArray<xiiRenderPipelinePassConnection*> m_Inputs;
    xiiDynamicArray<xiiRenderPipelinePassConnection*> m_Outputs;
  };
  xiiDynamicArray<xiiUniquePtr<xiiRenderPipelinePassBase>> m_Passes;      ///< The passes present in the pipeline in no particular order.
  xiiMap<const xiiRenderPipelinePassBase*, ConnectionData> m_Connections; ///< The connections in each pass.

  /// \brief Contains all connections that share the same path-through resource and their first and last usage pass index.
  struct ResourceUsageData
  {
    xiiHybridArray<xiiRenderPipelinePassConnection*, 4> m_UsedBy;                      ///< All the connections that use this resource. Due to passthrough pins, this can be larger than 1.
    xiiUInt32                                           m_uiFirstUsageIdx;             ///< Used to decide when to acquire a temporary resource.
    xiiUInt32                                           m_uiLastUsageIdx;              ///< Used to decide when to return a temporary resource.
    const xiiRenderPipelineNodePin*                     m_pResourceProvider = nullptr; ///< If set, this node and parent pass provide an external resource to the pipeline. This could be a render target from a xiiTargetPass or a history buffer that is preserved across frames. At the start of every frame the parent pass will be asked for the current value of the resource a this pin.
  };
  xiiDynamicArray<ResourceUsageData> m_ResourceUsage;                      ///< All unique resources used during the pipeline run.
  xiiDynamicArray<xiiUInt32>         m_ResourceUsageIdxSortedByFirstUsage; ///< Indices map into m_ResourceUsage.
  xiiDynamicArray<xiiUInt32>         m_ResourceUsageIdxSortedByLastUsage;  ///< Indices map into m_ResourceUsage.

  xiiHashTable<xiiRenderPipelinePassConnection*, xiiUInt32> m_ConnectionToResourceIndex;

  // Compiled execution plan: a compact, immutable representation created during Rebuild/Compile.
  struct CompiledPass
  {
    xiiUInt32                         m_uiPassIndex = xiiInvalidIndex; // index into m_Passes after sorting
    xiiEnum<xiiGALCommandQueueFlags>  m_QueueFlags;                       // which queue this pass prefers (graphics/compute/transfer)
    xiiDynamicArray<xiiUInt32>        m_ResourceIndices;                  // resource indices used by this pass (indices into m_ResourceUsage)
    xiiDynamicArray<xiiGALStateTransitionDescription> m_Barriers; // barriers to apply before executing this pass
    // Additional fields (pipeline keys, descriptor set indices, barriers) can be added during compilation.
  };

  struct QueueSynchronization
  {
    xiiUInt32                        m_uiProducerPassIdx; // pass that produces / last-writes a resource
    xiiUInt32                        m_uiConsumerPassIdx; // pass that consumes / first-reads the resource on another queue
    xiiEnum<xiiGALCommandQueueFlags> m_ProducerQueue;      // queue type of producer
    xiiEnum<xiiGALCommandQueueFlags> m_ConsumerQueue;      // queue type of consumer
    xiiUInt32                        m_uiFenceIdx = xiiInvalidIndex; // index into m_QueueSyncFences
  };

  xiiDynamicArray<CompiledPass>       m_CompiledPasses;       ///< Compiled, ordered passes used for execution.
  xiiDynamicArray<QueueSynchronization> m_QueueSynchronizations; ///< Explicit cross-queue synchronization points.
  xiiDynamicArray<xiiSharedPtr<xiiGALFence>> m_QueueSyncFences;    ///< Fence objects used for cross-queue sync
  xiiDynamicArray<xiiUInt64>                 m_QueueSyncValues;    ///< Recorded fence values after submission

  // Extractors
  xiiDynamicArray<xiiUniquePtr<xiiExtractor>> m_Extractors;
  xiiDynamicArray<xiiUniquePtr<xiiExtractor>> m_SortedExtractors;

  // Data Providers
  mutable xiiDynamicArray<xiiUniquePtr<xiiFrameDataProviderBase>> m_DataProviders;
  mutable xiiHashTable<const xiiRTTI*, xiiUInt32>                 m_TypeToDataProviderIndex;

  // Processors
  xiiDynamicArray<RenderDataProcessor> m_RenderDataProcessors;

  xiiDynamicArray<xiiGALPermutationVariable> m_PermutationVariables;
};
