/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Algorithm/HashingUtils.h>
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

/// Stable, process-independent identifier used by tooling, profiling and graph serialization.
/// Runtime handles additionally carry a dense table index; the stable ID deliberately does not.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphId : public xiiHashableStruct<xiiRenderGraphId>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64 m_uiValue = 0ULL;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiValue != 0ULL; }
  [[nodiscard]] XII_ALWAYS_INLINE bool operator==(const xiiRenderGraphId& rhs) const { return m_uiValue == rhs.m_uiValue; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphId);

using xiiRenderGraphGraphId    = xiiRenderGraphId;
using xiiRenderGraphPassId     = xiiRenderGraphId;
using xiiRenderGraphResourceId = xiiRenderGraphId;
using xiiRenderGraphVersionId  = xiiRenderGraphId;

/// Stable-ID to dense-index lookup rebuilt by the compiler. Dense indices are frame-local;
/// stable IDs remain suitable for captures, profiler streams and editor selections.
class XII_GRAPHICSCORE_DLL xiiRenderGraphIdTable
{
public:
  void Clear();
  void SetGraphId(xiiRenderGraphGraphId id);
  void RegisterPass(xiiRenderGraphPassId id, xiiUInt32 uiIndex);
  void RegisterResource(xiiRenderGraphResourceId id, xiiUInt32 uiIndex);
  void RegisterVersion(xiiRenderGraphVersionId id, xiiUInt32 uiIndex);

  [[nodiscard]] xiiRenderGraphGraphId GetGraphId() const;
  [[nodiscard]] xiiUInt32             FindPass(xiiRenderGraphPassId id) const;
  [[nodiscard]] xiiUInt32             FindResource(xiiRenderGraphResourceId id) const;
  [[nodiscard]] xiiUInt32             FindVersion(xiiRenderGraphVersionId id) const;

private:
  xiiRenderGraphGraphId              m_GraphId;
  xiiHashTable<xiiUInt64, xiiUInt32> m_Passes;
  xiiHashTable<xiiUInt64, xiiUInt32> m_Resources;
  xiiHashTable<xiiUInt64, xiiUInt32> m_Versions;
};

/// Flags that control the lifetime and behavior of a render graph resource.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphResourceFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None       = 0U,
    Transient  = XII_BIT(0), ///< Resource is transient and can be discarded after the last pass that uses it.
    External   = XII_BIT(1), ///< Resource is external and must not be destroyed by the render graph executor.
    Persistent = XII_BIT(2), ///< Resource is persistent and must not be destroyed by the render graph executor, even if it is not used in the current frame.

    Default = None
  };

  struct Bits
  {
    StorageType Transient : 1;
    StorageType External : 1;
    StorageType Persistent : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderGraphResourceFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphResourceFlags);

/// Opaque handle to a virtual texture resource declared in the render graph.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphTextureHandle : public xiiHashableStruct<xiiRenderGraphTextureHandle>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                m_uiIndex   = xiiInvalidIndex;
  xiiUInt16                m_uiVersion = 0U;
  xiiRenderGraphResourceId m_Id;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex; }
  [[nodiscard]] XII_ALWAYS_INLINE bool IsInvalidated() const { return !IsValid(); }
  [[nodiscard]] XII_ALWAYS_INLINE bool operator==(const xiiRenderGraphTextureHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex && m_uiVersion == rhs.m_uiVersion; }
};

/// Opaque handle to a virtual buffer resource declared in the render graph.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphBufferHandle : public xiiHashableStruct<xiiRenderGraphBufferHandle>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                m_uiIndex   = xiiInvalidIndex;
  xiiUInt16                m_uiVersion = 0U;
  xiiRenderGraphResourceId m_Id;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex; }
  [[nodiscard]] XII_ALWAYS_INLINE bool IsInvalidated() const { return !IsValid(); }
  [[nodiscard]] XII_ALWAYS_INLINE bool operator==(const xiiRenderGraphBufferHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex && m_uiVersion == rhs.m_uiVersion; }
};

