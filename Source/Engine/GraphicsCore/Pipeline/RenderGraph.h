/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

class xiiView;
struct xiiViewData;
class xiiRenderGraph;

/// Opaque handle to a virtual texture resource declared in the render graph.
struct XII_GRAPHICSCORE_DLL xiiRGTextureHandle : public xiiHashableStruct<xiiRGTextureHandle>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndex   = xiiInvalidIndex; ///< Index into the graph's resource table.
  xiiUInt16 m_uiVersion = 0U;              ///< Write version - read dependencies track this.

  /// Returns whether this handle references a valid texture resource in the graph.
  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex; }

  [[nodiscard]] XII_ALWAYS_INLINE bool operator==(const xiiRGTextureHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex && m_uiVersion == rhs.m_uiVersion; }
};

/// Opaque handle to a virtual buffer resource declared in the render graph.
struct XII_GRAPHICSCORE_DLL xiiRGBufferHandle : public xiiHashableStruct<xiiRGBufferHandle>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndex   = xiiInvalidIndex; ///< Index into the graph's resource table.
  xiiUInt16 m_uiVersion = 0U;              ///< Write version - read dependencies track this.

  /// Returns whether this handle references a valid buffer resource in the graph.
  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex; }

  [[nodiscard]] XII_ALWAYS_INLINE bool operator==(const xiiRGBufferHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex && m_uiVersion == rhs.m_uiVersion; }
};

/// Opaque handle to a registered render pass.
struct XII_GRAPHICSCORE_DLL xiiRGPassHandle : public xiiHashableStruct<xiiRGPassHandle>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndex = xiiInvalidIndex; ///< Index into the graph's pass table.

  /// Returns whether this handle references a valid pass in the graph.
  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex; }
};

/// Describes a resource state barrier synthesized during compilation.
struct XII_GRAPHICSCORE_DLL xiiRGBarrierDescription : public xiiHashableStruct<xiiRGBarrierDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                             m_uiResourceIndex   = xiiInvalidIndex;                      ///< Index into the resource table.
  bool                                  m_bIsTexture        = true;                                 ///< Whether the barrier is for a texture or a buffer resource.
  xiiUInt32                             m_uiFirstMipLevel   = 0U;                                   ///< For textures, the first mip level affected by the barrier. For buffers, this is always 0.
  xiiUInt32                             m_uiMipLevelCount   = XII_GAL_REMAINING_MIP_LEVELS;         ///< For textures, the number of mip levels affected by the barrier. For buffers, this is always 1.
  xiiUInt32                             m_uiFirstArraySlice = 0U;                                   ///< For textures, the first array slice affected by the barrier. For buffers, this is always 0.
  xiiUInt32                             m_uiArraySliceCount = XII_GAL_REMAINING_ARRAY_SLICES;       ///< For textures, the number of array slices affected by the barrier. For buffers, this is always 1.
  xiiBitflags<xiiGALResourceStateFlags> m_BeforeState       = xiiGALResourceStateFlags::Unknown;    ///< The resource state before the barrier. This is used for validation and may be Unknown if the state is not known at compile time.
  xiiBitflags<xiiGALResourceStateFlags> m_AfterState        = xiiGALResourceStateFlags::Unknown;    ///< The resource state after the barrier. This is used for validation and may be Unknown if the state is not known at compile time.
  xiiEnum<xiiGALStateTransitionType>    m_TransitionType    = xiiGALStateTransitionType::Immediate; ///< Whether this barrier is an immediate transition or a split barrier begin/end.
  xiiEnum<xiiGALStateTransitionFlags>   m_TransitionFlags   = xiiGALStateTransitionFlags::None;     ///< Additional flags for the barrier, such as whether to discard content or update internal resource state. Only relevant for immediate barriers and split-barrier ends.
};

