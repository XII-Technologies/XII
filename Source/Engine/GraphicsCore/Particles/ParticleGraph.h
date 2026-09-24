/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>
#include <GraphicsCore/Declarations.h>

using xiiParticleGraphResourceHandle = xiiTypedResourceHandle<class xiiParticleGraphResource>;

/// Attribute data format used by particle graph pins and GPU attribute streams.
struct XII_GRAPHICSCORE_DLL xiiParticleAttributeFormat
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Float = 0U, ///< 32-bit float, or 16-bit normalized (for vertex attributes).
    Float2,     ///< 2-component vector of 32-bit floats, or 16-bit normalized (for vertex attributes).
    Float3,     ///< 3-component vector of 32-bit floats, or 16-bit normalized (for vertex attributes).
    Float4,     ///< 4-component vector of 32-bit floats, or 16-bit normalized (for vertex attributes).
    UInt,       ///< 32-bit unsigned integer.
    UInt2,      ///< 2-component vector of 32-bit unsigned integers.
    UInt4,      ///< 4-component vector of 32-bit unsigned integers. Note that 3-component uint attributes are not natively supported by GPU APIs and should use UInt4 with the unused component set to zero.
    Int,        ///< 32-bit signed integer.
    Int2,       ///< 2-component vector of 32-bit signed integers.
    Int4,       ///< 4-component vector of 32-bit signed integers. Note that 3-component int attributes are not natively supported by GPU APIs and should use Int4 with the unused component set to zero.

    ENUM_COUNT,

    Default = Float4
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleAttributeFormat);

/// Built-in particle attribute semantics. Custom attributes are named by xiiParticleGraphPinDesc::m_sAttributeName.
struct XII_GRAPHICSCORE_DLL xiiParticleAttributeSemantic
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Custom = 0U,      ///< Custom attribute with user-defined name.
    Position,         ///< Particle position in simulation space.
    PreviousPosition, ///< Particle position in the previous simulation step, used for integration and collision.
    Velocity,         ///< Particle velocity vector.
    Acceleration,     ///< Particle acceleration vector, typically accumulated from forces.
    Mass,             ///< Particle mass, used for physics calculations. Zero mass can indicate an immovable object.
    InverseMass,      ///< Precomputed inverse of mass, used to optimize physics calculations. Zero inverse mass indicates an immovable object.
    Radius,           ///< Particle radius, used for rendering and collision. Note that non-spherical particles may need custom attributes for their dimensions or orientation.
    Color,            ///< Particle color, typically in RGBA format. Can be used for rendering or as a general-purpose attribute for graph logic.
    Age,              ///< Time since the particle was emitted, in seconds. Can be used for lifetime-based behaviors or effects.
    Lifetime,         ///< Total lifetime of the particle, in seconds. When Age exceeds Lifetime, the particle is typically killed.
    Orientation,      ///< Particle orientation, typically represented as a quaternion (x, y, z, w) or Euler angles. Used for rendering and oriented effects.
    AngularVelocity,  ///< Particle angular velocity, typically represented as a vector (x, y, z) for axis-angle or (x, y, z, w) for quaternion derivatives. Used for rotational motion.
    Temperature,      ///< Particle temperature, used for effects like heat distortion or phase changes.
    Density,          ///< Particle density, used for fluid simulations or effects that depend on local particle concentration.
    Charge,           ///< Particle electric charge, used for electrostatic forces or effects.
    MoleculeId,       ///< Identifier for molecular dynamics simulations, used to group particles into molecules or track interactions.
    CellId,           ///< Identifier for spatial partitioning cells, used for neighbor searches or grid-based effects.
    SortKey,          ///< Precomputed sort key for depth sorting in GPU rendering. Typically a 32-bit unsigned integer encoding depth or distance from the camera.

    ENUM_COUNT,

    Default = Custom
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleAttributeSemantic);