/// Opaque handle to a registered render pass.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphPassHandle : public xiiHashableStruct<xiiRenderGraphPassHandle>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32            m_uiIndex = xiiInvalidIndex;
  xiiRenderGraphPassId m_Id;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex; }
};

/// Describes a resource state barrier synthesized during compilation.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphBarrierDescription : public xiiHashableStruct<xiiRenderGraphBarrierDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                             m_uiResourceIndex         = xiiInvalidIndex;                      ///< Index into the resource table.
  bool                                  m_bIsTexture              = true;                                 ///< Whether the barrier is for a texture or a buffer resource.
  xiiUInt32                             m_uiFirstMipLevel         = 0U;                                   ///< For textures, the first mip level affected by the barrier. For buffers, this is always 0.
  xiiUInt32                             m_uiMipLevelCount         = XII_GAL_REMAINING_MIP_LEVELS;         ///< For textures, the number of mip levels affected by the barrier. For buffers, this is always 1.
  xiiUInt32                             m_uiFirstArraySlice       = 0U;                                   ///< For textures, the first array slice affected by the barrier. For buffers, this is always 0.
  xiiUInt32                             m_uiArraySliceCount       = XII_GAL_REMAINING_ARRAY_SLICES;       ///< For textures, the number of array slices affected by the barrier. For buffers, this is always 1.
  xiiBitflags<xiiGALResourceStateFlags> m_BeforeState             = xiiGALResourceStateFlags::Unknown;    ///< The resource state before the barrier. This is used for validation and may be Unknown if the state is not known at compile time.
  xiiBitflags<xiiGALResourceStateFlags> m_AfterState              = xiiGALResourceStateFlags::Unknown;    ///< The resource state after the barrier. This is used for validation and may be Unknown if the state is not known at compile time.
  xiiEnum<xiiGALStateTransitionType>    m_TransitionType          = xiiGALStateTransitionType::Immediate; ///< Whether this barrier is an immediate transition or a split barrier begin/end.
  xiiEnum<xiiGALStateTransitionFlags>   m_TransitionFlags         = xiiGALStateTransitionFlags::None;     ///< Additional flags for the barrier, such as whether to discard content or update internal resource state. Only relevant for immediate barriers and split-barrier ends.
  xiiUInt8                              m_uiSourceQueue           = 0U;                                   ///< Queue owning the resource before this transition.
  xiiUInt8                              m_uiTargetQueue           = 0U;                                   ///< Queue that consumes the resource after this transition.
  bool                                  m_bQueueOwnershipTransfer = false;                                ///< True when the transition is paired with a cross-queue fence wait.
};

/// Tool-facing metadata for one immutable resource version in the version timeline.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphResourceVersionDescription
{
  xiiRenderGraphVersionId               m_Id;
  xiiRenderGraphResourceId              m_ResourceId;
  xiiRenderGraphPassId                  m_ProducerPassId;
  xiiUInt16                             m_uiVersion           = 0U;
  xiiUInt16                             m_uiParentVersion     = 0xFFFFU;
  xiiUInt32                             m_uiFirstUsePassIndex = xiiInvalidIndex;
  xiiUInt32                             m_uiLastUsePassIndex  = xiiInvalidIndex;
  xiiBitflags<xiiGALResourceStateFlags> m_RequiredState       = xiiGALResourceStateFlags::Unknown;
  xiiBitflags<xiiGALResourceStateFlags> m_CurrentState        = xiiGALResourceStateFlags::Unknown;
  bool                                  m_bExported           = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphResourceVersionDescription);