/// Represents a group of consecutive passes merged into a single native render pass.
///
/// Within a group, passes share the same set of render-target and depth-stencil attachments.
/// The GPU never resolves tiles between passes in the group, which is critical for performance on tile-based architectures.
/// All passes must be on the same queue index.
struct XII_GRAPHICSCORE_DLL xiiRGMergeGroup
{
  xiiHybridArray<xiiUInt32, 8>    m_PassIndices; ///< Ordered pass indices belonging to this group.
  xiiSharedPtr<xiiGALRenderPass>  m_pNativeRenderPass;
  xiiSharedPtr<xiiGALFramebuffer> m_pFramebuffer;
};

/// A batch of passes submitted together to a single command queue.
///
/// The executor creates one command list per submission, records all passes, then submits it to the target queue.
/// Cross-queue dependencies are expressed via xiiGALFence signals and device-side waits inlined into the command list before the first consuming pass.
struct XII_GRAPHICSCORE_DLL xiiRGQueueSubmission
{
  xiiUInt32                                    m_uiQueueIndex = 0U;    ///< 0=Graphics, 1=AsyncCompute, 2=AsyncTransfer.
  xiiBitflags<xiiGALCommandQueueFlags>         m_QueueFlags;           ///< Redundant with the queue index, but useful to have directly available during command list creation.
  xiiDynamicArray<xiiUInt32>                   m_PassOrder;            ///< Ordered pass indices to execute.
  xiiSharedPtr<xiiGALFence>                    m_pSignalFence;         ///< Fence this submission signals after all its work (for downstream queues to DeviceWaitForFence).
  xiiUInt64                                    m_uiSignalValue = 0ULL; ///< Fence value to signal on m_pSignalFence.
  xiiHybridArray<xiiSharedPtr<xiiGALFence>, 2> m_WaitFences;           ///< Fences this submission must DeviceWaitForFence on before recording any commands.
  xiiHybridArray<xiiUInt64, 2>                 m_WaitValues;           ///< Fence values to wait for on m_WaitFences, indexed parallel to m_WaitFences.
};

/// A fully compiled render pass ready for execution.
struct XII_GRAPHICSCORE_DLL xiiRGCompiledPass
{
  xiiHashedString                            m_sName;                               ///< Debug name for this pass, used in profiling and diagnostics.
  xiiUInt32                                  m_uiPassIndex       = xiiInvalidIndex; ///< Index into the graph's pass table.
  xiiUInt32                                  m_uiQueueIndex      = 0U;              ///< 0=Graphics, 1=AsyncCompute, 2=AsyncTransfer.
  xiiUInt32                                  m_uiMergeGroupIndex = xiiInvalidIndex; ///< Index into the graph's merge group array, or xiiInvalidIndex if this pass is not merged with any others.
  bool                                       m_bHasSideEffects   = false;           ///< Whether this pass has side effects (e.g. present, copy to readback, UAV write with unknown output, etc.) that must be preserved even if no other pass reads from it.
  bool                                       m_bAllowMerge       = true;            ///< Whether this pass is allowed to be merged with adjacent passes on the same queue. This is a hint to the compiler, but not a guarantee.
  bool                                       m_bIsCulled         = false;           ///< Whether this pass was culled during compilation. Culled passes are not executed, but may still have side effects if they are reachable from a side-effect pass.
  xiiHybridArray<xiiUInt32, 4>               m_PreBarrierIndices;                   ///< For split barriers, the end barrier is emitted before the pass and the begin barrier is emitted after the pass, so the compiler can overlap the transition with GPU execution of this pass and future consumers.
  xiiHybridArray<xiiUInt32, 4>               m_PostBarrierBeginIndices;             ///< For split barriers, the begin barrier is emitted after the pass and the end barrier is emitted before the next producer, so the compiler can overlap the transition with GPU execution of this pass and past producers.
  xiiHybridArray<xiiUInt32, 4>               m_AcquireResourceIndices;              ///< Indices into the graph's resource table for transient resources whose lifetime starts at this pass. The executor will acquire these resources from the cache before executing the pass and return them to the cache after executing the pass.
  xiiHybridArray<xiiUInt32, 4>               m_ReleaseResourceIndices;              ///< Indices into the graph's resource table for transient resources whose lifetime ends at this pass. The executor will acquire these resources from the cache before executing the pass and return them to the cache after executing the pass.
  xiiHybridArray<xiiUInt32, 4>               m_DependencyPassIndices;               ///< Indices of passes that this pass depends on (i.e. there is a path of resource reads/writes from those passes to this pass). This is used for diagnostics and profiling, but not for execution order, which is determined by the queue submission order.
  void*                                      m_pPassData = nullptr;                 ///< The pass data struct is defined by the user in the setup callback and contains all information needed to execute the pass. The execute callback will cast this pointer back to the correct type.
  xiiDelegate<void(class xiiRGPassContext&)> m_ExecuteDelegate;                     ///< The execute callback records GPU commands for this pass into the command list provided by the context, using the resolved resources and blackboard data. The callback must not modify the graph or its resources, as it may be executed multiple times during the frame (e.g. for multi-GPU or split-frame rendering).
};