/// High-level graph node category for editor grouping and compile scheduling.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphNodeCategory
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Emitter = 0U, ///< Nodes that spawn new particles, typically at the beginning of the graph.
    Initialize,   ///< Nodes that set initial state for particles, typically after emission.
    Simulation,   ///< Nodes that perform per-frame simulation updates, typically in the middle of the graph.
    Solver,       ///< Nodes that perform constraint solving or iterative updates, typically after the main simulation step.
    Constraint,   ///< Nodes that apply constraints to particle state, such as collision response or custom rules, typically after solvers.
    Collision,    ///< Nodes that perform collision detection and response, typically after solvers or as part of constraint solving.
    Event,        ///< Nodes that emit or consume events, which can be used for communication between nodes or with the game engine, typically at any point in the graph.
    Render,       ///< Nodes that prepare particle data for rendering, such as billboard generation or GPU sorting, typically at the end of the graph.
    Utility,      ///< Nodes that perform general-purpose data processing or graph logic, such as math operations, attribute manipulation, or flow control. Can be scheduled flexibly based on their function and dependencies.
    IO,           ///< Nodes that perform input/output operations, such as reading/writing buffers, loading resources, or interfacing with engine systems. Can be scheduled flexibly based on their function and dependencies.
    Custom,       ///< Custom category for user-defined nodes that don't fit into the above categories. Scheduling can be determined by user-defined rules or editor settings.

    ENUM_COUNT,

    Default = Simulation
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphNodeCategory);

/// Where and how often a particle graph node executes.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphSchedule
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Once = 0U,    ///< Setup, reset, or one-shot GPU work.
    Spawn,        ///< Runs for emitted particles.
    PerParticle,  ///< Runs one thread per live particle.
    PerGroup,     ///< Runs one thread group per cell/tile/batch.
    Reduction,    ///< Parallel reduction or prefix operation.
    Sort,         ///< Sort-key generation or GPU sort stages.
    NeighborGrid, ///< Spatial hash, cell range, or neighbor-pair stage.
    Render,       ///< Draw preparation or billboard/mesh expansion.
    Readback,     ///< Optional asynchronous readback stage.

    ENUM_COUNT,

    Default = PerParticle
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphSchedule);

/// Node capabilities and compiler hints.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphNodeFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None                    = 0U,         ///< No special capabilities or requirements.
    HasSideEffects          = XII_BIT(0), ///< Node performs side effects such as writing to global buffers, emitting events, or interfacing with engine systems. Nodes without this flag can be safely reordered or culled if their outputs are unused.
    ReadsNeighborGrid       = XII_BIT(1), ///< Node reads from the neighbor grid, which may require special handling for data dependencies and scheduling. Nodes with this flag may need to be scheduled after the neighbor grid is built and before any nodes that write to it.
    WritesNeighborGrid      = XII_BIT(2), ///< Node writes to the neighbor grid, which may require special handling for data dependencies and scheduling. Nodes with this flag may need to be scheduled before any nodes that read from the neighbor grid.
    EmitsEvents             = XII_BIT(3), ///< Node emits events that can be consumed by other nodes or the game engine. This may require special handling for event queues and scheduling to ensure correct ordering and visibility of events.
    ConsumesEvents          = XII_BIT(4), ///< Node consumes events emitted by other nodes or the game engine. This may require special handling for event queues and scheduling to ensure correct ordering and visibility of events.
    SupportsAsyncCompute    = XII_BIT(5), ///< Node can be scheduled on an async compute queue, which may allow for better performance by overlapping with graphics work. Nodes without this flag may need to be scheduled on the main compute queue to ensure correct ordering with rendering.
    RequiresDeterminism     = XII_BIT(6), ///< Node requires deterministic execution order and results, which may restrict certain optimizations or scheduling strategies. Nodes with this flag may need to be scheduled in a fixed order and may not be eligible for certain parallelization techniques.
    SupportsMolecularDomain = XII_BIT(7), ///< Node can operate in a molecular dynamics domain, which may have special requirements for data layout, precision, or scheduling. Nodes without this flag may need to be scheduled in a separate phase or with different resource bindings when operating in a molecular domain.
    ToolOnly                = XII_BIT(8), ///< Node is only used for editor tooling and should be stripped from runtime builds. This may require special handling in the editor to allow usage of these nodes, while ensuring they are not included in runtime graphs.

    Default = SupportsAsyncCompute
  };

  struct Bits
  {
    StorageType HasSideEffects : 1;
    StorageType ReadsNeighborGrid : 1;
    StorageType WritesNeighborGrid : 1;
    StorageType EmitsEvents : 1;
    StorageType ConsumesEvents : 1;
    StorageType SupportsAsyncCompute : 1;
    StorageType RequiresDeterminism : 1;
    StorageType SupportsMolecularDomain : 1;
    StorageType ToolOnly : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiParticleGraphNodeFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphNodeFlags);

