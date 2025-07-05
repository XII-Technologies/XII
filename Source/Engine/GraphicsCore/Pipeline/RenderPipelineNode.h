#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

#include <GraphicsCore/Pipeline/Declarations.h>

class xiiRenderPipelineNode;

/// \brief Flags that describe the behavior and role of a node pin within the render pipeline.
///
/// These flags indicate whether a pin is an input, output, forwarder (pass-through), or a dynamic resource provider. They guide graph validation, UI rendering, and automatic resource connection logic.
struct xiiRenderPipelineNodePinFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown          = 0U,         ///< \brief Pin role is not specified. Treated as invalid in most contexts.
    Input            = XII_BIT(0), ///< \brief Marks this pin as an input to a node. Typically receives a texture, buffer, or resource from an upstream node. Input pins are eligible for connections from Output or PassThrough pins.
    Output           = XII_BIT(1), ///< \brief Marks this pin as an output from a node. Produces a new resource to be consumed downstream (e.g., a rendered image or computed buffer).
    PassThrough      = XII_BIT(2), ///< \brief Declares that this pin forwards data unmodified. Used to route a resource through a node without consuming or writing it (e.g. forwarding a shadow map).
    ResourceProvider = XII_BIT(3), ///< \brief Indicates that the node dynamically provides or overrides a resource at runtime. Typically used for nodes that bind external inputs such as camera buffers, reflection probes, or user-defined resources.

    Default = Unknown
  };

  struct Bits
  {
    StorageType Input : 1;
    StorageType Output : 1;
    StorageType PassThrough : 1;
    StorageType ResourceProvider : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderPipelineNodePinFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePinFlags);

/// \brief Describes the kind of resource a render graph node pin accepts or produces.
///
/// These values determine wiring compatibility between nodes, as well as how the pipeline compiler treats resource lifetimes, layout transitions, and usage roles.
///
/// \note These types are semantic, not strictly tied to GAL or low-level GPU types. For example, both ColourAttachment and DepthAttachment may be implemented as textures in the backend.
struct xiiRenderPipelineNodePinResourceType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0,           ///< Invalid or unspecified type. Treated as non-connectable.
    Buffer,                ///< General-purpose GPU buffer (uniforms, storage, vertex, etc.)
    ColourAttachment,      ///< Render target image: written as output or sampled later in post-processing. Typically bound as RTV or sampled as SRV.
    DepthAttachment,       ///< Used for depth testing or writing depth information. Format may or may not include stencil. Typically bound as DSV or depth-sampling input.
    Sampler,               ///< Describes sampling behavior (filtering, wrap modes). Connected to sampler inputs in shading nodes.
    AccelerationStructure, ///< Ray-tracing acceleration structure. Used in ray-gen, closest-hit, etc. shaders.
    StorageImage,          ///< UAV-capable image used in compute or deferred passes. Includes imageStore access. Useful to distinguish from sampled-only ColourAttachment.
    ReadWriteBuffer,       ///< Read/Write buffer accessed via UAV. Used in compute shaders or GPU-driven systems.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePinResourceType);

struct xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;

  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiBitflags<xiiRenderPipelineNodePinFlags>    m_Flags;
  xiiEnum<xiiRenderPipelineNodePinResourceType> m_ResourceType;
  xiiUInt8                                      m_uiInputIndex  = 0xFFU;
  xiiUInt8                                      m_uiOutputIndex = 0xFFU;
  xiiRenderPipelineNode*                        m_pParent       = nullptr;

  xiiEnum<xiiSourceFormat>                m_Format                          = xiiSourceFormat::Default;
  xiiEnum<xiiGALMSAASampleCount>          m_SampleCount                     = xiiGALMSAASampleCount::OneSample;
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentLoadOperation         = xiiGALAttachmentLoadOperation::Load;
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStoreOperation        = xiiGALAttachmentStoreOperation::Store;
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentStencilLoadOperation  = xiiGALAttachmentLoadOperation::Load;
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStencilStoreOperation = xiiGALAttachmentStoreOperation::Store;
  xiiColor                                m_ClearColor                      = xiiColor::Black;
  float                                   m_fDepthClearValue                = 1.0f;
  xiiUInt8                                m_uiStencilClearValue             = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePin);

///////////////////////////////////////////////////////////////////////////////
// Input Pins
///////////////////////////////////////////////////////////////////////////////

/// \brief Consumes a general-purpose GPU buffer (uniforms, storage, vertex data, etc.).
struct xiiRenderPipelineNodeInputBufferPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an input pin that reads from a Buffer resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputBufferPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Buffer;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputBufferPin);

/// \brief Consumes a color render target (e.g. MRT, texture for post-process).
struct xiiRenderPipelineNodeInputColourAttachmentPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an input pin that reads from a ColourAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputColourAttachmentPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::ColourAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputColourAttachmentPin);

/// \brief Consumes a depth buffer for depth-based effects or tests.
struct xiiRenderPipelineNodeInputDepthAttachmentPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an input pin that reads from a DepthAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputDepthAttachmentPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::DepthAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputDepthAttachmentPin);

/// \brief Consumes a sampler state for texture sampling configuration.
struct xiiRenderPipelineNodeInputSamplerPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an input pin that reads from a Sampler resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputSamplerPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Sampler;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputSamplerPin);