/// Controls optional features of the render graph compiler.
struct XII_GRAPHICSCORE_DLL xiiRGCompileSettings : public xiiHashableStruct<xiiRGCompileSettings>
{
  XII_DECLARE_POD_TYPE();

  bool      m_bEnablePassCulling   = true;  ///< Remove passes not reachable from any side-effect pass.
  bool      m_bEnableCompileCache  = true;  ///< Skip recompilation when the graph signature is unchanged.
  bool      m_bEnableSplitBarriers = false; ///< Use Begin/End split barriers to overlap transitions.
  bool      m_bEnableAsyncQueues   = true;  ///< Schedule async-compute/transfer passes on separate queues.
  bool      m_bEnableGPUProfiling  = false; ///< Emit Duration queries around each pass via the profiler.
  xiiUInt32 m_uiCacheSalt          = 0U;    ///< Invalidate the compile cache without changing the graph.
};

/// Per-compile statistics for diagnostics and HUD display.
struct XII_GRAPHICSCORE_DLL xiiRGStatistics : public xiiHashableStruct<xiiRGStatistics>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiRegisteredPassCount   = 0U;    ///< Passes registered with the graph before compilation.
  xiiUInt32 m_uiCompiledPassCount     = 0U;    ///< Passes that survived culling and were included in the final execution plan.
  xiiUInt32 m_uiCulledPassCount       = 0U;    ///< Passes that were culled because they are not reachable from any side-effect pass.
  xiiUInt32 m_uiTotalBarrierCount     = 0U;    ///< Total resource state barriers synthesized by the compiler.
  xiiUInt32 m_uiSplitBarrierCount     = 0U;    ///< Barriers that were split into Begin/End pairs to overlap with GPU execution.
  xiiUInt32 m_uiMergeGroupCount       = 0U;    ///< Groups of passes merged into a single native render pass.
  xiiUInt32 m_uiQueueSubmissionCount  = 0U;    ///< Command list batches scheduled on separate queues.
  xiiUInt32 m_uiTransientTextureCount = 0U;    ///< Transient textures that were automatically created and managed by the graph.
  xiiUInt32 m_uiTransientBufferCount  = 0U;    ///< Transient buffers that were automatically created and managed by the graph.
  xiiUInt64 m_uiGraphSignature        = 0ULL;  ///< Hash of the graph structure and compile settings, used for caching compiled results.
  bool      m_bUsedCachedCompile      = false; ///< Whether the compiler was able to skip work by reusing a cached execution plan from a previous compile with the same graph signature.
};

/// Execution context passed to every pass's execute callback.
///
/// Provides access to resolved GPU resources, the command list, the blackboard, the resource cache, and optional profiling / view data.
class XII_GRAPHICSCORE_DLL xiiRGPassContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRGPassContext);

