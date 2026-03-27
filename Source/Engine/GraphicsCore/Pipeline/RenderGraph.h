#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Texture.h>

#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>

class xiiGALDevice;
class xiiView;
struct xiiViewData;
class xiiRenderGraph;
class xiiRGBuilder;
class xiiRGPassContext;

// ============================================================================
//  Opaque Handle Types
// ============================================================================

/// \brief Opaque handle to a virtual texture resource declared in the render graph.
struct xiiRGTextureHandle
{
  static constexpr xiiUInt32 InvalidIndex = xiiInvalidIndex;

  xiiUInt32 m_uiIndex   = InvalidIndex; ///< Index into the graph's resource table.
  xiiUInt16 m_uiVersion = 0U;           ///< Write version — read dependencies track this.

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != InvalidIndex; }
  [[nodiscard]] XII_ALWAYS_INLINE bool operator==(const xiiRGTextureHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex && m_uiVersion == rhs.m_uiVersion; }
  [[nodiscard]] XII_ALWAYS_INLINE bool operator!=(const xiiRGTextureHandle& rhs) const { return !(*this == rhs); }
};

/// \brief Opaque handle to a virtual buffer resource declared in the render graph.
struct xiiRGBufferHandle
{
  static constexpr xiiUInt32 InvalidIndex = xiiInvalidIndex;

  xiiUInt32 m_uiIndex   = InvalidIndex;
  xiiUInt16 m_uiVersion = 0U;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != InvalidIndex; }
  [[nodiscard]] XII_ALWAYS_INLINE bool operator==(const xiiRGBufferHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex && m_uiVersion == rhs.m_uiVersion; }
  [[nodiscard]] XII_ALWAYS_INLINE bool operator!=(const xiiRGBufferHandle& rhs) const { return !(*this == rhs); }
};

/// \brief Opaque handle to a registered render pass.
struct xiiRGPassHandle
{
  static constexpr xiiUInt32 InvalidIndex = xiiInvalidIndex;

  xiiUInt32 m_uiIndex = InvalidIndex;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != InvalidIndex; }
};

// ============================================================================
//  Compile-time structures
// ============================================================================

/// \brief Describes a resource state barrier synthesized during compilation.
struct XII_GRAPHICSCORE_DLL xiiRGBarrierDesc
{
  xiiUInt32                             m_uiResourceIndex = xiiInvalidIndex; ///< Index into the resource table.
  bool                                  m_bIsTexture      = true;

  // Sub-resource range (textures only; for buffers these are ignored)
  xiiUInt32 m_uiFirstMipLevel   = 0U;
  xiiUInt32 m_uiMipLevelCount   = XII_GAL_REMAINING_MIP_LEVELS;
  xiiUInt32 m_uiFirstArraySlice = 0U;
  xiiUInt32 m_uiArraySliceCount = XII_GAL_REMAINING_ARRAY_SLICES;

  xiiBitflags<xiiGALResourceStateFlags> m_BeforeState = xiiGALResourceStateFlags::Unknown;
  xiiBitflags<xiiGALResourceStateFlags> m_AfterState  = xiiGALResourceStateFlags::Unknown;

  xiiEnum<xiiGALStateTransitionType>   m_TransitionType  = xiiGALStateTransitionType::Immediate;
  xiiEnum<xiiGALStateTransitionFlags>  m_TransitionFlags = xiiGALStateTransitionFlags::None;
};

/// \brief Represents a group of consecutive passes merged into a single native render pass.
///
/// Within a group, passes share the same set of render-target and depth-stencil attachments.
/// The GPU never resolves tiles between passes in the group, which is critical for performance
/// on tile-based architectures. All passes must be on the same queue index.
struct XII_GRAPHICSCORE_DLL xiiRGMergeGroup
{
  xiiHybridArray<xiiUInt32, 8>   m_PassIndices;             ///< Ordered pass indices belonging to this group.
  xiiSharedPtr<xiiGALRenderPass> m_pNativeRenderPass;
  xiiSharedPtr<xiiGALFramebuffer> m_pFramebuffer;
};