/// Tool-facing lifetime and aliasing record for a logical graph resource.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphResourceDescription
{
  xiiRenderGraphResourceId                 m_Id;
  xiiHashedString                          m_sName;
  xiiBitflags<xiiRenderGraphResourceFlags> m_Flags;
  xiiUInt32                                m_uiFirstUsePassIndex = xiiInvalidIndex;
  xiiUInt32                                m_uiLastUsePassIndex  = xiiInvalidIndex;
  xiiUInt32                                m_uiAliasGroup        = xiiInvalidIndex;
  bool                                     m_bIsTexture          = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphResourceDescription);

/// Represents a group of consecutive passes merged into a single native render pass.
///
/// Within a group, passes share the same set of render-target and depth-stencil attachments.
/// The GPU never resolves tiles between passes in the group, which is critical for performance on tile-based architectures.
/// All passes must be on the same queue index.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphMergeGroup
{
  xiiHybridArray<xiiUInt32, 8>    m_PassIndices; ///< Ordered pass indices belonging to this group.
  xiiSharedPtr<xiiGALRenderPass>  m_pNativeRenderPass;
  xiiSharedPtr<xiiGALFramebuffer> m_pFramebuffer;
};

/// A batch of passes submitted together to a single command queue.
///
/// The executor creates one command list per submission, records all passes, then submits it to the target queue.
/// Cross-queue dependencies are expressed via xiiGALFence signals and device-side waits inlined into the command list before the first consuming pass.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphQueueSubmission
{
  xiiUInt32                                    m_uiQueueIndex = 0U;     ///< 0=Graphics, 1=AsyncCompute, 2=AsyncTransfer.
  xiiBitflags<xiiGALCommandQueueFlags>         m_QueueFlags;            ///< Redundant with the queue index, but useful to have directly available during command list creation.
  xiiDynamicArray<xiiUInt32>                   m_PassOrder;             ///< Ordered pass indices to execute.
  xiiSharedPtr<xiiGALFence>                    m_pSignalFence;          ///< Fence this submission signals after all its work (for downstream queues to DeviceWaitForFence).
  xiiUInt64                                    m_uiSignalValue = 0ULL;  ///< Fence value to signal on m_pSignalFence.
  xiiHybridArray<xiiSharedPtr<xiiGALFence>, 2> m_WaitFences;            ///< Fences this submission must DeviceWaitForFence on before recording any commands.
  xiiHybridArray<xiiUInt64, 2>                 m_WaitValues;            ///< Fence values to wait for on m_WaitFences, indexed parallel to m_WaitFences.
  xiiHybridArray<xiiUInt32, 2>                 m_WaitSubmissionIndices; ///< Producer submissions that must complete before this submission starts.
};

