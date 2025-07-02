#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/RenderPipelineNode.h>
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

/// \brief Passed to xiiRenderPipelinePass::InitRenderPipelinePass to inform about existing connections on each input / output pin index.
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

class XII_GRAPHICSCORE_DLL xiiRenderPipelinePass : public xiiRenderPipelineNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePass, xiiRenderPipelineNode);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderPipelinePass);

public:
  xiiRenderPipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassFlags> flags, xiiEnum<xiiRenderPipelinePassConcurrencyHint> concurrencyHint);
  ~xiiRenderPipelinePass();

  /// \brief Sets the name of the pass.
  void SetName(xiiStringView sName); // [ property ]

  void SetPassFlags(xiiBitflags<xiiRenderPipelinePassFlags> flags); // [ property ]

  void SetPassConcurrencyHint(xiiEnum<xiiRenderPipelinePassConcurrencyHint> concurrencyHint); // [ property ]

  /// \brief For a given input pin configuration, provide the output configuration of this node.
  /// Outputs is already resized to the number of output pins.
  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> pInputs, xiiArrayPtr<xiiGALTextureCreationDescription> pOutputs) = 0;

  /// Returns the current texture this node provides at the given *ProviderPin.
  /// This function is called every frame if this node holds a xiiRenderPipelineNodeInputProviderPin or xiiRenderPipelineNodeOutputProviderPin pin. The node can return a valid texture handle, or an invalid handle, in which case the missing texture will be created from the texture pool.
  /// \param pPin - The member pin for which the texture is requested.
  /// \param desc - The format of the texture that should be provided.
  /// \return The texture view to use for this pin's connections. Or invalid, in which case it reverts to a regular input / output pin.
  virtual xiiSharedPtr<xiiGALTextureView> QueryTextureProvider(const xiiRenderPipelineNodePin* pPin, const xiiGALTextureCreationDescription& desc) { return nullptr; }

  /// \brief After GetRenderTargetDescriptions was called successfully for each pass, this function is called with the inputs and outputs for review. Disconnected pins have a nullptr value in the passed in arrays.
  /// This is the time to create additional resources that are not covered by the pins automatically, e.g. a picking texture or eye adaptation buffer.
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs);

  /// \brief Render into outputs. Both inputs and outputs are passed in with actual texture handles.
  /// Disconnected pins have a nullptr value in the passed in arrays. You can now create views and render target setups on the fly and fill the output targets with data.
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) = 0;

  virtual void ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs);

  /// \brief Allows for the pass to write data back using xiiView::SetRenderPassReadBackProperty. E.g. picking results etc.
  virtual void ReadBackProperties(xiiView* pView);

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream);

  void RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList, xiiRenderData::Category category, xiiRenderDataBatch::Filter filter = xiiRenderDataBatch::Filter());

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

  xiiRenderPipeline* m_pPipeline = nullptr;
  bool               m_bActive   = true;
  xiiHashedString    m_sName;

  xiiBitflags<xiiRenderPipelinePassFlags>       m_PassFlags;
  xiiEnum<xiiRenderPipelinePassConcurrencyHint> m_PassConcurrencyHint;
};
