#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/RenderPipelineNode.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>

class xiiStreamWriter;

/// \brief This describes the render-pipeline pass behavior and optimizations to inform the render-pipeline compiler about per-pass requirements, capabilities, and tuning hints.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None              = 0U,         ///< No special behavior; execute at full resolution on the graphics queue, do not fuse with neighbors.
    StereoAware       = XII_BIT(0), ///< Pass correctly handles stereo/XR rendering, invoked per eye.
    AllowSubpassFuse  = XII_BIT(1), ///< Allows fusion with adjacent passes into a single GPU render-pass when attachments and load/store operations match.
    AsyncCompute      = XII_BIT(2), ///< Dispatch this pass on an asynchronous compute queue in parallel with graphics workloads.
    AsyncTransfer     = XII_BIT(3), ///< Dispatch this pass on an asynchronous transfer queue to overlap copies/blits with graphics/compute work.
    DynamicResolution = XII_BIT(4), ///< Enable dynamic resolution scaling to render into targets resized each frame by a runtime scale factor.
    DebugPass         = XII_BIT(5), ///< Mark this pass as debug-only; omit it in shipping or release builds.

    Default = None
  };

  struct Bits
  {
    StorageType StereoAware : 1;
    StorageType AllowSubpassFuse : 1;
    StorageType AsyncCompute : 1;
    StorageType AsyncTransfer : 1;
    StorageType DynamicResolution : 1;
    StorageType DebugPass : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderPipelinePassFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePassFlags);

struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassConcurrencyHint
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Sequential = 0U,     ///< Execute this pass in strict sequence; no overlap with any other pass.
    ParallelIndependent, ///< No resource hazards; can be scheduled in parallel with other passes.
    ParallelWithSync,    ///< Can overlap with other passes but requires semaphores/fences to synchronize data dependencies.

    ENUM_COUNT,

    Default = Sequential
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePassConcurrencyHint);

/// \brief Passed to xiiRenderPipelinePass::InitializeRenderPipelinePass to inform about existing connections on each input / output pin index.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassConnection
{
  xiiRenderPipelinePassConnection() :
    m_pOutput(nullptr)
  {
  }

  xiiGALTextureCreationDescription                   m_TextureDescription;
  xiiSharedPtr<xiiGALTexture>                        m_pTexture;
  const xiiRenderPipelineNodePin*                    m_pOutput; ///< The output pin that this connection spawns from.
  xiiHybridArray<const xiiRenderPipelineNodePin*, 4> m_Inputs;  ///< The various input pins this connection is connected to.
};

class XII_GRAPHICSCORE_DLL xiiRenderPipelinePassBase : public xiiRenderPipelineNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePassBase, xiiRenderPipelineNode);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderPipelinePassBase);

public:
  xiiRenderPipelinePassBase(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassFlags> flags, xiiEnum<xiiRenderPipelinePassConcurrencyHint> concurrencyHint);

  ~xiiRenderPipelinePassBase();

  void SetName(xiiStringView sName); // [ property ]

  void SetPassFlags(xiiBitflags<xiiRenderPipelinePassFlags> flags); // [ property ]

  void SetPassConcurrencyHint(xiiEnum<xiiRenderPipelinePassConcurrencyHint> concurrencyHint); // [ property ]

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream);

  virtual void InitializeRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs);

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) = 0;

  virtual void ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs);

  virtual void ReadBackProperties(xiiView* pView);

public:
  /// \brief Returns the name of this render-pipeline pass.
  ///
  /// \return A string view of the pass's name.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }; // [ property ]

  /// \brief Returns the bitmask of flags describing this render-pipeline pass.
  XII_ALWAYS_INLINE xiiBitflags<xiiRenderPipelinePassFlags> GetPassFlags() const { return m_PassFlags; } // [ property ]

  /// \brief Retrieves the concurrency hint for scheduling this render-pipeline pass.
  XII_ALWAYS_INLINE xiiEnum<xiiRenderPipelinePassConcurrencyHint> GetPassConcurrencyHint() const { return m_PassConcurrencyHint; } // [ property ]

  /// \brief Determines whether this pass correctly handles stereo/XR rendering.
  ///
  /// When true, the pipeline will invoke this pass once per eye and bind separate per-eye resources as needed.
  ///
  /// \return true if the pass is stereo-aware, false otherwise.
  XII_ALWAYS_INLINE bool IsStereoAware() const { return m_PassFlags.IsSet(xiiRenderPipelinePassFlags::StereoAware); }

  /// \brief Retrieves the owning render pipeline for this pass.
  ///
  /// Use this to query pipeline-level resources or state from within a pass implementation.
  ///
  /// \return A pointer to the parent xiiRenderPipeline instance.
  XII_ALWAYS_INLINE xiiRenderPipeline* GetPipeline() { return m_pPipeline; }

  /// \brief Retrieves the owning render pipeline for this pass (const overload).
  ///
  /// Allows read-only access to pipeline state from const contexts.
  ///
  /// \return A const pointer to the parent xiiRenderPipeline instance.
  XII_ALWAYS_INLINE const xiiRenderPipeline* GetPipeline() const { return m_pPipeline; }

private:
  friend class xiiRenderPipeline;

  xiiRenderPipeline*                            m_pPipeline = nullptr;
  bool                                          m_bActive   = true;
  xiiHashedString                               m_sName;
  xiiBitflags<xiiRenderPipelinePassFlags>       m_PassFlags;
  xiiEnum<xiiRenderPipelinePassConcurrencyHint> m_PassConcurrencyHint;
};

class XII_GRAPHICSCORE_DLL xiiGraphicsPipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGraphicsPipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiGraphicsPipelinePass);

public:

  void RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiRenderData::Category category, xiiRenderDataBatch::Filter filter = xiiRenderDataBatch::Filter());
};

class XII_GRAPHICSCORE_DLL xiiComputePipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiComputePipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiComputePipelinePass);

public:
};

class XII_GRAPHICSCORE_DLL xiiCopyPipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCopyPipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCopyPipelinePass);

public:
};

class XII_GRAPHICSCORE_DLL xiiPresentPipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPresentPipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiPresentPipelinePass);

public:
};

class XII_GRAPHICSCORE_DLL xiiUtilityPipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUtilityPipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiUtilityPipelinePass);

public:
};