/// One input or output pin visible in the particle graph editor.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphPinDesc
{
  xiiHashedString                       m_sName;                 ///< Unique identifier for this pin within its node. For attribute pins, this is the name of the particle attribute. For parameter pins, this is the name of the parameter.
  xiiHashedString                       m_sAttributeName;        ///< For attribute pins, this is the name of the particle attribute this pin reads from or writes to. For parameter pins, this is typically empty and the parameter name is defined in xiiParticleGraphParameterDesc.
  xiiEnum<xiiParticleAttributeSemantic> m_Semantic;              ///< Built-in semantic for this pin, which can be used for special handling in the editor or runtime. Custom semantics should use 'Custom' and rely on m_sAttributeName for identification.
  xiiEnum<xiiParticleAttributeFormat>   m_Format;                ///< Data format of this pin, which determines how data is read from or written to particle attributes or parameters. This should be compatible with the expected format of the attribute or parameter this pin is connected to.
  xiiVariant                            m_DefaultValue;          ///< Default value for this pin, used when no connection is made. For input pins, this value will be used if the pin is not connected to an output. For output pins, this value may be ignored or used as an initial value depending on the node's implementation.
  bool                                  m_bRequired     = false; ///< Whether this pin must be connected for the node to function properly. This is a hint for the editor to indicate that the node may not execute or produce valid results if this pin is left unconnected.
  bool                                  m_bMultiConnect = false; ///< Whether this pin can be connected to multiple other pins. For input pins, this allows multiple outputs to feed into the same input, which may require special handling in the node's implementation (e.g., summing forces from multiple sources). For output pins, this allows the output to be used by multiple downstream nodes without needing to add explicit splitter nodes.
  bool                                  m_bHidden       = false; ///< Whether this pin should be hidden in the editor. Hidden pins are not visible or connectable in the editor, but can still be used for internal connections or by custom node implementations. This is useful for simplifying the editor UI by hiding pins that are not relevant for most use cases or that are only used for specific configurations.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphPinDesc);

/// Artist/scientist-facing parameter exposed by a particle graph node.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphParameterDesc
{
  xiiHashedString m_sName;               ///< Unique identifier for this parameter within its node. This is used for connecting to pins and for referencing the parameter in code. It should be unique among all parameters of the same node.
  xiiString       m_sDisplayName;        ///< User-friendly name for this parameter, shown in the editor UI. This can contain spaces and special characters, and is meant to be descriptive for artists and designers.
  xiiString       m_sCategory;           ///< Optional category for grouping parameters in the editor UI. Parameters with the same category may be shown together under a collapsible header. This is purely for organizational purposes in the editor.
  xiiString       m_sTooltip;            ///< Optional tooltip text for this parameter, shown in the editor UI when hovering over the parameter. This should provide a clear and concise description of what the parameter does and how it affects the node's behavior.
  xiiVariant      m_DefaultValue;        ///< Default value for this parameter, used when no value is set. This value will be used if the parameter is not connected to a pin or if the node's implementation chooses to use it as a fallback.
  xiiVariant      m_MinValue;            ///< Optional minimum value for this parameter, used for clamping in the editor UI. This is a hint for the editor to prevent artists from entering values that are outside of a reasonable range. The node's implementation may choose to enforce this clamp at runtime or simply use it as a guideline for the editor.
  xiiVariant      m_MaxValue;            ///< Optional maximum value for this parameter, used for clamping in the editor UI. This is a hint for the editor to prevent artists from entering values that are outside of a reasonable range. The node's implementation may choose to enforce this clamp at runtime or simply use it as a guideline for the editor.
  bool            m_bAnimatable = true;  ///< Whether this parameter can be animated over time in the editor. This is a hint for the editor to allow keyframing or curve editing for this parameter. The node's implementation may choose to support animation for this parameter or ignore this flag.
  bool            m_bAdvanced   = false; ///< Whether this parameter is considered advanced and should be hidden by default in the editor UI. This is a hint for the editor to allow artists to focus on the most commonly used parameters while still providing access to more advanced options when needed. The node's implementation may choose to support this flag by hiding advanced parameters in a collapsible section or simply use it as a guideline for the editor.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphParameterDesc);

/// A particle graph node. Nodes map to compute kernels, tool-only annotations, or custom engine callbacks.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphNodeDesc
{
  xiiUuid                                m_NodeId;       ///< Unique identifier for this node instance. This is used for connecting links and referencing the node in code. It should be generated when the node is created and remain stable for the lifetime of the node.
  xiiHashedString                        m_sType;        ///< Type identifier for this node, used to determine which compute kernel or engine callback to execute. This should be defined by the node's implementation and is used for dispatching the correct behavior at runtime.
  xiiString                              m_sDisplayName; ///< User-friendly name for this node, shown in the editor UI. This can contain spaces and special characters, and is meant to be descriptive for artists and designers.
  xiiString                              m_sKernelPath;  ///< Optional path to the compute shader kernel associated with this node. This is used for nodes that execute GPU code and should point to a valid shader file. Nodes without GPU execution may leave this empty or use it for other purposes (e.g., editor annotations).
  xiiString                              m_sEntryPoint;  ///< Optional entry point name within the compute shader specified by m_sKernelPath. This allows multiple nodes to share the same shader file while executing different kernels. Nodes without GPU execution may leave this empty or use it for other purposes (e.g., editor annotations).
  xiiEnum<xiiParticleGraphNodeCategory>  m_Category;     ///< High-level category for this node, used for editor grouping and compile scheduling. This is a hint for the editor to organize nodes into categories, and for the compiler to determine a default execution order. Custom scheduling may still be determined by user-defined rules or editor settings.
  xiiEnum<xiiParticleGraphSchedule>      m_Schedule;     ///< Where and how often this node executes, used for scheduling and determining execution frequency. This is a hint for the compiler to determine when this node should be executed in the simulation loop. Custom scheduling may still be determined by user-defined rules or editor settings.
  xiiBitflags<xiiParticleGraphNodeFlags> m_Flags;
  xiiVec2                                m_vEditorPosition   = xiiVec2::MakeZero(); ///< Position of this node in the editor graph view, used for layout and visualization. This is purely for editor purposes and has no effect on runtime behavior.
  xiiColor                               m_DebugColor        = xiiColor::White;     ///< Optional debug color for this node, used for visualization in the editor or runtime debugging. This is purely a hint for visualization purposes and has no effect on the node's behavior.
  xiiUInt32                              m_uiThreadGroupSize = 64U;                 ///< Optional thread group size for GPU execution, used for dispatching compute shaders. This is only relevant for nodes that execute GPU code and should be set to a reasonable value based on the expected workload of the node. Nodes without GPU execution may ignore this value.
  xiiUInt32                              m_uiEstimatedCost   = 1U;                  ///< Optional estimated cost for this node, used for scheduling heuristics. This is a hint for the compiler to determine how to schedule this node relative to others, with higher cost nodes potentially being scheduled less frequently or on async compute queues. The actual scheduling may still be determined by user-defined rules or editor settings.

  xiiDynamicArray<xiiParticleGraphPinDesc>       m_Inputs;          ///< Descriptions of the input pins for this node, which determine what data this node consumes. Each pin should have a unique name within this node and should specify the expected data format and semantics for connections.
  xiiDynamicArray<xiiParticleGraphPinDesc>       m_Outputs;         ///< Descriptions of the output pins for this node, which determine what data this node produces. Each pin should have a unique name within this node and should specify the expected data format and semantics for connections.
  xiiDynamicArray<xiiParticleGraphParameterDesc> m_Parameters;      ///< Descriptions of the parameters exposed by this node, which can be edited by artists and designers. Each parameter should have a unique name within this node and should specify the expected data format and semantics for connections.
  xiiDynamicArray<xiiHashedString>               m_ReadAttributes;  ///< List of particle attributes read by this node, used for determining data dependencies and scheduling. Each attribute name should correspond to a valid particle attribute that this node reads from. This is a hint for the compiler to determine when this node needs to be executed based on which attributes it reads and which nodes write to those attributes.
  xiiDynamicArray<xiiHashedString>               m_WriteAttributes; ///< List of particle attributes written by this node, used for determining data dependencies and scheduling. Each attribute name should correspond to a valid particle attribute that this node writes to. This is a hint for the compiler to determine when this node needs to be executed based on which attributes it writes and which nodes read from those attributes.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphNodeDesc);

/// Directed connection between two particle graph pins.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphLinkDesc
{
  xiiUuid         m_SourceNode;      ///< Unique identifier of the source node for this link. This should correspond to the m_NodeId of an existing node in the graph.
  xiiHashedString m_sSourcePin;      ///< Name of the source pin on the source node for this link. This should correspond to the m_sName of an output pin in the source node's m_Outputs array.
  xiiUuid         m_TargetNode;      ///< Unique identifier of the target node for this link. This should correspond to the m_NodeId of an existing node in the graph.
  xiiHashedString m_sTargetPin;      ///< Name of the target pin on the target node for this link. This should correspond to the m_sName of an input pin in the target node's m_Inputs array.
  bool            m_bEnabled = true; ///< Whether this link is enabled and should be considered for execution. This is a hint for the editor and runtime to allow temporarily disabling connections without deleting them, which can be useful for testing and debugging different graph configurations. The node's implementation may choose to ignore disabled links or use them as a hint to skip certain computations.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphLinkDesc);

/// Editor annotation/grouping data for DCC-style graph tooling.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphGroupDesc
{
  xiiUuid                  m_GroupId;                             ///< Unique identifier for this group instance. This is used for referencing the group in code and should be generated when the group is created and remain stable for the lifetime of the group.
  xiiString                m_sTitle;                              ///< User-friendly title for this group, shown in the editor UI. This can contain spaces and special characters, and is meant to be descriptive for artists and designers.
  xiiColor                 m_Color     = xiiColor::White;         ///< Optional color for this group, used for visualization in the editor. This is purely a hint for visualization purposes and has no effect on the group's behavior.
  xiiVec2                  m_vPosition = xiiVec2::MakeZero();     ///< Position of this group in the editor graph view, used for layout and visualization. This is purely for editor purposes and has no effect on runtime behavior.
  xiiVec2                  m_vSize     = xiiVec2(320.0f, 180.0f); ///< Size of this group in the editor graph view, used for layout and visualization. This is purely for editor purposes and has no effect on runtime behavior.
  xiiDynamicArray<xiiUuid> m_Nodes;                               ///< List of node IDs that belong to this group, used for organizational purposes in the editor. Each node ID should correspond to the m_NodeId of an existing node in the graph. This is purely for editor purposes and has no effect on runtime behavior, but can be used by the editor to visually group nodes together and allow for easier manipulation of related nodes.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphGroupDesc);

/// Serializable particle graph descriptor. It is intentionally data-only so tools can round-trip it losslessly.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphResourceDescriptor
{
  /// Adds a new node to the graph with the specified description and returns its unique identifier. The node description should have a unique m_NodeId, but if it is not set, a new one will be generated. The caller is responsible for ensuring that the node description is valid and that the m_NodeId is unique within this graph.
  xiiUuid AddNode(const xiiParticleGraphNodeDesc& node);

  /// Removes the node with the specified unique identifier from the graph, along with any links connected to it. Returns true if the node was found and removed, or false if no node with the specified ID exists in the graph. The caller is responsible for ensuring that any references to this node (e.g., in links or groups) are also updated or removed as necessary.
  bool RemoveNode(const xiiUuid& nodeId);

  /// Adds a new directed link between the specified source and target pins of the source and target nodes. The link description should reference valid node IDs and pin names that exist in this graph. The caller is responsible for ensuring that the link description is valid and that the source and target nodes and pins exist in this graph.
  void AddLink(const xiiParticleGraphLinkDesc& link);

  /// Removes the directed link between the specified source and target pins of the source and target nodes. Returns true if the link was found and removed, or false if no such link exists in the graph. The caller is responsible for ensuring that any references to this link (e.g., in node implementations) are also updated or removed as necessary.
  void Clear();

  /// Finds a node in the graph by its unique identifier. Returns a pointer to the node description if found, or nullptr if no node with the specified ID exists in the graph. The caller should not modify the returned node description directly, as it is owned by the graph. Instead, use the non-const version of this function to modify nodes.
  [[nodiscard]] const xiiParticleGraphNodeDesc* FindNode(const xiiUuid& nodeId) const;

  /// Finds a node in the graph by its unique identifier. Returns a pointer to the node description if found, or nullptr if no node with the specified ID exists in the graph. The caller can modify the returned node description directly, as it is owned by the graph.
  [[nodiscard]] xiiParticleGraphNodeDesc* FindNode(const xiiUuid& nodeId);

  /// Validates the integrity of the graph, checking for issues such as missing nodes, invalid links, duplicate IDs, and other potential problems. If the graph is valid, returns success. If the graph is invalid, returns failure and optionally fills out_pError with a description of the first encountered issue. The caller can use this function to ensure that the graph is well-formed before attempting to compile or execute it.
  [[nodiscard]] xiiResult Validate(xiiStringBuilder* out_pError = nullptr) const;

  /// Computes a hash value for the compute pipeline that would be generated from this graph. This can be used for caching and quick comparisons of graph configurations. The hash should take into account the structure of the graph, the types and connections of nodes, and any relevant parameters that would affect the generated compute shader. The exact hashing algorithm is up to the implementation, but it should produce the same hash for graphs that are functionally equivalent in terms of their compute behavior.
  [[nodiscard]] xiiUInt64 ComputePipelineHash() const;

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);

public:
  xiiString m_sGraphName;     ///< Name of the particle graph, used for identification and debugging purposes. This is purely a hint for visualization and has no effect on the graph's behavior.
  xiiString m_sDescription;   ///< Optional description of the particle graph, used for providing additional context and information in the editor. This is purely a hint for visualization and has no effect on the graph's behavior.
  xiiString m_sAuthoringTool; ///< Optional name of the tool or DCC application that authored this graph, used for tracking and debugging purposes. This is purely a hint for visualization and has no effect on the graph's behavior.

  xiiDynamicArray<xiiParticleGraphNodeDesc>      m_Nodes;             ///< List of nodes in this particle graph. Each node should have a unique m_NodeId, and links will reference nodes by their IDs. The order of nodes in this array does not determine execution order, which is determined by the links and scheduling hints.
  xiiDynamicArray<xiiParticleGraphLinkDesc>      m_Links;             ///< List of directed links between node pins in this particle graph. Each link should reference valid source and target nodes and pins. The order of links in this array does not determine execution order, which is determined by the scheduling hints on the nodes and the data dependencies implied by the links.
  xiiDynamicArray<xiiParticleGraphGroupDesc>     m_Groups;            ///< List of editor groups for organizing nodes in the graph. This is purely for editor purposes and has no effect on runtime behavior, but can be used by the editor to visually group nodes together and allow for easier manipulation of related nodes.
  xiiDynamicArray<xiiParticleGraphParameterDesc> m_ExposedParameters; ///< List of parameters exposed by the graph as a whole, which can be edited by artists and designers. Each parameter should have a unique name within this graph and should specify the expected data format and semantics for connections. These parameters can be used to control global aspects of the graph's behavior or to provide inputs that are not tied to a specific node.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphResourceDescriptor);

/// Resource wrapper around a DCC-authored GPU particle graph.
class XII_GRAPHICSCORE_DLL xiiParticleGraphResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleGraphResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiParticleGraphResource);

  XII_RESOURCE_DECLARE_CREATEABLE(xiiParticleGraphResource, xiiParticleGraphResourceDescriptor);

public:
  xiiParticleGraphResource();
  ~xiiParticleGraphResource();

  /// Returns a const reference to the descriptor that defines the contents of this particle graph resource.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiParticleGraphResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  /// Returns a hash value for the compute pipeline generated from this graph, which can be used for caching and quick comparisons of graph configurations.
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64 GetPipelineHash() const { return m_uiPipelineHash; }

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiParticleGraphResourceDescriptor m_Descriptor;            ///< The descriptor that defines the contents of this particle graph resource.
  xiiUInt64                          m_uiPipelineHash = 0ULL; ///< Cached hash value for the compute pipeline generated from this graph.
};