/// A fully compiled render pass ready for execution.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphCompiledPass
{
  xiiRenderGraphPassId                                m_Id;                                  ///< Stable pass ID for tools and profiling.
  xiiHashedString                                     m_sName;                               ///< Debug name for this pass, used in profiling and diagnostics.
  xiiUInt32                                           m_uiPassIndex       = xiiInvalidIndex; ///< Index into the graph's pass table.
  xiiUInt32                                           m_uiQueueIndex      = 0U;              ///< 0=Graphics, 1=AsyncCompute, 2=AsyncTransfer.
  xiiUInt32                                           m_uiMergeGroupIndex = xiiInvalidIndex; ///< Index into the graph's merge group array, or xiiInvalidIndex if this pass is not merged with any others.
  bool                                                m_bHasSideEffects   = false;           ///< Whether this pass has side effects (e.g. present, copy to readback, UAV write with unknown output, etc.) that must be preserved even if no other pass reads from it.
  bool                                                m_bAllowMerge       = true;            ///< Whether this pass is allowed to be merged with adjacent passes on the same queue. This is a hint to the compiler, but not a guarantee.
  bool                                                m_bIsCulled         = false;           ///< Whether this pass was culled during compilation. Culled passes are not executed, but may still have side effects if they are reachable from a side-effect pass.
  xiiHybridArray<xiiUInt32, 4>                        m_PreBarrierIndices;                   ///< For split barriers, the end barrier is emitted before the pass and the begin barrier is emitted after the pass, so the compiler can overlap the transition with GPU execution of this pass and future consumers.
  xiiHybridArray<xiiUInt32, 4>                        m_PostBarrierBeginIndices;             ///< For split barriers, the begin barrier is emitted after the pass and the end barrier is emitted before the next producer, so the compiler can overlap the transition with GPU execution of this pass and past producers.
  xiiHybridArray<xiiUInt32, 4>                        m_PostBarrierIndices;                  ///< Immediate export/ownership transitions emitted after the pass.
  xiiHybridArray<xiiUInt32, 4>                        m_AcquireResourceIndices;              ///< Indices into the graph's resource table for transient resources whose lifetime starts at this pass. The executor will acquire these resources from the cache before executing the pass and return them to the cache after executing the pass.
  xiiHybridArray<xiiUInt32, 4>                        m_ReleaseResourceIndices;              ///< Indices into the graph's resource table for transient resources whose lifetime ends at this pass. The executor will acquire these resources from the cache before executing the pass and return them to the cache after executing the pass.
  xiiHybridArray<xiiUInt32, 4>                        m_DependencyPassIndices;               ///< Indices of passes that this pass depends on (i.e. there is a path of resource reads/writes from those passes to this pass). This is used for diagnostics and profiling, but not for execution order, which is determined by the queue submission order.
  void*                                               m_pPassData = nullptr;                 ///< The pass data struct is defined by the user in the setup callback and contains all information needed to execute the pass. The execute callback will cast this pointer back to the correct type.
  xiiDelegate<void(class xiiRenderGraphPassContext&)> m_ExecuteDelegate;                     ///< The execute callback records GPU commands for this pass into the command list provided by the context, using the resolved resources and blackboard data. The callback must not modify the graph or its resources, as it may be executed multiple times during the frame (e.g. for multi-GPU or split-frame rendering).
};

/// Controls optional features of the render graph compiler.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphCompileSettings : public xiiHashableStruct<xiiRenderGraphCompileSettings>
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
struct XII_GRAPHICSCORE_DLL xiiRenderGraphStatistics : public xiiHashableStruct<xiiRenderGraphStatistics>
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
  xiiUInt32 m_uiAliasGroupCount       = 0U;    ///< Number of compatible physical allocation groups used by transient resources.
  xiiUInt32 m_uiAliasedResourceCount  = 0U;    ///< Number of logical resources sharing an allocation group with another resource.
  xiiUInt64 m_uiGraphSignature        = 0ULL;  ///< Hash of the graph structure and compile settings, used for caching compiled results.
  bool      m_bUsedCachedCompile      = false; ///< Whether the compiler was able to skip work by reusing a cached execution plan from a previous compile with the same graph signature.
};

/// Execution context passed to every pass's execute callback.
///
/// Provides access to resolved GPU resources, the command list, the blackboard, the resource cache, and optional profiling / view data.
class XII_GRAPHICSCORE_DLL xiiRenderGraphPassContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphPassContext);

public:
  /// Returns the command list for this pass to record GPU commands into.
  [[nodiscard]] xiiGALCommandList& GetCommandList() const;

  /// Resolves a virtual texture handle to its actual GPU texture for this frame.
  [[nodiscard]] xiiGALTexture* GetTexture(xiiRenderGraphTextureHandle hTexture) const;

  /// Resolves a virtual buffer handle to its actual GPU buffer for this frame.
  [[nodiscard]] xiiGALBuffer* GetBuffer(xiiRenderGraphBufferHandle hBuffer) const;

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

  xiiRenderGraphPassContext() = default;

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
class XII_GRAPHICSCORE_DLL xiiRenderGraphBuilder
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphBuilder);