public:
  /// Returns the command list for this pass to record GPU commands into.
  [[nodiscard]] xiiGALCommandList& GetCommandList() const;

  /// Resolves a virtual texture handle to its actual GPU texture for this frame.
  [[nodiscard]] xiiGALTexture* GetTexture(xiiRGTextureHandle hTexture) const;

  /// Resolves a virtual buffer handle to its actual GPU buffer for this frame.
  [[nodiscard]] xiiGALBuffer* GetBuffer(xiiRGBufferHandle hBuffer) const;

  /// Returns the per-frame blackboard for typed inter-pass data exchange.
  [[nodiscard]] xiiRenderGraphBlackboard& GetBlackboard() const;

  /// Returns the resource cache for transient GPU resource allocation.
  [[nodiscard]] xiiRenderGraphResourceCache& GetResourceCache() const;

  /// Returns the current view (may be null for headless passes).
  [[nodiscard]] const xiiView* GetView() const;

  /// Returns the frame index for the current execution.
  [[nodiscard]] xiiUInt64 GetFrameIndex() const;

  /// Returns the pass name for labeling / assertions.
  [[nodiscard]] xiiHashedString GetPassName() const;

private:
  friend class xiiRenderGraph;

  xiiRGPassContext() = default;

  xiiGALCommandList*           m_pCommandList   = nullptr;
  xiiRenderGraphBlackboard*    m_pBlackboard    = nullptr;
  xiiRenderGraphResourceCache* m_pResourceCache = nullptr;
  const xiiView*               m_pView          = nullptr;
  xiiUInt64                    m_uiFrameIndex   = 0ULL;
  xiiHashedString              m_sPassName;

  // Resource resolution tables is owned by the graph, borrowed for pass duration.
  xiiArrayPtr<xiiSharedPtr<xiiGALTexture>> m_ResolvedTextures;
  xiiArrayPtr<xiiSharedPtr<xiiGALBuffer>>  m_ResolvedBuffers;
};

/// Declarative API used inside a pass's setup callback to declare resource usage.
///
/// Each call to Read* or Write* records a dependency edge in the graph's dependency table and advances the resource version on writes.
/// The builder may only be used within the setup callback passed to xiiRenderGraph::AddPass().
class XII_GRAPHICSCORE_DLL xiiRGBuilder
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRGBuilder);

