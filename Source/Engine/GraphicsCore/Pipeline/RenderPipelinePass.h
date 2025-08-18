#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/RenderPipelineNode.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Sampler.h>

class xiiStreamWriter;
struct xiiRenderPipelinePassResource;

/// \brief Declares the fixed capabilities of a render-pipeline pass.
///
/// These flags describe immutable attributes of a pass that are determined by its implementation. They inform the graph compiler about how the pass can be optimized or scheduled.
///
/// Unlike xiiRenderPipelinePassFlags, these capabilities cannot be changed at runtime or in the editor.
///
/// Usage examples:
/// - StereoAware: Pass supports multi-eye rendering and will be invoked per eye.
/// - AllowSubpassFuse: Permits merging this pass with adjacent ones into a single hardware render pass if compatible.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassCapabilityFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None             = 0U,         ///< No declared capabilities.
    StereoAware      = XII_BIT(0), ///< Pass handles stereo rendering correctly; invoked per eye when rendering for XR.
    AllowSubpassFuse = XII_BIT(1), ///< Indicates pass is eligible for subpass fusion with adjacent passes if attachment usage matches.

    Default = None
  };

  struct Bits
  {
    StorageType StereoAware : 1;
    StorageType AllowSubpassFuse : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderPipelinePassCapabilityFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePassCapabilityFlags);

/// \brief Declares runtime-configurable behavior and tuning hints for a render-pipeline pass.
///
/// These flags inform the render-graph compiler or scheduler about how a pass should be dispatched and whether it should support dynamic features or be excluded from release builds.
///
/// Unlike xiiRenderPipelinePassCapabilityFlags, these flags can be toggled per pass instance.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None              = 0U,         ///< No behavioral overrides; pass runs on graphics queue without fusion or dynamic scaling.
    AsyncCompute      = XII_BIT(0), ///< Run this pass on a compute queue asynchronously from graphics work.
    AsyncTransfer     = XII_BIT(1), ///< Schedule this pass on a dedicated transfer queue for background data movement.
    DynamicResolution = XII_BIT(2), ///< Allow targets used in this pass to be dynamically resized at runtime (e.g. based on performance).
    DebugPass         = XII_BIT(3), ///< This pass is for debugging or visualization; skip it in shipping builds.

    Default = None
  };

  struct Bits
  {
    StorageType AsyncCompute : 1;
    StorageType AsyncTransfer : 1;
    StorageType DynamicResolution : 1;
    StorageType DebugPass : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderPipelinePassFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePassFlags);

/// \brief Describes the concurrency behavior expected of a render pipeline pass during scheduling.
///
/// This hint informs the render graph compiler whether the pass can be overlapped with others, and if so, whether synchronization mechanisms are required.
///
/// Unlike queue-type hints (e.g., compute vs graphics), this models scheduling *independence*, allowing fine control over pass parallelism.
///
/// Typical usage:
/// - Use Sequential for passes with resource or ordering constraints.
/// - Use ParallelIndependent for stateless or purely additive passes.
/// - Use ParallelWithSync when interleaving is okay but hazards must be synchronized.
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

