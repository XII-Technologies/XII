#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/RenderPipelineNode.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

class xiiView;
class xiiFrustum;
class xiiDGMLGraph;
class xiiRasterizerView;
class xiiFrameDataProviderBase;
class xiiRenderPipelinePassBase;

struct xiiGALPermutationVariable;

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
  void          SortExtractors();
  void          UpdateViewData(const xiiView& view, xiiUInt32 uiDataIndex);

  void RemoveConnections(xiiRenderPipelinePassBase* pPass);
  void ClearRenderPassGraphResources();
  bool AreInputDescriptionsAvailable(const xiiRenderPipelinePassBase* pPass, const xiiHybridArray<xiiRenderPipelinePassBase*, 32>& done) const;
  bool ArePassThroughInputsDone(const xiiRenderPipelinePassBase* pPass, const xiiHybridArray<xiiRenderPipelinePassBase*, 32>& done) const;

  xiiFrameDataProviderBase* GetFrameDataProvider(const xiiRTTI* pRtti) const;

  void ExtractData(const xiiView& view);
  void FindVisibleObjects(const xiiView& view);

  void Render();

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
    xiiUInt16                                           m_uiFirstUsageIdx;             ///< Used to decide when to acquire a temporary resource.
    xiiUInt16                                           m_uiLastUsageIdx;              ///< Used to decide when to return a temporary resource.
    const xiiRenderPipelineNodePin*                     m_pResourceProvider = nullptr; ///< If set, this node and parent pass provide an external resource to the pipeline. This could be a render target from a xiiTargetPass or a history buffer that is preserved across frames. At the start of every frame the parent pass will be asked for the current value of the resource a this pin.
  };
  xiiDynamicArray<ResourceUsageData> m_ResourceUsage;                      ///< All unique resources used during the pipeline run.
  xiiDynamicArray<xiiUInt16>         m_ResourceUsageIdxSortedByFirstUsage; ///< Indices map into m_ResourceUsage.
  xiiDynamicArray<xiiUInt16>         m_ResourceUsageIdxSortedByLastUsage;  ///< Indices map into m_ResourceUsage.

  xiiHashTable<xiiRenderPipelinePassConnection*, xiiUInt32> m_ConnectionToResourceIndex;

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