public:
  /// Declares a new transient texture resource owned by the graph.
  ///        Returns a handle pointing to version 0 (unwritten). Normally followed immediately by WriteTexture() to register the first write.
  [[nodiscard]] xiiRenderGraphTextureHandle DeclareTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description);

  /// Declares a texture with explicit lifetime flags. Exactly one of Transient or Persistent must be set.
  [[nodiscard]] xiiRenderGraphTextureHandle DeclareTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description, xiiBitflags<xiiRenderGraphResourceFlags> flags);

  /// Imports an externally-owned texture as a read-only graph resource.
  [[nodiscard]] xiiRenderGraphTextureHandle ImportTexture(xiiStringView sName, xiiSharedPtr<xiiGALTexture> pTexture, xiiBitflags<xiiGALResourceStateFlags> currentState);

  /// Declares a read dependency on the given texture at its current version.
  ///        Creates a dependency edge: this pass depends on the last writer.
  [[nodiscard]] xiiRenderGraphTextureHandle ReadTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Read a previously declared/imported texture by name. This is less efficient than using the handle directly, so prefer to store the handle if you need to read the same resource multiple times.
  [[nodiscard]] xiiRenderGraphTextureHandle ReadTexture(xiiStringView sName, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Declares a write to the given texture, bumping its version.
  ///        Returns the new versioned handle - store this, not the input handle.
  [[nodiscard]] xiiRenderGraphTextureHandle WriteTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Declares a new transient texture resource owned by the graph and registers the first write in one call.
  [[nodiscard]] xiiRenderGraphTextureHandle WriteTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Marks a texture version as a graph output and optionally transitions it to the consumer's state.
  void ExportTexture(xiiRenderGraphTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> finalState);


  /// Declares a new transient buffer resource owned by the graph.
  ///        Returns a handle pointing to version 0 (unwritten). Normally followed immediately by WriteBuffer() to register the first write.
  [[nodiscard]] xiiRenderGraphBufferHandle DeclareBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description);

  /// Declares a buffer with explicit lifetime flags. Exactly one of Transient or Persistent must be set.
  [[nodiscard]] xiiRenderGraphBufferHandle DeclareBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description, xiiBitflags<xiiRenderGraphResourceFlags> flags);

  /// Imports an externally-owned buffer as a read-only graph resource.
  [[nodiscard]] xiiRenderGraphBufferHandle ImportBuffer(xiiStringView sName, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiBitflags<xiiGALResourceStateFlags> currentState);

  /// Declares a read dependency on the given buffer at its current version.
  ///        Creates a dependency edge: this pass depends on the last writer.
  [[nodiscard]] xiiRenderGraphBufferHandle ReadBuffer(xiiRenderGraphBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Read a previously declared/imported buffer by name. This is less efficient than using the handle directly, so prefer to store the handle if you need to read the same resource multiple times.
  [[nodiscard]] xiiRenderGraphBufferHandle ReadBuffer(xiiStringView sName, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Declares a write to the given buffer, bumping its version.
  ///        Returns the new versioned handle - store this, not the input handle.
  [[nodiscard]] xiiRenderGraphBufferHandle WriteBuffer(xiiRenderGraphBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Declares a new transient buffer resource owned by the graph and registers the first write in one call.
  [[nodiscard]] xiiRenderGraphBufferHandle WriteBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState);

  /// Marks a buffer version as a graph output and optionally transitions it to the consumer's state.
  void ExportBuffer(xiiRenderGraphBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> finalState);


  /// Marks this pass as having side effects that prevent it from being culled.
  ///        Call this for passes that write to swap-chain images, initiate readbacks, etc.
  void SetPassSideEffects(bool bHasSideEffects);

  /// Controls whether this pass participates in render-pass merge groups.
  ///        Default is true. Set to false if the pass must stand alone (e.g. readback).
  void SetPassAllowMerge(bool bAllowMerge);

private:
  friend class xiiRenderGraph;

  xiiRenderGraphBuilder(xiiRenderGraph& graph, xiiUInt32 uiPassIndex);

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
///   [](MyPassData& data, xiiRenderGraphBuilder& builder)
///   {
///     data.hDepth = builder.WriteTexture("SceneDepth", depthTextureDescription, xiiGALResourceStateFlags::DepthWrite);
///   },
///   [](const MyPassData& data, xiiRenderGraphPassContext& context)
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
  explicit xiiRenderGraph(xiiStringView sName = "RenderGraph");
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
  /// \param setupFunc       - Called immediately to declare resource usage via xiiRenderGraphBuilder.
  /// \param executeFunc     - Called during Execute() to record GPU commands.
  /// \param bHasSideEffects - If true, this pass is never culled even if no pass reads its outputs.
  ///
  /// \returns A pair of (raw pointer to TPassData, pass handle). The pointer is stable for the lifetime of this frame's graph.
  template <typename TPassData>
  std::pair<TPassData*, xiiRenderGraphPassHandle> AddPass(xiiStringView sName, xiiBitflags<xiiGALCommandQueueFlags> queueFlags, xiiDelegate<void(TPassData&, xiiRenderGraphBuilder&)> setupDelegate, xiiDelegate<void(const TPassData&, xiiRenderGraphPassContext&)> executeDelegate, bool bHasSideEffects = false);

  /// Finalizes the setup phase. Must be called after all AddPass calls.
  void EndSetup();

  /// Compiles the render graph.
  ///
  /// Runs all 7 compiler phases:
  ///   A. Resource versioning (already completed during AddPass setup callbacks).
  ///   B. Topological sort + live-pass culling.
  ///   C. Transient resource lifetime analysis.
  ///   D. Advanced barrier synthesis (sub-resource, UAV, split barriers).
  ///   E. Multi-queue scheduling -> xiiRenderGraphQueueSubmission list with fence handshakes.
  ///   F. Render pass group merging -> xiiRenderGraphMergeGroup list.
  ///   G. Signature hash + compile cache (skip if graph is unchanged).
  ///
  /// \param settings   - Optional compile configuration.
  /// \param out_pError - Optional diagnostics string populated on XII_FAILURE.
  ///
  /// \returns XII_SUCCESS on success, XII_FAILURE if a cycle or invalid state is detected.
  [[nodiscard]] xiiResult Compile(const xiiRenderGraphCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);


  /// Executes the compiled render graph.
  ///
  /// For each xiiRenderGraphQueueSubmission:
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
  [[nodiscard]] const xiiRenderGraphStatistics& GetStatistics() const;

  /// Returns the list of compiled passes, in execution order, with all metadata needed for execution and profiling.
  [[nodiscard]] xiiArrayPtr<const xiiRenderGraphCompiledPass> GetCompiledPasses() const;

  /// Returns the list of resource barriers synthesized by the compiler, in the order they are emitted during execution.
  [[nodiscard]] xiiArrayPtr<const xiiRenderGraphBarrierDescription> GetBarriers() const;

  /// Returns the list of render pass merge groups synthesized by the compiler.
  [[nodiscard]] xiiArrayPtr<const xiiRenderGraphMergeGroup> GetMergeGroups() const;

  /// Returns the list of queue submissions synthesized by the compiler, in execution order.
  [[nodiscard]] xiiArrayPtr<const xiiRenderGraphQueueSubmission> GetQueueSubmissions() const;

  /// Stable ID and name of this graph instance.
  [[nodiscard]] xiiRenderGraphGraphId GetId() const;
  [[nodiscard]] xiiStringView         GetName() const;

  /// Reflection/editor-friendly compiler records.
  [[nodiscard]] xiiArrayPtr<const xiiRenderGraphResourceDescription>        GetResourceDescriptions() const;
  [[nodiscard]] xiiArrayPtr<const xiiRenderGraphResourceVersionDescription> GetResourceVersions() const;
  [[nodiscard]] const xiiRenderGraphIdTable&                                GetIdTable() const;

  /// Returns a strong reference to an exported graph-owned result after Execute().
  [[nodiscard]] xiiSharedPtr<xiiGALTexture> GetExportedTexture(xiiRenderGraphTextureHandle hTexture) const;
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer>  GetExportedBuffer(xiiRenderGraphBufferHandle hBuffer) const;

  /// Serializes the compiled graph to a DOT string for Graphviz visualization.
  [[nodiscard]] xiiResult DumpToDot(xiiStringBuilder& out_sDot) const;

  /// True if the graph has been compiled and not yet invalidated.
  [[nodiscard]] bool IsCompiled() const;

private:
  friend class xiiRenderGraphBuilder;

  struct VersionEntry
  {
    xiiRenderGraphVersionId               m_Id;
    xiiUInt16                             m_uiVersion         = 0U;
    xiiUInt16                             m_uiParentVersion   = 0xFFFFU;
    xiiUInt32                             m_uiProducerPassIdx = xiiInvalidIndex;
    xiiUInt32                             m_uiFirstUsePassIdx = xiiInvalidIndex;
    xiiUInt32                             m_uiLastUsePassIdx  = xiiInvalidIndex;
    xiiBitflags<xiiGALResourceStateFlags> m_RequiredState     = xiiGALResourceStateFlags::Unknown;
    xiiBitflags<xiiGALResourceStateFlags> m_CurrentState      = xiiGALResourceStateFlags::Unknown;
    bool                                  m_bExported         = false;
    xiiHybridArray<xiiUInt32, 4>          m_ReaderPassIndices;
  };

  struct ResourceEntry
  {
    xiiRenderGraphResourceId                 m_Id;
    xiiHashedString                          m_sName;
    xiiBitflags<xiiRenderGraphResourceFlags> m_Flags        = xiiRenderGraphResourceFlags::Transient;
    bool                                     m_bIsTexture   = true;
    bool                                     m_bIsImported  = false;
    bool                                     m_bIsTransient = true;
    xiiGALTextureCreationDescription         m_TextureDescription;
    xiiGALBufferCreationDescription          m_BufferDescription;
    xiiSharedPtr<xiiGALTexture>              m_pImportedTexture;
    xiiSharedPtr<xiiGALBuffer>               m_pImportedBuffer;
    xiiBitflags<xiiGALResourceStateFlags>    m_ImportedInitialState     = xiiGALResourceStateFlags::Unknown;
    xiiBitflags<xiiGALResourceStateFlags>    m_CurrentState             = xiiGALResourceStateFlags::Unknown;
    xiiBitflags<xiiGALResourceStateFlags>    m_ExportFinalState         = xiiGALResourceStateFlags::Unknown;
    xiiUInt16                                m_uiCurrentVersion         = 0U;
    xiiUInt32                                m_uiCurrentProducerPassIdx = xiiInvalidIndex;
    xiiUInt32                                m_uiFirstUsePassIdx        = xiiInvalidIndex;
    xiiUInt32                                m_uiLastUsePassIdx         = xiiInvalidIndex;
    xiiUInt32                                m_uiAliasGroup             = xiiInvalidIndex;
    xiiUInt8                                 m_uiQueueMask              = 0U;
    xiiDynamicArray<VersionEntry>            m_Versions;
  };

  struct ResourceUsage
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32                             m_uiResourceIndex = xiiInvalidIndex;
    bool                                  m_bIsTexture      = true;
    xiiUInt16                             m_uiVersion       = 0U;
    xiiBitflags<xiiGALResourceStateFlags> m_RequiredState   = xiiGALResourceStateFlags::Unknown;
    bool                                  m_bIsWrite        = false;
  };

  struct PassEntry
  {
    xiiRenderGraphPassId m_Id;

    xiiHashedString                      m_sName;
    xiiBitflags<xiiGALCommandQueueFlags> m_QueueFlags;
    bool                                 m_bHasSideEffects = false;
    bool                                 m_bAllowMerge     = true;
    xiiHybridArray<ResourceUsage, 8U>    m_Reads;
    xiiHybridArray<ResourceUsage, 8U>    m_Writes;

    void*                                         m_pPassData = nullptr;
    xiiDelegate<void(void*)>                      m_DestroyPassDataDelegate;
    xiiDelegate<void(xiiRenderGraphPassContext&)> m_ExecuteDelegate;
  };

  void PhaseB_TopologicalSortAndCull(const xiiRenderGraphCompileSettings& settings, xiiDynamicArray<xiiUInt32>& out_sortedIndices);
  void PhaseC_LifetimeAnalysis(const xiiDynamicArray<xiiUInt32>& sortedIndices);
  void PhaseD_BarrierSynthesis(const xiiDynamicArray<xiiUInt32>& sortedIndices, const xiiRenderGraphCompileSettings& settings);
  void PhaseE_MultiQueueScheduling(const xiiDynamicArray<xiiUInt32>& sortedIndices, xiiGALDevice* pDevice, const xiiRenderGraphCompileSettings& settings);
  void PhaseF_RenderPassMerging(xiiGALDevice* pDevice);
  void PhaseG_SignatureAndCache(const xiiRenderGraphCompileSettings& settings);

  void EmitBarrier(xiiUInt32 uiConsumerPassIdx, xiiUInt32 uiResourceIdx, bool bIsTexture, xiiBitflags<xiiGALResourceStateFlags> afterState, bool bSplitBarrier, xiiUInt32 uiFirstMip = 0U, xiiUInt32 uiMipCount = XII_GAL_REMAINING_MIP_LEVELS, xiiUInt32 uiFirstSlice = 0U, xiiUInt32 uiSliceCount = XII_GAL_REMAINING_ARRAY_SLICES);

  [[nodiscard]] static xiiUInt64 ComputeSignature(const xiiDynamicArray<PassEntry>& passes);

private:
  xiiDynamicArray<PassEntry>               m_Passes;
  xiiDynamicArray<ResourceEntry>           m_Resources;
  xiiHashTable<xiiHashedString, xiiUInt32> m_ResourceNameIndex;
  xiiHashedString                          m_sName;
  xiiRenderGraphGraphId                    m_Id;
  xiiRenderGraphIdTable                    m_IdTable;

  xiiHashTable<xiiHashedString, xiiSharedPtr<xiiGALTexture>> m_PersistentTextures;
  xiiHashTable<xiiHashedString, xiiSharedPtr<xiiGALBuffer>>  m_PersistentBuffers;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALTexture>>       m_ExportedTextures;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiGALBuffer>>        m_ExportedBuffers;

  // Compiled outputs.
  xiiDynamicArray<xiiRenderGraphCompiledPass>               m_CompiledPasses;
  xiiDynamicArray<xiiRenderGraphBarrierDescription>         m_Barriers;
  xiiDynamicArray<xiiRenderGraphMergeGroup>                 m_MergeGroups;
  xiiDynamicArray<xiiRenderGraphQueueSubmission>            m_QueueSubmissions;
  xiiDynamicArray<xiiRenderGraphResourceDescription>        m_ResourceDescriptions;
  xiiDynamicArray<xiiRenderGraphResourceVersionDescription> m_ResourceVersions;

  xiiRenderGraphStatistics      m_Statistics;
  xiiRenderGraphCompileSettings m_LastCompileSettings;
  xiiUInt64                     m_uiLastSignature = 0ULL;
  xiiUInt64                     m_uiFrameIndex    = 0ULL;
  bool                          m_bIsSetupOpen    = false;
  bool                          m_bIsCompiled     = false;
};

#include <GraphicsCore/Pipeline/Implementation/RenderGraph_inl.h>