/// \brief Consumes a ray-tracing acceleration structure.
struct xiiRenderPipelineNodeInputAccelerationStructurePin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an input pin that reads from an AccelerationStructure resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputAccelerationStructurePin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::AccelerationStructure;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputAccelerationStructurePin);

///////////////////////////////////////////////////////////////////////////////
// Output Pins
///////////////////////////////////////////////////////////////////////////////

/// \brief Produces a general-purpose GPU buffer (uniforms, storage, vertex data, etc.).
struct xiiRenderPipelineNodeOutputBufferPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an output pin that writes to a Buffer resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputBufferPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Buffer;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputBufferPin);

/// \brief Produces a color render target (e.g. MRT, texture for post-process).
struct xiiRenderPipelineNodeOutputColourAttachmentPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an output pin that writes to a ColourAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputColourAttachmentPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::ColourAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputColourAttachmentPin);

/// \brief Produces a depth buffer for depth-based effects or tests.
struct xiiRenderPipelineNodeOutputDepthAttachmentPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an output pin that writes to a DepthAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputDepthAttachmentPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::DepthAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputDepthAttachmentPin);

/// \brief Produces a sampler state for texture sampling configuration.
struct xiiRenderPipelineNodeOutputSamplerPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an output pin that writes to a Sampler resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputSamplerPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Sampler;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputSamplerPin);

/// \brief Produces a ray-tracing acceleration structure.
struct xiiRenderPipelineNodeOutputAccelerationStructurePin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an output pin that writes to an AccelerationStructure resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputAccelerationStructurePin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::AccelerationStructure;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputAccelerationStructurePin);

///////////////////////////////////////////////////////////////////////////////
// Pass-Through Pins
///////////////////////////////////////////////////////////////////////////////

/// \brief Forwards a general-purpose GPU buffer without modifying it.
struct xiiRenderPipelineNodePassThroughBufferPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a pass-through pin for a Buffer resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodePassThroughBufferPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::PassThrough;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Buffer;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePassThroughBufferPin);

/// \brief Forwards a color render target without modifying it.
struct xiiRenderPipelineNodePassThroughColourAttachmentPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a pass-through pin for a ColourAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodePassThroughColourAttachmentPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::PassThrough;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::ColourAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePassThroughColourAttachmentPin);

/// \brief Forwards a depth buffer without modifying it.
struct xiiRenderPipelineNodePassThroughDepthAttachmentPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a pass-through pin for a DepthAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodePassThroughDepthAttachmentPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::PassThrough;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::DepthAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePassThroughDepthAttachmentPin);

/// \brief Forwards a sampler state without modifying it.
struct xiiRenderPipelineNodePassThroughSamplerPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a pass-through pin for a Sampler resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodePassThroughSamplerPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::PassThrough;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Sampler;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePassThroughSamplerPin);

/// \brief Forwards an acceleration structure without modifying it.
struct xiiRenderPipelineNodePassThroughAccelerationStructurePin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a pass-through pin for an AccelerationStructure resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodePassThroughAccelerationStructurePin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::PassThrough;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::AccelerationStructure;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePassThroughAccelerationStructurePin);

///////////////////////////////////////////////////////////////////////////////
// Resource Provider Pins
///////////////////////////////////////////////////////////////////////////////

/// \brief Dynamically provides a general-purpose GPU buffer each frame.
struct xiiRenderPipelineNodeBufferProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a provider pin for a Buffer resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeBufferProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Buffer;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeBufferProviderPin);

/// \brief Dynamically provides a color render target each frame.
struct xiiRenderPipelineNodeColourAttachmentProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a provider pin for a ColourAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeColourAttachmentProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::ColourAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeColourAttachmentProviderPin);

/// \brief Dynamically provides a depth buffer each frame.
struct xiiRenderPipelineNodeDepthAttachmentProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a provider pin for a DepthAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeDepthAttachmentProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::DepthAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeDepthAttachmentProviderPin);

/// \brief Dynamically provides a sampler state each frame.
struct xiiRenderPipelineNodeSamplerProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a provider pin for a Sampler resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeSamplerProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Sampler;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeSamplerProviderPin);

/// \brief Dynamically provides a ray-tracing acceleration structure each frame.
struct xiiRenderPipelineNodeAccelerationStructureProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs a provider pin for an AccelerationStructure resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeAccelerationStructureProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::AccelerationStructure;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeAccelerationStructureProviderPin);

class XII_GRAPHICSCORE_DLL xiiRenderPipelineNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineNode, xiiReflectedClass);

public:
  virtual ~xiiRenderPipelineNode() = default;

  void InitializePins();

  xiiHashedString                 GetPinName(const xiiRenderPipelineNodePin* pPin) const;
  const xiiRenderPipelineNodePin* GetPinByName(xiiStringView sName) const;
  const xiiRenderPipelineNodePin* GetPinByName(xiiHashedString sName) const;

  XII_ALWAYS_INLINE const xiiArrayPtr<const xiiRenderPipelineNodePin* const> GetInputPins() const { return m_InputPins; }
  XII_ALWAYS_INLINE const xiiArrayPtr<const xiiRenderPipelineNodePin* const> GetOutputPins() const { return m_OutputPins; }

private:
  xiiDynamicArray<const xiiRenderPipelineNodePin*>               m_InputPins;
  xiiDynamicArray<const xiiRenderPipelineNodePin*>               m_OutputPins;
  xiiHashTable<xiiHashedString, const xiiRenderPipelineNodePin*> m_NameToPin;
};