/// \brief A batch of passes submitted together to a single command queue.
///
/// The executor creates one command list per submission, records all passes, then submits
/// it to the target queue. Cross-queue dependencies are expressed via xiiGALFence signals
/// and device-side waits inlined into the command list before the first consuming pass.
struct XII_GRAPHICSCORE_DLL xiiRGQueueSubmission
{
  xiiUInt32                             m_uiQueueIndex; ///< 0=Graphics, 1=AsyncCompute, 2=AsyncTransfer.
  xiiBitflags<xiiGALCommandQueueFlags>  m_QueueFlags;
  xiiDynamicArray<xiiUInt32>            m_PassOrder;    ///< Ordered pass indices to execute.

  /// Fence this submission signals after all its work (for downstream queues to DeviceWaitForFence).
  xiiSharedPtr<xiiGALFence>            m_pSignalFence;
  xiiUInt64                            m_uiSignalValue = 0ULL;

  /// Fences this submission must DeviceWaitForFence on before recording any commands.
  xiiHybridArray<xiiSharedPtr<xiiGALFence>, 2> m_WaitFences;
  xiiHybridArray<xiiUInt64, 2>                 m_WaitValues;
};

/// \brief A fully compiled render pass ready for execution.
struct XII_GRAPHICSCORE_DLL xiiRGCompiledPass
{
  xiiHashedString                      m_sName;
  xiiUInt32                            m_uiPassIndex      = xiiInvalidIndex;
  xiiUInt32                            m_uiQueueIndex     = 0U;
  xiiUInt32                            m_uiMergeGroupIndex = xiiInvalidIndex; ///< xiiInvalidIndex = not merged.

  bool m_bHasSideEffects = false;
  bool m_bAllowMerge     = true;
  bool m_bIsCulled       = false;

  /// Indices into the master barrier array emitted BEFORE this pass (clear split-barrier ends + immediates).
  xiiHybridArray<xiiUInt32, 4> m_PreBarrierIndices;
  /// Indices into the master barrier array emitted AFTER this pass (split-barrier begins for future consumers).
  xiiHybridArray<xiiUInt32, 4> m_PostBarrierBeginIndices;

  /// Resource indices to acquire from xiiRenderGraphResourceCache at pass start.
  xiiHybridArray<xiiUInt32, 4> m_AcquireResourceIndices;
  /// Resource indices to return to xiiRenderGraphResourceCache after pass end.
  xiiHybridArray<xiiUInt32, 4> m_ReleaseResourceIndices;

  /// Topological dependency pass indices.
  xiiHybridArray<xiiUInt32, 4> m_DependencyPassIndices;

  /// Type-erased pointer to the pass data struct. Owned by the graph.
  void*                                m_pPassData = nullptr;
  /// Type-erased execute callback.
  xiiDelegate<void(xiiRGPassContext&)> m_ExecuteFunc;
};

// ============================================================================
//  Compile Settings + Statistics
// ============================================================================

/// \brief Controls optional features of the render graph compiler.
struct XII_GRAPHICSCORE_DLL xiiRGCompileSettings
{
  bool      m_bEnablePassCulling   = true;  ///< Remove passes not reachable from any side-effect pass.
  bool      m_bEnableCompileCache  = true;  ///< Skip recompilation when the graph signature is unchanged.
  bool      m_bEnableSplitBarriers = true;  ///< Use Begin/End split barriers to overlap transitions.
  bool      m_bEnableAsyncQueues   = true;  ///< Schedule async-compute/transfer passes on separate queues.
  bool      m_bEnableGPUProfiling  = false; ///< Emit Duration queries around each pass via the profiler.
  xiiUInt32 m_uiCacheSalt          = 0U;    ///< Invalidate the compile cache without changing the graph.
};

/// \brief Per-compile statistics for diagnostics and HUD display.
struct XII_GRAPHICSCORE_DLL xiiRGStatistics
{
  xiiUInt32 m_uiRegisteredPassCount   = 0U;
  xiiUInt32 m_uiCompiledPassCount     = 0U;  ///< Passes retained after culling.
  xiiUInt32 m_uiCulledPassCount       = 0U;
  xiiUInt32 m_uiTotalBarrierCount     = 0U;
  xiiUInt32 m_uiSplitBarrierCount     = 0U;
  xiiUInt32 m_uiMergeGroupCount       = 0U;
  xiiUInt32 m_uiQueueSubmissionCount  = 0U;
  xiiUInt32 m_uiTransientTextureCount = 0U;
  xiiUInt32 m_uiTransientBufferCount  = 0U;
  xiiUInt64 m_uiGraphSignature        = 0ULL;
  bool      m_bUsedCachedCompile      = false;
};

// ============================================================================
//  Pass Context
// ============================================================================

