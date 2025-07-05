#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

#include <GraphicsCore/Pipeline/Declarations.h>

class xiiRenderPipelineNode;

/// \brief Flags that describe the behavior and role of a node pin within the render pipeline.
///
/// These flags indicate whether a pin is an input, output, forwarder (pass-through), or a dynamic resource provider. They guide graph validation, UI rendering, and automatic resource connection logic.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodePinFlags
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodePinResourceType
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

struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiRenderPipelineNodePinFlags>    m_Flags;
  xiiEnum<xiiRenderPipelineNodePinResourceType> m_ResourceType;
  xiiUInt8                                      m_uiInputIndex  = 0xFFU;
  xiiUInt8                                      m_uiOutputIndex = 0xFFU;
  xiiRenderPipelineNode*                        m_pParent       = nullptr;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePin);

///////////////////////////////////////////////////////////////////////////////
// Input Pins
///////////////////////////////////////////////////////////////////////////////

/// \brief Consumes a general-purpose GPU buffer (uniforms, storage, vertex data, etc.).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputBufferPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputColourAttachmentPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an input pin that reads from a ColourAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputColourAttachmentPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::ColourAttachment;
  }

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;

  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiEnum<xiiSourceFormat>                m_Format                   = xiiSourceFormat::Default;              ///< The texture format this pass expects.
  xiiEnum<xiiGALMSAASampleCount>          m_SampleCount              = xiiGALMSAASampleCount::OneSample;      ///< Number of MSAA samples for this attachment.
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentLoadOperation  = xiiGALAttachmentLoadOperation::Load;   ///< Load operation applied at the start of the pass.
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStoreOperation = xiiGALAttachmentStoreOperation::Store; ///< Store operation applied at the end of the pass.
  xiiColor                                m_ClearColor               = xiiColor::Black;                       ///< Clear color used when load operation is Clear.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputColourAttachmentPin);

/// \brief Consumes a depth buffer for depth-based effects or tests.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputDepthAttachmentPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// \brief Constructs an input pin that reads from a DepthAttachment resource.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputDepthAttachmentPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::DepthAttachment;
  }

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;

  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiEnum<xiiSourceFormat>                m_Format                          = xiiSourceFormat::Default;              ///< The texture format this pass expects (depth-only or depth-stencil).
  xiiEnum<xiiGALMSAASampleCount>          m_SampleCount                     = xiiGALMSAASampleCount::OneSample;      ///< Number of MSAA samples for this attachment.
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentLoadOperation         = xiiGALAttachmentLoadOperation::Load;   ///< Load operation for the depth aspect at the start of the pass.
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStoreOperation        = xiiGALAttachmentStoreOperation::Store; ///< Store operation for the depth aspect at the end of the pass.
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentStencilLoadOperation  = xiiGALAttachmentLoadOperation::Load;   ///< Load operation for the stencil aspect at the start of the pass.
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStencilStoreOperation = xiiGALAttachmentStoreOperation::Store; ///< Store operation for the stencil aspect at the end of the pass.
  float                                   m_fDepthClearValue                = 1.0f;                                  ///< Clear value for depth when load operation is Clear.
  xiiUInt8                                m_uiStencilClearValue             = 0U;                                    ///< Clear value for stencil when load operation is Clear.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputDepthAttachmentPin);

/// \brief Consumes a sampler state for texture sampling configuration.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputSamplerPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputAccelerationStructurePin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputBufferPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputColourAttachmentPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputDepthAttachmentPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputSamplerPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputAccelerationStructurePin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodePassThroughBufferPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodePassThroughColourAttachmentPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodePassThroughDepthAttachmentPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodePassThroughSamplerPin : public xiiRenderPipelineNodePin
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
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodePassThroughAccelerationStructurePin : public xiiRenderPipelineNodePin
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
// Input-Provider Pins
///////////////////////////////////////////////////////////////////////////////

/// \brief Dynamically provides a buffer each frame (Input + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputBufferProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an input-provider pin for Buffer resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputBufferProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Buffer;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputBufferProviderPin);

/// \brief Dynamically provides a color render target each frame (Input + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputColourAttachmentProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an input-provider pin for ColourAttachment resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputColourAttachmentProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::ColourAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputColourAttachmentProviderPin);

/// \brief Dynamically provides a depth render target each frame (Input + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputDepthAttachmentProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an input-provider pin for DepthAttachment resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputDepthAttachmentProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::DepthAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputDepthAttachmentProviderPin);

/// \brief Dynamically provides a sampler state each frame (Input + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputSamplerProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an input-provider pin for Sampler resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputSamplerProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Sampler;
  }
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputSamplerProviderPin);

/// \brief Dynamically provides an acceleration structure each frame (Input + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeInputAccelerationStructureProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an input-provider pin for AccelerationStructure resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputAccelerationStructureProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Input | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::AccelerationStructure;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputAccelerationStructureProviderPin);

///////////////////////////////////////////////////////////////////////////////
// Output-Provider Pins
///////////////////////////////////////////////////////////////////////////////

/// \brief Dynamically provides a buffer each frame (Output + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputBufferProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an output-provider pin for Buffer resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputBufferProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Buffer;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputBufferProviderPin);

/// \brief Dynamically provides a color render target each frame (Output + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputColourAttachmentProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an output-provider pin for ColourAttachment resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputColourAttachmentProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::ColourAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputColourAttachmentProviderPin);

/// \brief Dynamically provides a depth render target each frame (Output + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputDepthAttachmentProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an output-provider pin for DepthAttachment resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputDepthAttachmentProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::DepthAttachment;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputDepthAttachmentProviderPin);

/// \brief Dynamically provides a sampler state each frame (Output + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputSamplerProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an output-provider pin for Sampler resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputSamplerProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::Sampler;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputSamplerProviderPin);

/// \brief Dynamically provides a ray-tracing acceleration structure each frame (Output + Provider).
struct XII_GRAPHICSCORE_DLL xiiRenderPipelineNodeOutputAccelerationStructureProviderPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  /// Constructs an output-provider pin for AccelerationStructure resources.
  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputAccelerationStructureProviderPin()
  {
    m_Flags        = xiiRenderPipelineNodePinFlags::Output | xiiRenderPipelineNodePinFlags::ResourceProvider;
    m_ResourceType = xiiRenderPipelineNodePinResourceType::AccelerationStructure;
  }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputAccelerationStructureProviderPin);

/// \brief Base class for all nodes in the render pipeline graph.
///
/// Each node owns a set of input and output pins and provides lookup and initialization logic. Nodes are reflectable via RTTI to support editor integration and serialization.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineNode, xiiReflectedClass);

public:
  /// \brief Virtual destructor.
  virtual ~xiiRenderPipelineNode() = default;

  /// \brief Populates the internal arrays of input and output pins.
  ///
  /// This must be called after all pins have been created or registered so that GetInputPins() and GetOutputPins() return valid data.
  void InitializePins();

  /// \brief Returns the hashed name associated with a given pin.
  ///
  /// \param pPin - Pointer to the pin whose name is queried.
  ///
  /// \returns The hashed string name for the specified pin, or an empty hash if not found.
  xiiHashedString GetPinName(const xiiRenderPipelineNodePin* pPin) const;

  /// \brief Finds a pin by its string name.
  ///
  /// \param sName - The textual name of the pin to search for.
  ///
  /// \returns Pointer to the matching pin, or nullptr if no pin with that name exists.
  const xiiRenderPipelineNodePin* GetPinByName(xiiStringView sName) const;

  /// \brief Finds a pin by its hashed name.
  ///
  /// \param sName - The hashed name of the pin to search for.
  ///
  /// \returns Pointer to the matching pin, or nullptr if no pin with that hash exists.
  const xiiRenderPipelineNodePin* GetPinByName(xiiHashedString sName) const;

  /// \brief Retrieves all input pins of this node.
  ///
  /// \returns An array pointer to the collection of input pin pointers.
  XII_ALWAYS_INLINE const xiiArrayPtr<const xiiRenderPipelineNodePin* const> GetInputPins() const { return m_InputPins; }

  /// \brief Retrieves all output pins of this node.
  ///
  /// \returns An array pointer to the collection of output pin pointers.
  XII_ALWAYS_INLINE const xiiArrayPtr<const xiiRenderPipelineNodePin* const> GetOutputPins() const { return m_OutputPins; }

private:
  /// \brief Holds pointers to all input pins for quick iteration.
  xiiDynamicArray<const xiiRenderPipelineNodePin*> m_InputPins;

  /// \brief Holds pointers to all output pins for quick iteration.
  xiiDynamicArray<const xiiRenderPipelineNodePin*> m_OutputPins;

  /// \brief Maps pin names to their corresponding pin pointers.
  xiiHashTable<xiiHashedString, const xiiRenderPipelineNodePin*> m_NameToPin;
};