public:
  /// Declares a new transient texture resource owned by the graph.
  ///        Returns a handle pointing to version 0 (unwritten). Normally followed immediately by WriteTexture() to register the first write.
  [[nodiscard]] xiiRGTextureHandle DeclareTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description);

  /// Imports an externally-owned texture as a read-only graph resource.
  [[nodiscard]] xiiRGTextureHandle ImportTexture(xiiStringView sName, xiiSharedPtr<xiiGALTexture> pTexture, xiiBitflags<xiiGALResourceStateFlags> currentState);

  /// Declares a read dependency on the given texture at its current version.
  ///        Creates a dependency edge: this pass depends on the last writer.
  [[nodiscard]] xiiRGTextureHandle ReadTexture(xiiRGTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Read a previously declared/imported texture by name. This is less efficient than using the handle directly, so prefer to store the handle if you need to read the same resource multiple times.
  [[nodiscard]] xiiRGTextureHandle ReadTexture(xiiStringView sName, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Declares a write to the given texture, bumping its version.
  ///        Returns the new versioned handle - store this, not the input handle.
  [[nodiscard]] xiiRGTextureHandle WriteTexture(xiiRGTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Declares a new transient texture resource owned by the graph and registers the first write in one call.
  [[nodiscard]] xiiRGTextureHandle WriteTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState);


  /// Declares a new transient buffer resource owned by the graph.
  ///        Returns a handle pointing to version 0 (unwritten). Normally followed immediately by WriteBuffer() to register the first write.
  [[nodiscard]] xiiRGBufferHandle DeclareBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description);

  /// Imports an externally-owned buffer as a read-only graph resource.
  [[nodiscard]] xiiRGBufferHandle ImportBuffer(xiiStringView sName, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiBitflags<xiiGALResourceStateFlags> currentState);

  /// Declares a read dependency on the given buffer at its current version.
  ///        Creates a dependency edge: this pass depends on the last writer.
  [[nodiscard]] xiiRGBufferHandle ReadBuffer(xiiRGBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Read a previously declared/imported buffer by name. This is less efficient than using the handle directly, so prefer to store the handle if you need to read the same resource multiple times.
  [[nodiscard]] xiiRGBufferHandle ReadBuffer(xiiStringView sName, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Declares a write to the given buffer, bumping its version.
  ///        Returns the new versioned handle - store this, not the input handle.
  [[nodiscard]] xiiRGBufferHandle WriteBuffer(xiiRGBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Declares a new transient buffer resource owned by the graph and registers the first write in one call.
  [[nodiscard]] xiiRGBufferHandle WriteBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState);


  /// Marks this pass as having side effects that prevent it from being culled.
  ///        Call this for passes that write to swap-chain images, initiate readbacks, etc.
  void SetPassSideEffects(bool bHasSideEffects);

  /// Controls whether this pass participates in render-pass merge groups.
  ///        Default is true. Set to false if the pass must stand alone (e.g. readback).
  void SetPassAllowMerge(bool bAllowMerge);

private:
  friend class xiiRenderGraph;

  xiiRGBuilder(xiiRenderGraph& graph, xiiUInt32 uiPassIndex);

  xiiRenderGraph& m_Graph;
  xiiUInt32       m_uiPassIndex;
};

/// Advanced high-performance render graph.
///
/// ## Usage per frame
/// \code{.cpp}
/// graph.BeginSetup(uiFrameIndex);
///
/// auto [pData, hPass] = graph.AddPass<MyPassData>(
///   "DepthPrepass",
///   xiiGALCommandQueueFlags::Graphics,
///   [](MyPassData& data, xiiRGBuilder& builder)
///   {
///     data.hDepth = builder.WriteTexture("SceneDepth", depthTextureDescription, xiiGALResourceStateFlags::DepthWrite);
///   },
///   [](const MyPassData& data, xiiRGPassContext& context)
///   {
///     context.GetCommandList().ClearDepthStencilView(...);
///   },
///   /*bHasSideEffects=*/false
/// );
/// pData->uiDrawCount = scene.GetDrawCount();
///
/// graph.EndSetup();
/// if (graph.Compile(settings).Succeeded())
/// {
///   graph.Execute(pDevice, pView, pBlackboard, pCache, pProfiler);
/// }
/// \endcode
///
/// ## Threading
/// - BeginSetup / AddPass / EndSetup: single-threaded (main thread).
/// - Compile: single-threaded.
/// - Execute: command list recording per pass is sequential per queue. Queues run genuinely in parallel on the GPU side.
/// - Pass execute callbacks may safely call xiiRenderGraphBlackboard::TryGet concurrently.
class XII_GRAPHICSCORE_DLL xiiRenderGraph
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraph);

public:
  xiiRenderGraph();
  ~xiiRenderGraph();

  /// Begins graph setup for the given frame index.
  ///        Clears all pass registrations and resource declarations from the previous frame.
  void BeginSetup(xiiUInt64 uiFrameIndex);

  /// Registers a typed pass and immediately invokes its setup callback.
  ///
  /// \tparam TPassData - Plain data struct holding per-frame data and resource handles. Must be default-constructible. Lifetime is managed by the graph.
  ///
  /// \param sName           - Unique name used for debugging, profiling, and the compile cache.
  /// \param queueFlags      - Target queue (Graphics / Compute / Transfer).
  /// \param setupFunc       - Called immediately to declare resource usage via xiiRGBuilder.
  /// \param executeFunc     - Called during Execute() to record GPU commands.
  /// \param bHasSideEffects - If true, this pass is never culled even if no pass reads its outputs.
  ///
  /// \returns A pair of (raw pointer to TPassData, pass handle). The pointer is stable for the lifetime of this frame's graph.
  template <typename TPassData>
  std::pair<TPassData*, xiiRGPassHandle> AddPass(xiiStringView sName, xiiBitflags<xiiGALCommandQueueFlags> queueFlags, xiiDelegate<void(TPassData&, xiiRGBuilder&)> setupDelegate, xiiDelegate<void(const TPassData&, xiiRGPassContext&)> executeDelegate, bool bHasSideEffects = false);

  /// Finalizes the setup phase. Must be called after all AddPass calls.
  void EndSetup();

  /// Compiles the render graph.
  ///
  /// Runs all 7 compiler phases:
  ///   A. Resource versioning (already completed during AddPass setup callbacks).
  ///   B. Topological sort + live-pass culling.
  ///   C. Transient resource lifetime analysis.
  ///   D. Advanced barrier synthesis (sub-resource, UAV, split barriers).
  ///   E. Multi-queue scheduling -> xiiRGQueueSubmission list with fence handshakes.
  ///   F. Render pass group merging -> xiiRGMergeGroup list.
  ///   G. Signature hash + compile cache (skip if graph is unchanged).
  ///
  /// \param settings   - Optional compile configuration.
  /// \param out_pError - Optional diagnostics string populated on XII_FAILURE.
  ///
  /// \returns XII_SUCCESS on success, XII_FAILURE if a cycle or invalid state is detected.
  [[nodiscard]] xiiResult Compile(const xiiRGCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);


  /// Executes the compiled render graph.
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
  /// \param pDevice        - Target GAL device (must remain valid for the call duration).
  /// \param pView          - Optional view providing camera / viewport data to passes.
  /// \param pBlackboard    - Per-frame data exchange store.
  /// \param pResourceCache - Transient resource pool for this frame.
  /// \param pProfiler      - Optional GPU timing profiler.
  /// \param out_pError     - Optional diagnostics output.
  ///
  /// \returns XII_SUCCESS on success.
  [[nodiscard]] xiiResult Execute(xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphBlackboard* pBlackboard, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler = nullptr, xiiStringBuilder* out_pError = nullptr);


  /// Returns the graph's compile-time statistics, populated after Compile() and useful for diagnostics.
  [[nodiscard]] const xiiRGStatistics& GetStatistics() const;

  /// Returns the list of compiled passes, in execution order, with all metadata needed for execution and profiling.
  [[nodiscard]] xiiArrayPtr<const xiiRGCompiledPass> GetCompiledPasses() const;

  /// Returns the list of resource barriers synthesized by the compiler, in the order they are emitted during execution.
  [[nodiscard]] xiiArrayPtr<const xiiRGBarrierDescription> GetBarriers() const;

  /// Returns the list of render pass merge groups synthesized by the compiler.
  [[nodiscard]] xiiArrayPtr<const xiiRGMergeGroup> GetMergeGroups() const;

  /// Returns the list of queue submissions synthesized by the compiler, in execution order.
  [[nodiscard]] xiiArrayPtr<const xiiRGQueueSubmission> GetQueueSubmissions() const;

  /// Serializes the compiled graph to a DOT string for Graphviz visualization.
  [[nodiscard]] xiiResult DumpToDot(xiiStringBuilder& out_sDot) const;

  /// True if the graph has been compiled and not yet invalidated.
  [[nodiscard]] bool IsCompiled() const;

private:
  friend class xiiRGBuilder;

  struct ResourceEntry
  {
    xiiHashedString m_sName;
    bool            m_bIsTexture   = true;
    bool            m_bIsImported  = false;
    bool            m_bIsTransient = false; ///< True = graph-owned, allocated via ResourceCache.

    xiiGALTextureCreationDescription m_TextureDescription;
    xiiGALBufferCreationDescription  m_BufferDescription;

    // Imported external resources (non-transient).
    xiiSharedPtr<xiiGALTexture>           m_pImportedTexture;
    xiiSharedPtr<xiiGALBuffer>            m_pImportedBuffer;
    xiiBitflags<xiiGALResourceStateFlags> m_ImportedInitialState = xiiGALResourceStateFlags::Unknown;

    // Resolved for current frame execution (filled by executor).
    xiiSharedPtr<xiiGALTexture> m_pResolvedTexture;
    xiiSharedPtr<xiiGALBuffer>  m_pResolvedBuffer;

    // Current write version (bumped on each write during setup).
    xiiUInt16 m_uiCurrentVersion         = 0U;
    xiiUInt32 m_uiCurrentProducerPassIdx = xiiInvalidIndex;

    // Lifetime (populated during Phase C).
    xiiUInt32 m_uiFirstUsePassIdx = xiiInvalidIndex;
    xiiUInt32 m_uiLastUsePassIdx  = xiiInvalidIndex;

    // Current state known at compile time (used for barrier synthesis).
    xiiBitflags<xiiGALResourceStateFlags> m_CurrentState = xiiGALResourceStateFlags::Unknown;
  };

  struct ResourceUsage
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiResourceIndex;
    bool      m_bIsTexture;
    xiiUInt16 m_uiVersion; ///< Version being read or written.

    xiiBitflags<xiiGALResourceStateFlags> m_RequiredState;
    bool                                  m_bIsWrite;
  };

  struct PassEntry
  {
    xiiHashedString                      m_sName;
    xiiBitflags<xiiGALCommandQueueFlags> m_QueueFlags;
    bool                                 m_bHasSideEffects = false;
    bool                                 m_bAllowMerge     = true;

    xiiHybridArray<ResourceUsage, 8> m_Reads;
    xiiHybridArray<ResourceUsage, 8> m_Writes;

    // Type-erased pass data and execution delegate (owned by this entry).
    void*                                m_pPassData = nullptr;
    xiiDelegate<void(void*)>             m_DestroyPassDataDelegate;
    xiiDelegate<void(xiiRGPassContext&)> m_ExecuteDelegate;
  };

  void PhaseB_TopologicalSortAndCull(const xiiRGCompileSettings& settings, xiiDynamicArray<xiiUInt32>& out_sortedIndices);
  void PhaseC_LifetimeAnalysis(const xiiDynamicArray<xiiUInt32>& sortedIndices);
  void PhaseD_BarrierSynthesis(const xiiDynamicArray<xiiUInt32>& sortedIndices, const xiiRGCompileSettings& settings);
  void PhaseE_MultiQueueScheduling(const xiiDynamicArray<xiiUInt32>& sortedIndices, xiiGALDevice* pDevice, const xiiRGCompileSettings& settings);
  void PhaseF_RenderPassMerging(xiiGALDevice* pDevice);
  void PhaseG_SignatureAndCache(const xiiRGCompileSettings& settings);

  void EmitBarrier(xiiUInt32 uiConsumerPassIdx, xiiUInt32 uiResourceIdx, bool bIsTexture, xiiBitflags<xiiGALResourceStateFlags> afterState, bool bSplitBarrier, xiiUInt32 uiFirstMip = 0U, xiiUInt32 uiMipCount = XII_GAL_REMAINING_MIP_LEVELS, xiiUInt32 uiFirstSlice = 0U, xiiUInt32 uiSliceCount = XII_GAL_REMAINING_ARRAY_SLICES);

  [[nodiscard]] static xiiUInt64 ComputeSignature(const xiiDynamicArray<PassEntry>& passes);

private:
  xiiDynamicArray<PassEntry>               m_Passes;            ///< Setup-phase pass list, cleared each BeginSetup().
  xiiDynamicArray<ResourceEntry>           m_Resources;         ///< Virtual resource table, cleared each BeginSetup().
  xiiHashTable<xiiHashedString, xiiUInt32> m_ResourceNameIndex; ///< Fast name -> resource index lookup.

  // Compiled outputs.
  xiiDynamicArray<xiiRGCompiledPass>       m_CompiledPasses;
  xiiDynamicArray<xiiRGBarrierDescription> m_Barriers;
  xiiDynamicArray<xiiRGMergeGroup>         m_MergeGroups;
  xiiDynamicArray<xiiRGQueueSubmission>    m_QueueSubmissions;

  xiiRGStatistics      m_Statistics;
  xiiRGCompileSettings m_LastCompileSettings;
  xiiUInt64            m_uiLastSignature = 0ULL;
  xiiUInt64            m_uiFrameIndex    = 0ULL;
  bool                 m_bIsSetupOpen    = false;
  bool                 m_bIsCompiled     = false;
};

#include <GraphicsCore/Pipeline/Implementation/RenderGraph_inl.h>