/// \brief Execution context passed to every pass's execute callback.
///
/// Provides access to resolved GPU resources, the command list, the blackboard,
/// the resource cache, and optional profiling / view data.
class XII_GRAPHICSCORE_DLL xiiRGPassContext
{
public:
  // Non-copyable — created per-pass by the executor.
  xiiRGPassContext(const xiiRGPassContext&) = delete;
  xiiRGPassContext& operator=(const xiiRGPassContext&) = delete;

  /// \brief Returns the command list for this pass to record GPU commands into.
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALCommandList& GetCommandList() const
  {
    XII_ASSERT_DEV(m_pCommandList != nullptr, "Command list is null.");
    return *m_pCommandList;
  }

  /// \brief Resolves a virtual texture handle to its actual GPU texture for this frame.
  [[nodiscard]] xiiGALTexture* GetTexture(xiiRGTextureHandle handle) const;

  /// \brief Resolves a virtual buffer handle to its actual GPU buffer for this frame.
  [[nodiscard]] xiiGALBuffer* GetBuffer(xiiRGBufferHandle handle) const;

  /// \brief Returns the per-frame blackboard for typed inter-pass data exchange.
  [[nodiscard]] XII_ALWAYS_INLINE xiiRenderGraphBlackboard& GetBlackboard() const
  {
    XII_ASSERT_DEV(m_pBlackboard != nullptr, "Blackboard is null.");
    return *m_pBlackboard;
  }

  /// \brief Returns the resource cache for transient GPU resource allocation.
  [[nodiscard]] XII_ALWAYS_INLINE xiiRenderGraphResourceCache& GetResourceCache() const
  {
    XII_ASSERT_DEV(m_pResourceCache != nullptr, "ResourceCache is null.");
    return *m_pResourceCache;
  }

  /// \brief Returns the current view (may be null for headless passes).
  [[nodiscard]] XII_ALWAYS_INLINE const xiiView* GetView() const { return m_pView; }

  /// \brief Returns the frame index for the current execution.
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64 GetFrameIndex() const { return m_uiFrameIndex; }

  /// \brief Returns the pass name for labeling / assertions.
  [[nodiscard]] XII_ALWAYS_INLINE xiiHashedString GetPassName() const { return m_sPassName; }

private:
  friend class xiiRenderGraph; // Only the graph executor may construct this.

  xiiRGPassContext() = default;

  xiiGALCommandList*         m_pCommandList  = nullptr;
  xiiRenderGraphBlackboard*  m_pBlackboard   = nullptr;
  xiiRenderGraphResourceCache* m_pResourceCache = nullptr;
  const xiiView*             m_pView         = nullptr;
  xiiUInt64                  m_uiFrameIndex  = 0ULL;
  xiiHashedString            m_sPassName;

  // Resource resolution tables — owned by the graph, borrowed for pass duration.
  xiiArrayPtr<xiiSharedPtr<xiiGALTexture>> m_ResolvedTextures;
  xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>>  m_ResolvedBuffers;
};

// ============================================================================
//  Builder
// ============================================================================

/// \brief Declarative API used inside a pass's setup callback to declare resource usage.
///
/// Each call to Read* or Write* records a dependency edge in the graph's dependency
/// table and advances the resource version on writes. The builder may only be used
/// within the setup callback passed to xiiRenderGraph::AddPass().
class XII_GRAPHICSCORE_DLL xiiRGBuilder
{
public:
  // Non-copyable — tied to a specific graph and pass index.
  xiiRGBuilder(const xiiRGBuilder&) = delete;
  xiiRGBuilder& operator=(const xiiRGBuilder&) = delete;

  // ── Texture declarations ────────────────────────────────────────────────

  /// \brief Declares a new transient texture resource owned by the graph.
  ///        Returns a handle pointing to version 0 (unwritten). Normally
  ///        followed immediately by WriteTexture() to register the first write.
  [[nodiscard]] xiiRGTextureHandle DeclareTexture(xiiHashedString sName, const xiiGALTextureCreationDescription& desc);

  /// \brief Imports an externally-owned texture as a read-only graph resource.
  [[nodiscard]] xiiRGTextureHandle ImportTexture(xiiHashedString sName, xiiSharedPtr<xiiGALTexture> pTexture,
    xiiBitflags<xiiGALResourceStateFlags> currentState);