/// \brief Describes a request to create or resolve a GPU resource during render pipeline compilation.
///
/// This structure encapsulates both the intended resource type and its creation parameters, and is typically used by resource provider interfaces or pass hooks (e.g. QueryResourceProvider).
///
/// Only one variant inside the union is valid at a time, as determined by m_Type.
///
/// Example usage:
/// - A pass emitting a Texture pin may populate m_Texture with desired resolution, format, and usage.
/// - The graph compiler or engine module inspects the request and returns an appropriate runtime handle.
///
/// \see xiiRenderPipelinePassResource, xiiRenderPipelinePassBase::QueryResourceProvider
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineResourceRequest
{
  xiiRenderPipelineResourceRequest();
  xiiRenderPipelineResourceRequest(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALBufferCreationDescription& description);
  xiiRenderPipelineResourceRequest(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALTextureCreationDescription& description);
  xiiRenderPipelineResourceRequest(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALSamplerCreationDescription& description);
  xiiRenderPipelineResourceRequest(const xiiRenderPipelinePassResource& passResource);

  xiiEnum<xiiRenderPipelineNodePinResourceType> m_Type; ///< Indicates which union field is valid.

  /// \brief Returns true if this resource request is a buffer.
  XII_ALWAYS_INLINE constexpr bool IsBuffer() const { return m_Type == xiiRenderPipelineNodePinResourceType::Buffer || m_Type == xiiRenderPipelineNodePinResourceType::ReadWriteBuffer; }

  /// \brief Returns true if this resource request is a texture.
  XII_ALWAYS_INLINE constexpr bool IsTexture() const { return m_Type == xiiRenderPipelineNodePinResourceType::ColourAttachment || m_Type == xiiRenderPipelineNodePinResourceType::DepthAttachment; }

  /// \brief Returns true if this resource request is a sampler.
  XII_ALWAYS_INLINE constexpr bool IsSampler() const { return m_Type == xiiRenderPipelineNodePinResourceType::Sampler; }

  /// \brief Returns true if this resource request is an acceleration structure.
  XII_ALWAYS_INLINE constexpr bool IsAccelerationStructure() const { return m_Type == xiiRenderPipelineNodePinResourceType::AccelerationStructure; }

  union
  {
    xiiGALTextureCreationDescription m_Texture; ///< Texture creation parameters.
    xiiGALBufferCreationDescription  m_Buffer;  ///< Buffer creation parameters.
    xiiGALSamplerCreationDescription m_Sampler; ///< Sampler creation parameters.
  };
};

/// \brief Represents a strongly typed resource instance produced by a render pipeline pass, including its creation parameters and resolved runtime handle.
///
/// This structure is typically embedded in a xiiRenderPipelinePassConnection to describe the output produced by a specific output pin. It acts as a type-safe union containing
/// exactly one valid resource type (Texture, Buffer, Sampler, or Acceleration Structure), along with its associated descriptor and GPU handle.
///
/// The active resource variant is indicated by m_Type. Accessing an inactive variant is undefined behavior.
///
/// \see xiiRenderPipelineNodePinResourceType
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassResource
{
  xiiRenderPipelinePassResource();
  xiiRenderPipelinePassResource(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALBufferCreationDescription& description, const xiiSharedPtr<xiiGALBuffer>& pBuffer = {});
  xiiRenderPipelinePassResource(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALTextureCreationDescription& description, const xiiSharedPtr<xiiGALTexture>& pTexture = {});
  xiiRenderPipelinePassResource(xiiRenderPipelineNodePinResourceType::Enum resourceType, const xiiGALSamplerCreationDescription& description, const xiiSharedPtr<xiiGALSampler>& pSampler = {});
  xiiRenderPipelinePassResource(const xiiRenderPipelinePassResource& other);
  ~xiiRenderPipelinePassResource();

  xiiRenderPipelinePassResource& operator=(const xiiRenderPipelinePassResource& other);

  xiiUInt32 CalculateDescriptorHash() const;

  /// \brief Returns true if this resource is a buffer.
  XII_ALWAYS_INLINE constexpr bool IsBuffer() const { return m_Type == xiiRenderPipelineNodePinResourceType::Buffer || m_Type == xiiRenderPipelineNodePinResourceType::ReadWriteBuffer; }

  /// \brief Returns true if this resource is a texture.
  XII_ALWAYS_INLINE constexpr bool IsTexture() const { return m_Type == xiiRenderPipelineNodePinResourceType::ColourAttachment || m_Type == xiiRenderPipelineNodePinResourceType::DepthAttachment; }

  /// \brief Returns true if this resource is a sampler.
  XII_ALWAYS_INLINE constexpr bool IsSampler() const { return m_Type == xiiRenderPipelineNodePinResourceType::Sampler; }

  /// \brief Returns true if this resource is an acceleration structure.
  XII_ALWAYS_INLINE constexpr bool IsAccelerationStructure() const { return m_Type == xiiRenderPipelineNodePinResourceType::AccelerationStructure; }

  xiiEnum<xiiRenderPipelineNodePinResourceType> m_Type; ///< Indicates which union field is valid.

  union
  {
    struct
    {
      xiiGALTextureCreationDescription m_Description; ///< Describes the texture format, usage, size, etc.
      xiiSharedPtr<xiiGALTexture>      m_pTexture;    ///< The actual resolved GPU texture.
    } m_Texture;

    struct
    {
      xiiGALBufferCreationDescription m_Description; ///< Buffer configuration (element size, usage, etc.).
      xiiSharedPtr<xiiGALBuffer>      m_pBuffer;     ///< The runtime buffer handle.
    } m_Buffer;

    struct
    {
      xiiGALSamplerCreationDescription m_Description; ///< Sampler filtering, addressing, and LOD parameters.
      xiiSharedPtr<xiiGALSampler>      m_pSampler;    ///< The compiled sampler object used in shaders.
    } m_Sampler;
  };
};

