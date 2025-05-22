#pragma once

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

class xiiProfilingId;
class xiiView;
class xiiRenderPipelinePass;
class xiiFrameDataProviderBase;
struct xiiPermutationVar;
class xiiDGMLGraph;
class xiiFrustum;
class xiiRasterizerView;

class XII_GRAPHICSCORE_DLL xiiRenderPipeline : public xiiRefCounted
{
public:
  enum class PipelineState
  {
    Uninitialized,
    RebuildError,
    Initialized
  };

  xiiRenderPipeline();
  ~xiiRenderPipeline();

  void                   AddPass(xiiUniquePtr<xiiRenderPipelinePass>&& pPass);
  void                   RemovePass(xiiRenderPipelinePass* pPass);
  void                   GetPasses(xiiDynamicArray<const xiiRenderPipelinePass*>& ref_passes) const;
  void                   GetPasses(xiiDynamicArray<xiiRenderPipelinePass*>& ref_passes);
  xiiRenderPipelinePass* GetPassByName(const xiiStringView& sPassName);
  xiiHashedString        GetViewName() const;

  bool Connect(xiiRenderPipelinePass* pOutputNode, xiiStringView sOutputPinName, xiiRenderPipelinePass* pInputNode, xiiStringView sInputPinName);
  bool Connect(xiiRenderPipelinePass* pOutputNode, xiiHashedString sOutputPinName, xiiRenderPipelinePass* pInputNode, xiiHashedString sInputPinName);
  bool Disconnect(xiiRenderPipelinePass* pOutputNode, xiiHashedString sOutputPinName, xiiRenderPipelinePass* pInputNode, xiiHashedString sInputPinName);

  const xiiRenderPipelinePassConnection* GetInputConnection(const xiiRenderPipelinePass* pPass, xiiHashedString sInputPinName) const;
  const xiiRenderPipelinePassConnection* GetOutputConnection(const xiiRenderPipelinePass* pPass, xiiHashedString sOutputPinName) const;

  void          AddExtractor(xiiUniquePtr<xiiExtractor>&& pExtractor);
  void          RemoveExtractor(xiiExtractor* pExtractor);
  void          GetExtractors(xiiDynamicArray<const xiiExtractor*>& ref_extractors) const;
  void          GetExtractors(xiiDynamicArray<xiiExtractor*>& ref_extractors);
  xiiExtractor* GetExtractorByName(const xiiStringView& sExtractorName);

  template <typename T>
  XII_ALWAYS_INLINE T* GetFrameDataProvider() const
  {
    return static_cast<T*>(GetFrameDataProvider(xiiGetStaticRTTI<T>()));
  }

  const xiiExtractedRenderData& GetRenderData() const;
  xiiRenderDataBatchList        GetRenderDataBatchesWithCategory(xiiRenderData::Category category, xiiRenderDataBatch::Filter filter = xiiRenderDataBatch::Filter()) const;

  /// \brief Creates a DGML graph of all passes and textures. Can be used to verify that no accidental temp textures are created due to poorly constructed pipelines or errors in code.
  void CreateDgmlGraph(xiiDGMLGraph& ref_graph);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  static xiiCVarBool cvar_SpatialCullingVis;
#endif

  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderPipeline);

private:
  friend class xiiRenderWorld;
  friend class xiiView;

  // \brief Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild(const xiiView& view);
  bool          RebuildInternal(const xiiView& view);
  bool          SortPasses();
  bool          InitRenderTargetDescriptions(const xiiView& view);
  bool          CreateRenderTargetUsage(const xiiView& view);
  bool          InitRenderPipelinePasses();
  void          SortExtractors();
  void          UpdateViewData(const xiiView& view, xiiUInt32 uiDataIndex);

  void RemoveConnections(xiiRenderPipelinePass* pPass);
  void ClearRenderPassGraphTextures();
  bool AreInputDescriptionsAvailable(const xiiRenderPipelinePass* pPass, const xiiHybridArray<xiiRenderPipelinePass*, 32>& done) const;
  bool ArePassThroughInputsDone(const xiiRenderPipelinePass* pPass, const xiiHybridArray<xiiRenderPipelinePass*, 32>& done) const;

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
  xiiDynamicArray<xiiUniquePtr<xiiRenderPipelinePass>> m_Passes;      ///< The passes present in the pipeline in no particular order.
  xiiMap<const xiiRenderPipelinePass*, ConnectionData> m_Connections; ///< The passes present in the pipeline in no particular order.

  /// \brief Contains all connections that share the same path-through texture and their first and last usage pass index.
  struct TextureUsageData
  {
    xiiHybridArray<xiiRenderPipelinePassConnection*, 4> m_UsedBy;                     ///< All the connections that use this texture. Due to passthrough pins, this can be larger than 1.
    xiiUInt16                                           m_uiFirstUsageIdx;            ///< Used to decide when to acquire a temp texture.
    xiiUInt16                                           m_uiLastUsageIdx;             ///< Used to decide when to return a temp texture.
    const xiiRenderPipelineNodePin*                     m_pTextureProvider = nullptr; ///< If set, this node and parent pass provide an external texture to the pipeline. This could be a render target from a xiiTargetPass or a history buffer that is preserved across frames. At the start of every frame the parent pass will be asked for the current value of the texture a this pin.
  };
  xiiDynamicArray<TextureUsageData> m_TextureUsage;                      ///< All unique textures used during the pipeline run.
  xiiDynamicArray<xiiUInt16>        m_TextureUsageIdxSortedByFirstUsage; ///< Indices map into m_TextureUsage
  xiiDynamicArray<xiiUInt16>        m_TextureUsageIdxSortedByLastUsage;  ///< Indices map into m_TextureUsage

  xiiHashTable<xiiRenderPipelinePassConnection*, xiiUInt32> m_ConnectionToTextureIndex;

  // Extractors
  xiiDynamicArray<xiiUniquePtr<xiiExtractor>> m_Extractors;
  xiiDynamicArray<xiiUniquePtr<xiiExtractor>> m_SortedExtractors;

  // Data Providers
  mutable xiiDynamicArray<xiiUniquePtr<xiiFrameDataProviderBase>> m_DataProviders;
  mutable xiiHashTable<const xiiRTTI*, xiiUInt32>                 m_TypeToDataProviderIndex;

  xiiDynamicArray<xiiPermutationVar> m_PermutationVars;

  // Occlusion Culling
  xiiSharedPtr<xiiGALTexture> m_pOcclusionDebugViewTexture;
};