  /// \brief Declares a read dependency on the given texture at its current version.
  ///        Creates a dependency edge: this pass depends on the last writer.
  [[nodiscard]] xiiRGTextureHandle ReadTexture(xiiRGTextureHandle handle, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// \brief Declares a write to the given texture, bumping its version.
  ///        Returns the new versioned handle — store this, not the input handle.
  [[nodiscard]] xiiRGTextureHandle WriteTexture(xiiRGTextureHandle handle, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// \brief Shorthand: declare transient texture AND register first write in one call.
  [[nodiscard]] xiiRGTextureHandle WriteTexture(xiiHashedString sName, const xiiGALTextureCreationDescription& desc,
    xiiBitflags<xiiGALResourceStateFlags> requiredState);

  // ── Buffer declarations ─────────────────────────────────────────────────

  /// \brief Declares a new transient buffer resource owned by the graph.
  [[nodiscard]] xiiRGBufferHandle DeclareBuffer(xiiHashedString sName, const xiiGALBufferCreationDescription& desc);

  /// \brief Imports an externally-owned buffer into the graph.
  [[nodiscard]] xiiRGBufferHandle ImportBuffer(xiiHashedString sName, xiiSharedPtr<xiiGALBuffer> pBuffer,
    xiiBitflags<xiiGALResourceStateFlags> currentState);

  /// \brief Declares a read dependency on the buffer.
  [[nodiscard]] xiiRGBufferHandle ReadBuffer(xiiRGBufferHandle handle, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// \brief Declares a write to the buffer, bumping its version.
  [[nodiscard]] xiiRGBufferHandle WriteBuffer(xiiRGBufferHandle handle, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// \brief Shorthand: declare transient buffer AND register first write.
  [[nodiscard]] xiiRGBufferHandle WriteBuffer(xiiHashedString sName, const xiiGALBufferCreationDescription& desc,
    xiiBitflags<xiiGALResourceStateFlags> requiredState);

  // ── Pass-level flags ────────────────────────────────────────────────────

  /// \brief Marks this pass as having side effects that prevent it from being culled.
  ///        Call this for passes that write to swap-chain images, initiate readbacks, etc.
  void SetPassSideEffects(bool bHasSideEffects);

  /// \brief Controls whether this pass participates in render-pass merge groups.
  ///        Default is true. Set to false if the pass must stand alone (e.g. readback).
  void SetPassAllowMerge(bool bAllowMerge);

private:
  friend class xiiRenderGraph; // Only the graph may construct builders.

  xiiRGBuilder(xiiRenderGraph& graph, xiiUInt32 uiPassIndex);

  xiiRenderGraph& m_Graph;
  xiiUInt32       m_uiPassIndex;
};

// ============================================================================
//  xiiRenderGraph — main class
// ============================================================================

/// \brief Advanced high-performance render graph for the XII engine.
///
/// ## Usage per frame
/// \code
/// graph.BeginSetup(frameIndex);
///
/// auto [pData, hPass] = graph.AddPass<MyPassData>(
///   xiiHashedString("DepthPrepass"),
///   xiiGALCommandQueueFlags::Graphics,
///   [](MyPassData& data, xiiRGBuilder& builder)
///   {
///     data.hDepth = builder.WriteTexture("SceneDepth", depthDesc, xiiGALResourceStateFlags::DepthWrite);
///   },
///   [](const MyPassData& data, xiiRGPassContext& ctx)
///   {
///     ctx.GetCommandList().ClearDepthStencilView(...);
///   },
///   /*bHasSideEffects=*/false
/// );
/// pData->uiDrawCount = scene.GetDrawCount();
///
/// graph.EndSetup();
/// if (graph.Compile(settings).Succeeded())
///     graph.Execute(pDevice, pView, pBlackboard, pCache, pProfiler);
/// \endcode
///
/// ## Threading
/// - BeginSetup / AddPass / EndSetup: single-threaded (main thread).
/// - Compile: single-threaded.
/// - Execute: command list recording per pass is sequential per queue;
///   queues run genuinely in parallel on the GPU side.
/// - Pass execute callbacks may safely call xiiRenderGraphBlackboard::TryGet concurrently.
class XII_GRAPHICSCORE_DLL xiiRenderGraph
{
public:
  xiiRenderGraph();
  ~xiiRenderGraph();

  xiiRenderGraph(const xiiRenderGraph&) = delete;
  xiiRenderGraph& operator=(const xiiRenderGraph&) = delete;

  // ── Setup phase ─────────────────────────────────────────────────────────

  /// \brief Begins graph setup for the given frame index.
  ///        Clears all pass registrations and resource declarations from the previous frame.
  void BeginSetup(xiiUInt64 uiFrameIndex);

  /// \brief Registers a typed pass and immediately invokes its setup callback.
  ///
  /// \tparam TPassData  Plain data struct holding per-frame data and resource handles.
  ///                    Must be default-constructible. Lifetime is managed by the graph.
  ///
  /// \param sName         Unique name used for debugging, profiling, and the compile cache.
  /// \param queueFlags    Target queue (Graphics / Compute / Transfer).
  /// \param setupFunc     Called immediately to declare resource usage via xiiRGBuilder.
  /// \param executeFunc   Called during Execute() to record GPU commands.
  /// \param bHasSideEffects  If true, this pass is never culled even if no pass reads its outputs.
  ///
  /// \returns A pair of (raw pointer to TPassData, pass handle).
  ///          The pointer is stable for the lifetime of this frame's graph.
  template <typename TPassData>
  std::pair<TPassData*, xiiRGPassHandle> AddPass(
    xiiHashedString                                          sName,
    xiiBitflags<xiiGALCommandQueueFlags>                     queueFlags,
    xiiDelegate<void(TPassData&, xiiRGBuilder&)>             setupFunc,
    xiiDelegate<void(const TPassData&, xiiRGPassContext&)>   executeFunc,
    bool                                                     bHasSideEffects = false);

  /// \brief Finalizes the setup phase. Must be called after all AddPass calls.
  void EndSetup();

  // ── Compile phase ───────────────────────────────────────────────────────

  /// \brief Compiles the render graph.
  ///
  /// Runs all 7 compiler phases:
  ///   A. Resource versioning (already completed during AddPass setup callbacks).
  ///   B. Topological sort + live-pass culling.
  ///   C. Transient resource lifetime analysis.
  ///   D. Advanced barrier synthesis (sub-resource, UAV, split barriers).
  ///   E. Multi-queue scheduling → xiiRGQueueSubmission list with fence handshakes.
  ///   F. Render pass group merging → xiiRGMergeGroup list.
  ///   G. Signature hash + compile cache (skip if graph is unchanged).
  ///
  /// \param settings    Optional compile configuration.
  /// \param out_pError  Optional diagnostics string populated on XII_FAILURE.
  ///
  /// \returns XII_SUCCESS on success, XII_FAILURE if a cycle or invalid state is detected.
  [[nodiscard]] xiiResult Compile(const xiiRGCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);

  // ── Execute phase ───────────────────────────────────────────────────────

  /// \brief Executes the compiled render graph.
  ///
  /// For each xiiRGQueueSubmission:
  ///   1. Acquires the xiiGALCommandQueue via pDevice->GetCommandQueue(queueFlags).
  ///   2. Creates / resets a command list for that queue.
  ///   3. Emits DeviceWaitForFence calls for cross-queue dependencies.
  ///   4. Acquires transient resources from pResourceCache.
  ///   5. Emits pre-barriers (split-bar ends + immediates).
  ///   6. Opens a render pass group if the pass starts a merge group.
  ///   7. Calls pProfiler->OnPassBegin (if non-null).
  ///   8. Calls the pass execute callback.
  ///   9. Calls pProfiler->OnPassEnd (if non-null).
  ///  10. Closes the render pass group if the pass ends a merge group.
  ///  11. Emits post-barrier-begins (split-bar starts for downstream consumers).
  ///  12. Returns transient resources whose lifetime ends at this pass.
  ///  13. EnqueueSignals the cross-queue fence for downstream submissions.
  ///  14. Submits the command list.
  ///
  /// \param pDevice        Target GAL device (must remain valid for the call duration).
  /// \param pView          Optional view providing camera / viewport data to passes.
  /// \param pBlackboard    Per-frame data exchange store.
  /// \param pResourceCache Transient resource pool for this frame.
  /// \param pProfiler      Optional GPU timing profiler.
  /// \param out_pError     Optional diagnostics output.
  ///
  /// \returns XII_SUCCESS on success.
  [[nodiscard]] xiiResult Execute(
    xiiGALDevice*                pDevice,
    const xiiView*               pView,
    xiiRenderGraphBlackboard*    pBlackboard,
    xiiRenderGraphResourceCache* pResourceCache,
    xiiRenderGraphProfiler*      pProfiler  = nullptr,
    xiiStringBuilder*            out_pError = nullptr);

  // ── Query ────────────────────────────────────────────────────────────────

  [[nodiscard]] const xiiRGStatistics& GetStatistics() const { return m_Statistics; }

  [[nodiscard]] const xiiDynamicArray<xiiRGCompiledPass>&    GetCompiledPasses()     const { return m_CompiledPasses; }
  [[nodiscard]] const xiiDynamicArray<xiiRGBarrierDesc>&     GetBarriers()           const { return m_Barriers; }
  [[nodiscard]] const xiiDynamicArray<xiiRGMergeGroup>&      GetMergeGroups()        const { return m_MergeGroups; }
  [[nodiscard]] const xiiDynamicArray<xiiRGQueueSubmission>& GetQueueSubmissions()   const { return m_QueueSubmissions; }

  /// \brief Serialises the compiled graph to a DOT string for Graphviz visualisation.
  [[nodiscard]] xiiResult DumpToDot(xiiStringBuilder& out_sDot) const;

  /// \brief True if the graph has been compiled and not yet invalidated.
  [[nodiscard]] bool IsCompiled() const { return m_bIsCompiled; }

private:
  // ── Internal resource / pass entry types ─────────────────────────────────

  struct ResourceEntry
  {
    xiiHashedString                          m_sName;
    bool                                     m_bIsTexture      = true;
    bool                                     m_bIsImported     = false;
    bool                                     m_bIsTransient    = false; ///< True = graph-owned, allocated via ResourceCache.

    xiiGALTextureCreationDescription         m_TextureDesc;
    xiiGALBufferCreationDescription          m_BufferDesc;

    // Imported external resources (non-transient).
    xiiSharedPtr<xiiGALTexture>              m_pImportedTexture;
    xiiSharedPtr<xiiGALBuffer>              m_pImportedBuffer;
    xiiBitflags<xiiGALResourceStateFlags>    m_ImportedInitialState = xiiGALResourceStateFlags::Unknown;

    // Resolved for current frame execution (filled by executor).
    xiiSharedPtr<xiiGALTexture>              m_pResolvedTexture;
    xiiSharedPtr<xiiGALBuffer>              m_pResolvedBuffer;

    // Current write version (bumped on each write during setup).
    xiiUInt16                                m_uiCurrentVersion         = 0U;
    xiiUInt32                                m_uiCurrentProducerPassIdx = xiiInvalidIndex;

    // Lifetime (populated during Phase C).
    xiiUInt32                                m_uiFirstUsePassIdx = xiiInvalidIndex;
    xiiUInt32                                m_uiLastUsePassIdx  = xiiInvalidIndex;

    // Current state known at compile time (used for barrier synthesis).
    xiiBitflags<xiiGALResourceStateFlags>    m_CurrentState = xiiGALResourceStateFlags::Unknown;
  };

  struct ResourceUsage
  {
    xiiUInt32                             m_uiResourceIndex;
    bool                                  m_bIsTexture;
    xiiUInt16                             m_uiVersion;    ///< Version being read or written.
    xiiBitflags<xiiGALResourceStateFlags> m_RequiredState;
    bool                                  m_bIsWrite;
  };

  struct PassEntry
  {
    xiiHashedString                      m_sName;
    xiiBitflags<xiiGALCommandQueueFlags> m_QueueFlags;
    bool                                 m_bHasSideEffects = false;
    bool                                 m_bAllowMerge     = true;

    xiiHybridArray<ResourceUsage, 8>     m_Reads;
    xiiHybridArray<ResourceUsage, 8>     m_Writes;

    // Type-erased pass data + execute func (owned by this entry).
    void*                                m_pPassData          = nullptr;
    void (*m_pfnDestroyPassData)(void*)  = nullptr;
    xiiDelegate<void(xiiRGPassContext&)> m_ExecuteFunc;
  };

  // ── Compiler phase helpers ────────────────────────────────────────────────

  void PhaseB_TopologicalSortAndCull(const xiiRGCompileSettings& settings, xiiDynamicArray<xiiUInt32>& out_sortedIndices);
  void PhaseC_LifetimeAnalysis(const xiiDynamicArray<xiiUInt32>& sortedIndices);
  void PhaseD_BarrierSynthesis(const xiiDynamicArray<xiiUInt32>& sortedIndices, const xiiRGCompileSettings& settings);
  void PhaseE_MultiQueueScheduling(const xiiDynamicArray<xiiUInt32>& sortedIndices, xiiGALDevice* pDevice, const xiiRGCompileSettings& settings);
  void PhaseF_RenderPassMerging(xiiGALDevice* pDevice);
  void PhaseG_SignatureAndCache(const xiiRGCompileSettings& settings);

  void EmitBarrier(xiiUInt32 uiConsumerPassIdx, xiiUInt32 uiResourceIdx, bool bIsTexture,
    xiiBitflags<xiiGALResourceStateFlags> afterState, bool bSplitBarrier,
    xiiUInt32 uiFirstMip = 0U, xiiUInt32 uiMipCount = XII_GAL_REMAINING_MIP_LEVELS,
    xiiUInt32 uiFirstSlice = 0U, xiiUInt32 uiSliceCount = XII_GAL_REMAINING_ARRAY_SLICES);

  [[nodiscard]] static xiiBitflags<xiiGALResourceStateFlags> InferStateFromUsage(const ResourceUsage& usage);
  [[nodiscard]] static xiiUInt64 ComputeSignature(const xiiDynamicArray<PassEntry>& passes);

  // ── Private data ──────────────────────────────────────────────────────────

  xiiDynamicArray<PassEntry>     m_Passes;       ///< Setup-phase pass list, cleared each BeginSetup().
  xiiDynamicArray<ResourceEntry> m_Resources;    ///< Virtual resource table, cleared each BeginSetup().
  xiiHashTable<xiiHashedString, xiiUInt32> m_ResourceNameIndex; ///< Fast name → resource index lookup.

  // Compiled outputs
  xiiDynamicArray<xiiRGCompiledPass>    m_CompiledPasses;
  xiiDynamicArray<xiiRGBarrierDesc>     m_Barriers;
  xiiDynamicArray<xiiRGMergeGroup>      m_MergeGroups;
  xiiDynamicArray<xiiRGQueueSubmission> m_QueueSubmissions;

  xiiRGStatistics    m_Statistics;
  xiiRGCompileSettings m_LastCompileSettings;
  xiiUInt64          m_uiLastSignature = 0ULL;
  xiiUInt64          m_uiFrameIndex   = 0ULL;
  bool               m_bIsSetupOpen   = false;
  bool               m_bIsCompiled    = false;
};

// ============================================================================
//  AddPass — template implementation (must be in header)
// ============================================================================

template <typename TPassData>
std::pair<TPassData*, xiiRGPassHandle> xiiRenderGraph::AddPass(
  xiiHashedString                                          sName,
  xiiBitflags<xiiGALCommandQueueFlags>                     queueFlags,
  xiiDelegate<void(TPassData&, xiiRGBuilder&)>             setupFunc,
  xiiDelegate<void(const TPassData&, xiiRGPassContext&)>   executeFunc,
  bool                                                     bHasSideEffects)
{
  XII_ASSERT_DEV(m_bIsSetupOpen, "AddPass must be called between BeginSetup() and EndSetup().");
  XII_ASSERT_DEV(setupFunc.IsValid(), "Setup function must be valid.");
  XII_ASSERT_DEV(executeFunc.IsValid(), "Execute function must be valid.");

  const xiiUInt32 uiPassIndex = m_Passes.GetCount();

  PassEntry& entry           = m_Passes.ExpandAndGetRef();
  entry.m_sName              = sName;
  entry.m_QueueFlags         = queueFlags;
  entry.m_bHasSideEffects    = bHasSideEffects;
  entry.m_bAllowMerge        = true;

  // Allocate pass data on the heap for pointer stability.
  TPassData* pData            = new TPassData();
  entry.m_pPassData           = pData;
  entry.m_pfnDestroyPassData  = [](void* p) { delete static_cast<TPassData*>(p); };

  // Wrap typed execute func in a type-erased delegate.
  entry.m_ExecuteFunc = [executeFunc, pData](xiiRGPassContext& ctx) { executeFunc(*pData, ctx); };

  // Call setup func immediately — this populates m_Reads / m_Writes via the builder.
  xiiRGBuilder builder(*this, uiPassIndex);
  setupFunc(*pData, builder);

  m_bIsCompiled = false; // Invalidate any previous compile.

  xiiRGPassHandle handle;
  handle.m_uiIndex = uiPassIndex;
  return {pData, handle};
}

#include <GraphicsCore/Pipeline/Implementation/RenderGraph_inl.h>