/// \brief Describes a resolved link between one output pin and one or more input pins in the render pipeline graph.
///
/// This structure is created by the graph compiler during pass initialization and represents a single directed edge in the dataflow graph, connecting a producing pass (via its output pin)
/// to one or more consuming passes (via their input pins).
///
/// It also holds the fully resolved resource (texture, buffer, etc.) and its creation metadata.
///
/// \see xiiRenderPipelinePassResource
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassConnection
{
  const xiiRenderPipelineNodePin*                    m_pOutput = nullptr; ///< The originating output pin that produces the resource.
  xiiHybridArray<const xiiRenderPipelineNodePin*, 4> m_Inputs;            ///< All input pins across other passes that consume this resource. Each pin will receive the same shared resource instance declared in m_Resource.
  xiiRenderPipelinePassResource                      m_Resource;          ///< The resolved GPU resource produced by the output pin, including descriptor and handle. Only one active resource type is valid, as indicated by m_Resource.m_Type.
};

/// \brief Base class for render pipeline passes within a data-driven render graph system.
///
/// Each render pass defines its inputs, outputs, capabilities, and runtime execution logic. This class handles naming, serialization, and interaction with the render pipeline compiler.
///
/// Passes are responsible for describing their resource requirements, initializing resources and logic based on resolved graph connections, and executing their logic during render view processing.
///
/// Derive from this class to implement custom rendering functionality.
///
/// \see xiiRenderPipelineNode, xiiRenderPipelinePassConnection, xiiRenderPipelinePassResource
class XII_GRAPHICSCORE_DLL xiiRenderPipelinePassBase : public xiiRenderPipelineNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePassBase, xiiRenderPipelineNode);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderPipelinePassBase);

public:
  /// \brief Constructor to define a named render pass with required capability flags.
  ///
  /// \param sName           - The name identifier of the pass (used for debugging and editor integration).
  /// \param capabilityFlags - Declares what features this pass supports (e.g., stereo-aware, subpass fusion).
  xiiRenderPipelinePassBase(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags);

  /// \brief Virtual destructor.
  virtual ~xiiRenderPipelinePassBase();

  /// \brief Assigns the user-visible name of the pass.
  void SetName(xiiStringView sName); // [ property ]

  /// \brief Sets additional flags that describe how the pass behaves or should be scheduled.
  void SetPassFlags(xiiBitflags<xiiRenderPipelinePassFlags> flags); // [ property ]

  /// \brief Provides a hint to the graph scheduler regarding parallel execution potential.
  void SetPassConcurrencyHint(xiiEnum<xiiRenderPipelinePassConcurrencyHint> concurrencyHint); // [ property ]

  /// \brief Serializes the internal pass configuration (not resource or runtime state).
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const;

  /// \brief Restores the internal pass configuration from serialized data.
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream);

  /// \brief Must be implemented by each pass to describe the GPU resources it requires on its output pins.
  ///
  /// The graph compiler uses this to allocate or pool GPU resources like textures, samplers, and buffers.
  /// Derived passes should populate each entry in \a pOutputs with the corresponding resource description.
  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) = 0;

  /// \brief Called once before the first execution to allow the pass to initialize itself based on the graph and view context.
  ///
  /// This is typically used to:
  /// - Resolve input/output resource handles from connections.
  /// - Cache relevant view properties.
  /// - Prepare internal state (e.g., sampler bindings, descriptor layouts).
  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs);

  /// \brief Allows a pass to override resource creation for one of its outputs, typically to reuse or share GPU resources.
  ///
  /// If a non-null shared pointer is returned, it will be used instead of the default resource construction logic.
  /// Returning `nullptr` delegates resource creation to the graph compiler or the default allocator.
  virtual xiiSharedPtr<xiiGALDeviceObject> QueryResourceProvider(const xiiRenderPipelineNodePin* pPin, const xiiRenderPipelineResourceRequest& request);

  /// \brief Must be implemented by derived classes to perform the pass's actual rendering or processing logic.
  ///
  /// Called once per frame (and per view, if applicable).
  ///
  /// Inputs and outputs are guaranteed to be valid and resolved GPU handles.
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) = 0;

  /// \brief Called instead of Execute() if the pass is inactive (e.g., temporarily disabled or culled).
  ///
  /// Can be used for non-draw side effects such as animation updates or property blending.
  virtual void ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs);

  /// \brief Allows the pass to update scene or view properties after rendering (e.g., exposure, feedback).
  virtual void ReadBackProperties(xiiView* pView);

public:
  /// \brief Returns the name of this render-pipeline pass.
  ///
  /// \return A string view of the pass's name.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }; // [ property ]

  /// \brief Retrieves the render pipeline pass capability flags.
  XII_ALWAYS_INLINE xiiBitflags<xiiRenderPipelinePassCapabilityFlags> GetCapabilityFlags() const { return m_CapabilityFlags; }

  /// \brief Returns the bitmask of flags describing this render-pipeline pass.
  XII_ALWAYS_INLINE xiiBitflags<xiiRenderPipelinePassFlags> GetPassFlags() const { return m_PassFlags; } // [ property ]

  /// \brief Retrieves the concurrency hint for scheduling this render-pipeline pass.
  XII_ALWAYS_INLINE xiiEnum<xiiRenderPipelinePassConcurrencyHint> GetPassConcurrencyHint() const { return m_PassConcurrencyHint; } // [ property ]

  /// \brief Determines whether this pass correctly handles stereo/XR rendering.
  ///
  /// When true, the pipeline will invoke this pass once per eye and bind separate per-eye resources as needed.
  ///
  /// \return true if the pass is stereo-aware, false otherwise.
  XII_ALWAYS_INLINE bool IsStereoAware() const { return m_CapabilityFlags.IsSet(xiiRenderPipelinePassCapabilityFlags::StereoAware); }

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

  xiiRenderPipeline*                                m_pPipeline = nullptr;
  bool                                              m_bActive   = true;
  xiiHashedString                                   m_sName;
  xiiBitflags<xiiRenderPipelinePassCapabilityFlags> m_CapabilityFlags;
  xiiBitflags<xiiRenderPipelinePassFlags>           m_PassFlags;
  xiiEnum<xiiRenderPipelinePassConcurrencyHint>     m_PassConcurrencyHint;
};

class XII_GRAPHICSCORE_DLL xiiGraphicsPipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGraphicsPipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiGraphicsPipelinePass);

public:
  xiiGraphicsPipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags);

  virtual ~xiiGraphicsPipelinePass();

  void RenderDataWithCategory(const xiiRenderViewContext& renderViewContext, xiiRenderData::Category category);
};

class XII_GRAPHICSCORE_DLL xiiComputePipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiComputePipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiComputePipelinePass);

public:
  xiiComputePipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags);

  virtual ~xiiComputePipelinePass();
};

class XII_GRAPHICSCORE_DLL xiiCopyPipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCopyPipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiCopyPipelinePass);

public:
  xiiCopyPipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags = xiiRenderPipelinePassCapabilityFlags::StereoAware);

  virtual ~xiiCopyPipelinePass();
};

class XII_GRAPHICSCORE_DLL xiiPresentPipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPresentPipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiPresentPipelinePass);

public:
  xiiPresentPipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags = xiiRenderPipelinePassCapabilityFlags::StereoAware);

  virtual ~xiiPresentPipelinePass();
};

class XII_GRAPHICSCORE_DLL xiiUtilityPipelinePass : public xiiRenderPipelinePassBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUtilityPipelinePass, xiiRenderPipelinePassBase);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiUtilityPipelinePass);

public:
  xiiUtilityPipelinePass(xiiStringView sName, xiiBitflags<xiiRenderPipelinePassCapabilityFlags> capabilityFlags = xiiRenderPipelinePassCapabilityFlags::StereoAware);

  virtual ~xiiUtilityPipelinePass();
};
