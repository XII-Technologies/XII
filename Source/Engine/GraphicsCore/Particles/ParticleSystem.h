/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Types/Delegate.h>
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Particles/ParticleGraph.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Resources/Buffer.h>

class xiiParticleSystemRuntime;
struct xiiMsgExtractRenderData;

using xiiParticleSystemComponentManager = xiiComponentManager<class xiiParticleSystemComponent, xiiBlockStorageType::FreeList>;

/// Constants shared by the CPU runtime and shader-side particle layouts.
struct XII_GRAPHICSCORE_DLL xiiParticleSystemConstants
{
  static constexpr xiiUInt32 s_uiDefaultMaxParticles    = 1024U * 1024U;
  static constexpr xiiUInt32 s_uiDefaultMaxEmitters     = 1024U;
  static constexpr xiiUInt32 s_uiDefaultMaxEvents       = 1024U * 1024U;
  static constexpr xiiUInt32 s_uiDefaultThreadGroupSize = 64U;
  static constexpr xiiUInt32 s_uiMaxSupportedParticles  = 16U * 1024U * 1024U;
};

/// Simulation space for particle data.
struct XII_GRAPHICSCORE_DLL xiiParticleSimulationSpace
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    World = 0U, ///< Particle positions and velocities are in world space, and the system's local transform is ignored for simulation. This is the most common choice for general-purpose particle systems.
    Local,      ///< Particle positions and velocities are in the local space of the system's transform, which is used as the simulation origin. This allows for easy movement and rotation of the entire system by modifying its transform, but may require special handling for emitter shapes and rendering to ensure correct orientation and scaling.
    View,       ///< Particle positions and velocities are in view space, which is defined by the camera's position and orientation. This can be useful for certain screen-space effects or when you want particles to always face the camera, but may require special handling for emitter shapes and rendering to ensure correct orientation and scaling.

    ENUM_COUNT,

    Default = World
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleSimulationSpace);

/// Feature flags controlling runtime allocation and pass scheduling.
struct XII_GRAPHICSCORE_DLL xiiParticleSystemFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None                = 0U,          ///< No special features enabled, which may result in a very basic and limited particle system with minimal GPU usage. This can be useful for simple effects or when you want to minimize GPU overhead, but may not support many common particle behaviors or rendering techniques.
    GPUDriven           = XII_BIT(0),  ///< Particle simulation and update logic runs entirely on the GPU, with minimal CPU involvement. This can allow for very large particle counts and complex behaviors, but may require more careful design of the particle graph and may not support certain CPU-side features such as complex event handling or integration with game logic.
    AsyncCompute        = XII_BIT(1),  ///< Simulation and update passes can be scheduled on an async compute queue, allowing for better performance by overlapping with graphics work. This may require careful handling of resource synchronization and may not be compatible with all GPU features or platforms.
    IndirectDraw        = XII_BIT(2),  ///< Rendering uses indirect draw calls, which can reduce CPU overhead and allow for more dynamic rendering techniques, but may require more complex GPU-side logic for generating draw arguments and may not be supported on all platforms.
    GPUCulling          = XII_BIT(3),  ///< The system performs GPU-based culling of particles, which can improve performance by reducing the number of particles that need to be processed and rendered, but may require additional GPU resources for culling and may not be suitable for all types of particle systems (e.g., those with complex interactions or non-spherical bounds).
    SortByDepth         = XII_BIT(4),  ///< Particles are sorted by depth on the GPU, which can improve rendering quality for transparent particles but may require additional GPU resources for sorting and may not be necessary for all types of particle systems (e.g., opaque particles or those that use order-independent transparency techniques).
    StableParticleIds   = XII_BIT(5),  ///< Particle IDs remain stable across frames, which can allow for more consistent behavior and easier debugging, but may require additional GPU resources for maintaining and sorting particles by ID and may not be necessary for all types of particle systems (e.g., those that don't rely on particle identity or have simple behaviors).
    EnableEvents        = XII_BIT(6),  ///< The system supports emitting and consuming events, which can allow for more complex interactions between particles and with the game engine, but may require additional GPU resources for event queues and may not be necessary for all types of particle systems (e.g., those that don't rely on events or have simple behaviors).
    EnableReadback      = XII_BIT(7),  ///< The system supports asynchronous readback of particle data to the CPU, which can allow for CPU-side processing or integration with game logic, but may require additional GPU resources for staging buffers and may not be necessary for all types of particle systems (e.g., those that are fully GPU-driven and don't require CPU interaction).
    Deterministic       = XII_BIT(8),  ///< The system is designed to produce deterministic results across runs, which can be important for certain types of simulations or for debugging, but may require additional constraints on the particle graph and may limit certain optimizations or features that could introduce non-determinism.
    MolecularDynamics   = XII_BIT(9),  ///< The system is designed to support molecular dynamics simulations, which may have special requirements for data layout, precision, and scheduling. This may require additional GPU resources for neighbor searching and may not be necessary for all types of particle systems (e.g., those that don't rely on particle interactions or have simple behaviors).
    NeighborSearch      = XII_BIT(10), ///< The system supports GPU-based neighbor searching, which can allow for more complex particle interactions such as collisions or fluid dynamics, but may require additional GPU resources for spatial data structures and may not be necessary for all types of particle systems (e.g., those that don't rely on particle interactions or have simple behaviors).
    DoubleBufferedState = XII_BIT(11), ///< The system uses double buffering for particle state, which can help to avoid read-write hazards and allow for more flexible scheduling of simulation and rendering passes, but may require additional GPU resources for maintaining multiple state buffers and may not be necessary for all types of particle systems (e.g., those with simple update logic or that can tolerate some level of read-write contention).

    Default = GPUDriven | AsyncCompute | IndirectDraw | GPUCulling | StableParticleIds | DoubleBufferedState
  };

  struct Bits
  {
    StorageType GPUDriven : 1;
    StorageType AsyncCompute : 1;
    StorageType IndirectDraw : 1;
    StorageType GPUCulling : 1;
    StorageType SortByDepth : 1;
    StorageType StableParticleIds : 1;
    StorageType EnableEvents : 1;
    StorageType EnableReadback : 1;
    StorageType Deterministic : 1;
    StorageType MolecularDynamics : 1;
    StorageType NeighborSearch : 1;
    StorageType DoubleBufferedState : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiParticleSystemFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleSystemFlags);

/// GPU particle state. Keep in sync with Data/Base/Shaders/Pipeline/GPUParticleSimulate.xiiShader.
struct alignas(16) XII_GRAPHICSCORE_DLL xiiParticleGPUState
{
  XII_DECLARE_POD_TYPE();

  xiiVec4   m_vPositionAge;            ///< xyz = position, w = age.
  xiiVec4   m_vPreviousPositionRadius; ///< xyz = previous position, w = radius.
  xiiVec4   m_vVelocityLifetime;       ///< xyz = velocity, w = lifetime.
  xiiVec4   m_vColor;                  ///< rgba.
  xiiVec4   m_vForceInverseMass;       ///< xyz = accumulated force, w = inverse mass.
  xiiVec4   m_vCustom0;                ///< graph-owned custom payload.
  xiiVec4   m_vCustom1;                ///< graph-owned custom payload.
  xiiUInt32 m_uiId      = 0U;          ///< Unique particle ID, used for stable sorting and graph operations that require particle identity. The exact semantics of this ID (e.g., whether it's recycled after a particle dies) are defined by the particle graph and may depend on the enabled features (e.g., StableParticleIds).
  xiiUInt32 m_uiFlags   = 0U;          ///< Graph-defined flags for various purposes (e.g., marking particles for special processing or rendering). The exact semantics of these flags are defined by the particle graph and may depend on the enabled features.
  xiiUInt32 m_uiEmitter = 0U;          ///< Index of the emitter that spawned this particle, which can be used for emitter-specific behavior or rendering. The exact semantics of this index are defined by the particle graph and may depend on the enabled features (e.g., whether emitter information is tracked on particles).
  xiiUInt32 m_uiCellId  = 0U;          ///< Cell ID for neighbor search, if enabled. The exact semantics of this ID (e.g., how it's calculated and updated) are defined by the particle graph and may depend on the enabled features (e.g., NeighborSearch).
};

static_assert((sizeof(xiiParticleGPUState) % 16U) == 0U);

/// GPU counters consumed by emit, compact, simulate, render and readback passes.
struct XII_GRAPHICSCORE_DLL xiiParticleGPUCounters
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiAliveCount        = 0U; ///< Number of currently alive particles. This is updated by the GPU after emit, simulate, and compact passes, and is used for determining how many particles to process in subsequent passes (e.g., rendering or readback). The exact semantics of this count (e.g., whether it includes newly emitted particles before simulation) are defined by the particle graph and may depend on the enabled features.
  xiiUInt32 m_uiDeadCount         = 0U; ///< Number of currently dead particles. This is updated by the GPU after emit, simulate, and compact passes, and is used for determining how many particles can be emitted in subsequent emit passes (e.g., if there's a maximum capacity). The exact semantics of this count (e.g., whether it includes particles that just died in the current frame) are defined by the particle graph and may depend on the enabled features.
  xiiUInt32 m_uiSpawnRequestCount = 0U; ///< Number of spawn requests made by emitters in the current frame. This is updated by the GPU during emit passes, and can be used for debugging or for implementing features such as emitter throttling based on demand. The exact semantics of this count (e.g., whether it includes requests that were denied due to capacity limits) are defined by the particle graph and may depend on the enabled features.
  xiiUInt32 m_uiEventCount        = 0U; ///< Number of events emitted in the current frame. This is updated by the GPU during simulation passes when events are emitted, and can be used for debugging or for implementing features such as event throttling based on demand. The exact semantics of this count (e.g., whether it includes events that were emitted but not yet processed) are defined by the particle graph and may depend on the enabled features (e.g., EnableEvents).
};

/// Runtime descriptor for large GPU particle simulations.
struct XII_GRAPHICSCORE_DLL xiiParticleSystemDescriptor
{
  /// Returns the center of the local bounding volume.
  XII_ALWAYS_INLINE xiiVec3 GetLocalBoundsCenter() const { return m_LocalBounds.m_vCenter; }

  /// Sets the center of the local bounding volume. This should be set to the expected center of the particle distribution in the system's local space (e.g., the emitter position) for best culling performance, but may be overridden by the particle graph if it has specific requirements for the bounding volume.
  XII_ALWAYS_INLINE void SetLocalBoundsCenter(xiiVec3 vCenter) { m_LocalBounds.m_vCenter = vCenter; }

  /// Returns the half extents of the local bounding box. This should be set to a tight fit around the expected particle distribution in the system's local space for best culling performance, but may be overridden by the particle graph if it has specific requirements for the bounding volume (e.g., if GPUCulling is enabled, the graph may automatically update the bounds based on particle behavior).
  XII_ALWAYS_INLINE xiiVec3 GetLocalBoundsHalfExtents() const { return m_LocalBounds.m_vBoxHalfExtents; }

  /// Sets the half extents of the local bounding box. This should be set to a tight fit around the expected particle distribution in the system's local space for best culling performance, but may be overridden by the particle graph if it has specific requirements for the bounding volume (e.g., if GPUCulling is enabled, the graph may automatically update the bounds based on particle behavior). The sphere radius is automatically updated to encompass the box, so it is not necessary to set it separately unless you want a different value for specific use cases (e.g., using a larger sphere radius for effects with long trails or fast-moving particles).
  XII_ALWAYS_INLINE void SetLocalBoundsHalfExtents(xiiVec3 vHalfExtents)
  {
    m_LocalBounds.m_vBoxHalfExtents = vHalfExtents.CompMax(xiiVec3::MakeZero());
    m_LocalBounds.m_fSphereRadius   = m_LocalBounds.m_vBoxHalfExtents.GetLength();
  }

  /// Returns the radius of the local bounding sphere. This should be set to a value that encompasses the expected particle distribution in the system's local space for best culling performance, but may be overridden by the particle graph if it has specific requirements for the bounding volume (e.g., if GPUCulling is enabled, the graph may automatically update the bounds based on particle behavior).
  XII_ALWAYS_INLINE float GetLocalBoundsRadius() const { return m_LocalBounds.m_fSphereRadius; }

  /// Sets the radius of the local bounding sphere. This should be set to a value that encompasses the expected particle distribution in the system's local space for best culling performance, but may be overridden by the particle graph if it has specific requirements for the bounding volume (e.g., if GPUCulling is enabled, the graph may automatically update the bounds based on particle behavior). The box half extents are automatically updated to encompass the sphere, so it is not necessary to set them separately unless you want different values for specific use cases (e.g., using a larger box for effects with long trails or fast-moving particles).
  XII_ALWAYS_INLINE void SetLocalBoundsRadius(float fRadius) { m_LocalBounds.m_fSphereRadius = xiiMath::Max(0.0f, fRadius); }

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);

public:
  xiiParticleGraphResourceHandle      m_hGraph;                                                                   ///< Handle to the particle graph resource that defines the behavior of this particle system. The graph's descriptor contains all the necessary information about the emitters, simulation logic, and rendering techniques used by the system.
  xiiEnum<xiiParticleSimulationSpace> m_SimulationSpace;                                                          ///< The simulation space in which particle data is represented and updated. This affects how particle positions and velocities are interpreted and how the system's transform is used for simulation.
  xiiBitflags<xiiParticleSystemFlags> m_Flags;                                                                    ///< Feature flags that control various aspects of the particle system's behavior, including runtime allocation, pass scheduling, and supported features. These flags can enable or disable specific capabilities of the particle system, allowing for flexible configuration based on the needs of the effect being created.
  xiiBoundingBoxSphere                m_LocalBounds        = xiiBoundingBoxSphere::MakeZero();                    ///< The local bounding volume that encompasses all particles in their default state (e.g., at spawn with no velocity). This is used for culling and other optimizations, and should be set to a tight fit around the expected particle distribution for best performance. The exact semantics of this bounding volume (e.g., whether it's automatically updated based on particle behavior) are defined by the particle graph and may depend on the enabled features (e.g., GPUCulling).
  xiiUInt32                           m_uiMaxParticles     = xiiParticleSystemConstants::s_uiDefaultMaxParticles; ///< The maximum number of particles that can be alive in the system at any given time. This is used for allocating GPU buffers and for determining how many particles to process in simulation and rendering passes. The exact semantics of this limit (e.g., whether it's a hard cap or can be exceeded with performance consequences) are defined by the particle graph and may depend on the enabled features (e.g., GPUDriven).
  xiiUInt32                           m_uiMaxEmitters      = xiiParticleSystemConstants::s_uiDefaultMaxEmitters;  ///< The maximum number of emitters that can be active in the system at any given time. This is used for allocating GPU buffers and for determining how many emitters to process in simulation passes. The exact semantics of this limit (e.g., whether it's a hard cap or can be exceeded with performance consequences) are defined by the particle graph and may depend on the enabled features (e.g., GPUDriven).
  xiiUInt32                           m_uiMaxEvents        = xiiParticleSystemConstants::s_uiDefaultMaxEvents;    ///< The maximum number of events that can be emitted in the system at any given time. This is used for allocating GPU buffers and for determining how many events to process in simulation passes. The exact semantics of this limit (e.g., whether it's a hard cap or can be exceeded with performance consequences) are defined by the particle graph and may depend on the enabled features (e.g., EnableEvents).
  xiiUInt32                           m_uiMaxNeighborPairs = 0U;                                                  ///< The maximum number of neighbor pairs that can be generated in the system at any given time. This is used for allocating GPU buffers and for determining how many pairs to process in simulation passes when neighbor searching is enabled. The exact semantics of this limit (e.g., whether it's a hard cap or can be exceeded with performance consequences) are defined by the particle graph and may depend on the enabled features (e.g., NeighborSearch).
  xiiUInt32                           m_uiRandomSeed       = 0U;                                                  ///< The random seed used for any stochastic behavior in the particle graph. This can be set to a specific value for deterministic behavior or left at the default for more varied results. The exact semantics of this seed (e.g., how it's used in the graph and whether it affects all random operations) are defined by the particle graph and may depend on the enabled features (e.g., Deterministic).
  xiiUInt8                            m_uiMaxSubSteps      = 1U;                                                  ///< The maximum number of sub-steps to perform in a single frame when the fixed time step is exceeded. This can help to maintain stable simulations at low frame rates, but may increase GPU workload and latency. The exact semantics of this limit (e.g., whether it's a hard cap or can be exceeded with performance consequences) are defined by the particle graph and may depend on the enabled features (e.g., Deterministic).
  float                               m_fFixedTimeStep     = 1.0f / 60.0f;                                        ///< The fixed time step to use for the simulation when sub-stepping is necessary. This can help to maintain stable simulations at low frame rates, but may increase GPU workload and latency. The exact semantics of this time step (e.g., how it's applied in the graph and whether it affects all simulation logic) are defined by the particle graph and may depend on the enabled features (e.g., Deterministic).
  float                               m_fNeighborCellSize  = 1.0f;                                                ///< The cell size to use for spatial partitioning when GPU-based neighbor searching is enabled. This affects the performance and accuracy of neighbor searches, and should be set based on the expected particle distribution and interaction radius. The exact semantics of this cell size (e.g., how it's used in the graph and whether it can be changed at runtime) are defined by the particle graph and may depend on the enabled features (e.g., NeighborSearch).
  bool                                m_bAlwaysVisible     = false;                                               ///< Whether the system should be considered always visible for rendering and culling purposes, which can be useful for certain types of effects (e.g., screen-space effects or those that are meant to be visible regardless of distance), but may reduce performance by preventing culling optimizations. The exact semantics of this flag (e.g., whether it overrides all culling or can be combined with other visibility settings) are defined by the particle graph and may depend on the enabled features (e.g., GPUCulling).
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleSystemDescriptor);

/// Renderer-facing packet for GPU particle systems.
class XII_GRAPHICSCORE_DLL xiiParticleRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleRenderData, xiiRenderData);

public:
  xiiParticleSystemDescriptor m_Descriptor;      ///< The descriptor for the particle system, which contains all the necessary information about the particle graph, simulation space, features, and other parameters needed for rendering. This is filled by the xiiParticleSystemComponent when submitting render data, and is used by the renderer to set up the appropriate resources and shader parameters for rendering the particles.
  xiiUInt32                   m_uiUniqueID = 0U; ///< A unique identifier for this render data instance, which can be used for various purposes such as sorting, batching, or debugging. The exact semantics of this ID (e.g., how it's generated and whether it needs to be unique across frames) are defined by the renderer and may depend on the enabled features of the particle system.

  xiiSharedPtr<xiiGALBuffer> m_pParticleStateBuffer; ///< A buffer containing the current state of all particles in the system, which is updated by the GPU during simulation passes and read by the renderer for rendering. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering.
  xiiSharedPtr<xiiGALBuffer> m_pAliveIndexBuffer;    ///< A buffer containing the indices of currently alive particles, which is updated by the GPU during simulation passes and read by the renderer for rendering. This allows for efficient rendering of only the active particles without needing to process the entire state buffer.
  xiiSharedPtr<xiiGALBuffer> m_pCountersBuffer;      ///< A buffer containing various counters related to the particle system (e.g., alive count, dead count, spawn request count, event count), which is updated by the GPU during simulation passes and can be read by the renderer for rendering or debugging purposes.
  xiiSharedPtr<xiiGALBuffer> m_pDrawIndirectBuffer;  ///< A buffer containing the arguments for indirect draw calls, which is updated by the GPU during simulation passes (e.g., based on the alive count) and read by the renderer for rendering. This allows for efficient rendering of a variable number of particles without needing to read back counts to the CPU.

  xiiParticleSystemRuntime* m_pRuntime = nullptr; ///< Pointer to the particle system runtime that owns the resources and manages the simulation for this render data. This can be used by the renderer to access additional information or functionality related to the particle system, such as updating simulation parameters or triggering events.
};

/// Render graph pass data used by xiiParticleSystemRuntime::AddSimulationPasses().
struct XII_GRAPHICSCORE_DLL xiiParticleSimulationPassData
{
  xiiRenderGraphBufferHandle m_hParticleStateRead;      ///< Buffer handle for reading the current particle state. This is bound as a read-only resource in simulation passes and contains the state of all particles at the start of the pass. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation.
  xiiRenderGraphBufferHandle m_hParticleStateWrite;     ///< Buffer handle for writing the next particle state. This is bound as a write-only resource in simulation passes and contains the updated state of all particles at the end of the pass. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation.
  xiiRenderGraphBufferHandle m_hAliveIndexBuffer;       ///< Buffer handle for the alive index buffer, which contains the indices of currently alive particles. This is bound as a read-write resource in simulation passes and is updated by the GPU based on particle births and deaths during the pass.
  xiiRenderGraphBufferHandle m_hDeadIndexBuffer;        ///< Buffer handle for the dead index buffer, which contains the indices of currently dead particles. This is bound as a read-write resource in simulation passes and is updated by the GPU based on particle births and deaths during the pass.
  xiiRenderGraphBufferHandle m_hCountersBuffer;         ///< Buffer handle for the counters buffer, which contains various counters related to the particle system (e.g., alive count, dead count, spawn request count, event count). This is bound as a read-write resource in simulation passes and is updated by the GPU based on particle behavior during the pass.
  xiiRenderGraphBufferHandle m_hEventBuffer;            ///< Buffer handle for the event buffer, which contains events emitted by particles during simulation passes. This is bound as a read-write resource in simulation passes when event support is enabled, and is updated by the GPU based on particle behavior during the pass.
  xiiRenderGraphBufferHandle m_hSortKeyBuffer;          ///< Buffer handle for the sort key buffer, which contains keys used for sorting particles (e.g., by depth or by ID). This is bound as a read-write resource in simulation passes when sorting is enabled, and is updated by the GPU based on particle behavior during the pass.
  xiiRenderGraphBufferHandle m_hNeighborPairBuffer;     ///< Buffer handle for the neighbor pair buffer, which contains pairs of neighboring particles when GPU-based neighbor searching is enabled. This is bound as a read-write resource in simulation passes when neighbor searching is enabled, and is updated by the GPU based on particle positions during the pass.
  xiiRenderGraphBufferHandle m_hDrawIndirectBuffer;     ///< Buffer handle for the draw indirect buffer, which contains the arguments for indirect draw calls. This is bound as a read-write resource in simulation passes when indirect drawing is enabled, and is updated by the GPU based on the alive count and other factors during the pass.
  xiiRenderGraphBufferHandle m_hDispatchIndirectBuffer; ///< Buffer handle for the dispatch indirect buffer, which contains the arguments for indirect dispatch calls (e.g., for culling or sorting). This is bound as a read-write resource in simulation passes when async compute or GPU culling is enabled, and is updated by the GPU based on the alive count and other factors during the pass.

  xiiParticleGraphResourceHandle m_hGraph;                  ///< Handle to the particle graph resource that defines the behavior of the simulation pass. This is used by the node executor to determine which nodes to execute and how to set up shader parameters for the pass.
  xiiUInt32                      m_uiParticleCapacity = 0U; ///< The maximum number of particles that can be processed in this simulation pass, which is used for determining thread group counts and for validating buffer sizes. This should be set based on the descriptor of the particle system and the enabled features, and should be consistent with the actual capacity of the buffers bound to the pass.
  xiiUInt32                      m_uiDispatchGroups   = 0U; ///< The number of thread groups to dispatch for this simulation pass, which is calculated based on the particle capacity and the thread group size defined in the particle graph. This should be set to ensure that all particles are processed in the pass, and may be adjusted based on performance considerations or specific requirements of the simulation logic.
};

using xiiParticleGraphNodeExecutor = xiiDelegate<void(const xiiParticleGraphNodeDesc&, const xiiParticleSimulationPassData&, xiiRenderGraphPassContext&)>;

/// Persistent GPU runtime for one particle system. Owns large buffers and registers graph passes.
class XII_GRAPHICSCORE_DLL xiiParticleSystemRuntime
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiParticleSystemRuntime);

public:
  /// Returns the current capacity of the particle system, which is determined by the size of the allocated GPU buffers and the limits defined in the descriptor. This can be used for debugging or for making decisions about how to configure emitters or other aspects of the particle graph based on the available capacity.
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetParticleCapacity() const { return m_uiParticleCapacity; }

  /// Returns the current capacity for events in the particle system, which is determined by the size of the allocated GPU buffers and the limits defined in the descriptor. This can be used for debugging or for making decisions about how to configure event emitters or other aspects of the particle graph based on the available capacity for events.
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetEventCapacity() const { return m_uiEventCapacity; }

  /// Returns the current capacity for neighbor pairs in the particle system, which is determined by the size of the allocated GPU buffers and the limits defined in the descriptor. This can be used for debugging or for making decisions about how to configure particle interactions or other aspects of the particle graph based on the available capacity for neighbor pairs.
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetNeighborPairCapacity() const { return m_uiNeighborPairCapacity; }

  /// Returns the current particle state buffer for reading, which contains the state of all particles at the start of the current frame. This buffer is updated by the GPU during simulation passes and should be used for rendering or readback operations that need to access the current state of particles. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetCurrentParticleStateBuffer() const;

  /// Returns the next particle state buffer for writing, which is used for updating the state of all particles during simulation passes. This buffer is updated by the GPU during simulation passes and should be used for rendering or readback operations that need to access the updated state of particles after simulation. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetNextParticleStateBuffer() const;

  /// Returns the alive index buffer, which contains the indices of currently alive particles. This buffer is updated by the GPU during simulation passes and should be used for rendering or readback operations that need to access the list of active particles. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetAliveIndexBuffer() const { return m_pAliveIndexBuffer; }

  /// Returns the dead index buffer, which contains the indices of currently dead particles. This buffer is updated by the GPU during simulation passes and can be used for debugging or for implementing features such as emitter throttling based on available capacity. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetDeadIndexBuffer() const { return m_pDeadIndexBuffer; }

  /// Returns the counters buffer, which contains various counters related to the particle system (e.g., alive count, dead count, spawn request count, event count). This buffer is updated by the GPU during simulation passes and can be read by the renderer for rendering or debugging purposes. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetCountersBuffer() const { return m_pCountersBuffer; }

  /// Returns the event buffer, which contains events emitted by particles during simulation passes. This buffer is updated by the GPU during simulation passes when event support is enabled, and can be read by the renderer or by game logic for processing events. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and event processing.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetEventBuffer() const { return m_pEventBuffer; }

  /// Returns the sort key buffer, which contains keys used for sorting particles (e.g., by depth or by ID). This buffer is updated by the GPU during simulation passes when sorting is enabled, and can be read by the renderer for rendering or debugging purposes. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and rendering.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetSortKeyBuffer() const { return m_pSortKeyBuffer; }

  /// Returns the grid cell buffer, which contains the spatial partitioning of particles into grid cells when GPU-based neighbor searching is enabled. This buffer is updated by the GPU during simulation passes when neighbor searching is enabled, and can be read by the renderer or by game logic for processing particle interactions. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and neighbor searching.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetGridCellBuffer() const { return m_pGridCellBuffer; }

  /// Returns the grid cell range buffer, which contains the start and end indices of particles in each grid cell when GPU-based neighbor searching is enabled. This buffer is updated by the GPU during simulation passes when neighbor searching is enabled, and can be read by the renderer or by game logic for processing particle interactions. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and neighbor searching.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetGridCellRangeBuffer() const { return m_pGridCellRangeBuffer; }

  /// Returns the neighbor pair buffer, which contains pairs of neighboring particles when GPU-based neighbor searching is enabled. This buffer is updated by the GPU during simulation passes when neighbor searching is enabled, and can be read by the renderer or by game logic for processing particle interactions. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and neighbor searching.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetNeighborPairBuffer() const { return m_pNeighborPairBuffer; }

  /// Returns the draw indirect buffer, which contains the arguments for indirect draw calls. This buffer is updated by the GPU during simulation passes when indirect drawing is enabled, and can be read by the renderer for rendering. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and rendering.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetDrawIndirectBuffer() const { return m_pDrawIndirectBuffer; }

  /// Returns the dispatch indirect buffer, which contains the arguments for indirect dispatch calls (e.g., for culling or sorting). This buffer is updated by the GPU during simulation passes when async compute or GPU culling is enabled, and can be read by the renderer for rendering or by game logic for processing. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and any compute passes that use it.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetDispatchIndirectBuffer() const { return m_pDispatchIndirectBuffer; }

public:
  xiiParticleSystemRuntime();
  ~xiiParticleSystemRuntime();

  /// Initializes the runtime with the given device and descriptor, creating necessary GPU resources. Returns failure if initialization fails (e.g., due to insufficient GPU resources or invalid descriptor parameters), in which case the runtime should not be used.
  [[nodiscard]] xiiResult Initialize(xiiSharedPtr<xiiGALDevice> pDevice, const xiiParticleSystemDescriptor& descriptor);

  /// Shuts down the runtime and releases all GPU resources. After calling this, the runtime should not be used unless it is re-initialized.
  void Shutdown();

  /// Ensures that the runtime has sufficient capacity for the given descriptor, resizing GPU buffers if necessary. Returns failure if resizing fails (e.g., due to insufficient GPU resources), in which case the runtime may be left in a valid but potentially under-provisioned state.
  [[nodiscard]] xiiResult EnsureCapacity(const xiiParticleSystemDescriptor& descriptor);

  /// Returns whether the runtime has been successfully initialized and is ready for use. This checks whether necessary GPU resources have been created and are valid, but does not guarantee that the runtime is fully provisioned for a specific descriptor (e.g., if EnsureCapacity has not been called or has failed).
  [[nodiscard]] bool IsInitialized() const;

  /// Resets the simulation state, clearing all particles and events. This can be used to restart the simulation or to clear out any existing state before starting a new effect. If a command list is provided, the reset operations will be recorded on that command list; otherwise, they will be executed immediately on the current context.
  void ResetSimulationState(xiiGALCommandList* pCommandList = nullptr);

  /// Swaps the current and next particle state buffers, typically called at the end of a simulation pass to prepare for the next frame. This should be called after all simulation passes have been executed for the current frame, and before any rendering or readback operations that need to access the updated particle state. If double buffering is not enabled, this function may have no effect.
  void SwapParticleStateBuffers();

  /// Sets the node executor delegate, which is responsible for executing the logic of each particle graph node during simulation passes. This delegate will be called for each node in the graph that needs to be executed during a simulation pass, and should contain the logic for setting up shader parameters, dispatching compute shaders, and any other necessary operations to execute the node's behavior. The exact semantics of this delegate (e.g., how it's called and what it should do) are defined by the particle graph and may depend on the enabled features of the particle system.
  void SetNodeExecutor(xiiParticleGraphNodeExecutor executor);

  /// Adds the necessary simulation passes to the given render graph for executing the particle graph defined in the descriptor. This should be called during render graph construction, and will set up passes for emitting, simulating, and processing particles according to the logic defined in the particle graph resource. The sNamePrefix parameter can be used to give unique names to the passes for debugging purposes. Returns failure if adding the passes fails (e.g., due to invalid graph resources or issues with the render graph), in which case the render graph may be left in a valid but incomplete state.
  [[nodiscard]] xiiResult AddSimulationPasses(xiiRenderGraph& ref_graph, xiiStringView sNamePrefix, const xiiParticleSystemDescriptor& descriptor);

private:
  void SetupSimulationPass(xiiParticleSimulationPassData& ref_data, xiiRenderGraphBuilder& ref_builder);
  void ExecuteSimulationPass(const xiiParticleSimulationPassData& data, xiiRenderGraphPassContext& ref_context);

  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> CreateStructuredBuffer(xiiStringView sDebugName, xiiUInt64 uiElementCount, xiiUInt32 uiElementStride, xiiBitflags<xiiGALBindFlags> bindFlags) const;
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> CreateRawBuffer(xiiStringView sDebugName, xiiUInt64 uiByteSize, xiiBitflags<xiiGALBindFlags> bindFlags) const;

private:
  xiiSharedPtr<xiiGALDevice> m_pDevice;

  xiiSharedPtr<xiiGALBuffer> m_pParticleStateBuffers[2]; ///< Double buffer for particle state, allowing for reading from one buffer while writing to the other during simulation passes. The current and next buffers are swapped at the end of each frame. The exact layout of these buffers is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  xiiSharedPtr<xiiGALBuffer> m_pAliveIndexBuffer;        ///< Buffer containing the indices of currently alive particles, updated by the GPU during simulation passes. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  xiiSharedPtr<xiiGALBuffer> m_pDeadIndexBuffer;         ///< Buffer containing the indices of currently dead particles, updated by the GPU during simulation passes. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  xiiSharedPtr<xiiGALBuffer> m_pCountersBuffer;          ///< Buffer containing various counters related to the particle system (e.g., alive count, dead count, spawn request count, event count), updated by the GPU during simulation passes. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for rendering and simulation.
  xiiSharedPtr<xiiGALBuffer> m_pEventBuffer;             ///< Buffer containing events emitted by particles during simulation passes, updated by the GPU when event support is enabled. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and event processing.
  xiiSharedPtr<xiiGALBuffer> m_pSortKeyBuffer;           ///< Buffer containing keys used for sorting particles (e.g., by depth or by ID), updated by the GPU during simulation passes when sorting is enabled. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and rendering.
  xiiSharedPtr<xiiGALBuffer> m_pGridCellBuffer;          ///< Buffer containing the spatial partitioning of particles into grid cells when GPU-based neighbor searching is enabled, updated by the GPU during simulation passes. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and neighbor searching.
  xiiSharedPtr<xiiGALBuffer> m_pGridCellRangeBuffer;     ///< Buffer containing the start and end indices of particles in each grid cell when GPU-based neighbor searching is enabled, updated by the GPU during simulation passes. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and neighbor searching.
  xiiSharedPtr<xiiGALBuffer> m_pNeighborPairBuffer;      ///< Buffer containing pairs of neighboring particles when GPU-based neighbor searching is enabled, updated by the GPU during simulation passes. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and neighbor searching.
  xiiSharedPtr<xiiGALBuffer> m_pDrawIndirectBuffer;      ///< Buffer containing the arguments for indirect draw calls, updated by the GPU during simulation passes when indirect drawing is enabled. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and rendering.
  xiiSharedPtr<xiiGALBuffer> m_pDispatchIndirectBuffer;  ///< Buffer containing the arguments for indirect dispatch calls (e.g., for culling or sorting), updated by the GPU during simulation passes when async compute or GPU culling is enabled. The exact layout of this buffer is defined by the particle graph and should be compatible with the shader code used for simulation and any compute passes that use it.

  xiiParticleGraphNodeExecutor m_NodeExecutor;         ///< Delegate for executing particle graph nodes during simulation passes. This should be set by the user of the runtime to provide the logic for executing each node in the particle graph, including setting up shader parameters and dispatching compute shaders as needed.
  xiiParticleSystemDescriptor  m_Descriptor;           ///< The descriptor for the particle system that this runtime is configured for. This is used for reference and may be updated when EnsureCapacity is called with a new descriptor, but the runtime should be able to handle changes in the descriptor (e.g., changes in max particles or enabled features) without needing to be re-initialized, as long as EnsureCapacity is called to adjust resources accordingly.
  xiiString                    m_sGraphResourcePrefix; ///< A string prefix used for naming graph resources (e.g., buffers) for debugging purposes. This can help to identify which resources belong to which particle systems when analyzing GPU resource usage or debugging issues.

  xiiUInt32 m_uiParticleCapacity     = 0U; ///< The current capacity for particles in the system, determined by the size of the allocated GPU buffers and the limits defined in the descriptor. This is used for validating buffer sizes and for making decisions about how to configure emitters or other aspects of the particle graph based on the available capacity.
  xiiUInt32 m_uiEventCapacity        = 0U; ///< The current capacity for events in the system, determined by the size of the allocated GPU buffers and the limits defined in the descriptor. This is used for validating buffer sizes and for making decisions about how to configure event emitters or other aspects of the particle graph based on the available capacity for events.
  xiiUInt32 m_uiNeighborPairCapacity = 0U; ///< The current capacity for neighbor pairs in the system, determined by the size of the allocated GPU buffers and the limits defined in the descriptor. This is used for validating buffer sizes and for making decisions about how to configure particle interactions or other aspects of the particle graph based on the available capacity for neighbor pairs.
  xiiUInt32 m_uiReadBufferIndex      = 0U; ///< The index of the current read buffer in the double buffer for particle state. This is used to determine which buffer to read from and which buffer to write to during simulation passes, and is swapped at the end of each frame when SwapParticleStateBuffers() is called.
};

/// Scene component that exposes a GPU particle system to extraction and tooling.
class XII_GRAPHICSCORE_DLL xiiParticleSystemComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleSystemComponent, xiiRenderComponent, xiiParticleSystemComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiParticleSystemComponent
public:
  xiiParticleSystemComponent();
  ~xiiParticleSystemComponent();

  /// Sets the descriptor for the particle system, which defines the behavior of the simulation and rendering. This can be called at any time to update the configuration of the particle system, but changes will only take effect after PrepareRuntimeResources() is called to ensure that the runtime has sufficient resources for the new descriptor. The exact semantics of changing the descriptor (e.g., whether it resets the simulation state or requires re-initialization) are defined by the implementation of the component and runtime.
  void SetDescriptor(const xiiParticleSystemDescriptor& descriptor);

  /// Returns the current descriptor for the particle system, which contains the configuration for the simulation and rendering. This can be used for reference or for making decisions about how to configure emitters or other aspects of the particle graph based on the current settings. The exact semantics of the descriptor (e.g., whether it reflects pending changes that have not yet been applied to the runtime) are defined by the implementation of the component and runtime.
  const xiiParticleSystemDescriptor& GetDescriptor() const;

  /// Sets the particle graph resource handle, which defines the logic of the simulation and rendering. This can be called at any time to update the behavior of the particle system, but changes will only take effect after PrepareRuntimeResources() is called to ensure that the runtime has sufficient resources for the new graph. The exact semantics of changing the graph (e.g., whether it resets the simulation state or requires re-initialization) are defined by the implementation of the component and runtime.
  void SetParticleGraph(const xiiParticleGraphResourceHandle& hGraph);

  /// Returns the current particle graph resource handle, which defines the logic of the simulation and rendering. This can be used for reference or for making decisions about how to configure emitters or other aspects of the particle graph based on the current graph. The exact semantics of the graph (e.g., whether it reflects pending changes that have not yet been applied to the runtime) are defined by the implementation of the component and runtime.
  const xiiParticleGraphResourceHandle& GetParticleGraph() const;

  /// Prepares the runtime resources for the particle system based on the current descriptor and graph settings. This should be called after setting the descriptor and graph to ensure that the runtime has sufficient resources for simulation and rendering. Returns failure if resource preparation fails (e.g., due to insufficient GPU resources or invalid descriptor/graph parameters), in which case the runtime may be left in an uninitialized or partially initialized state.
  [[nodiscard]] xiiResult PrepareRuntimeResources(xiiSharedPtr<xiiGALDevice> pDevice) const;

  /// Returns a reference to the particle system runtime, which manages the GPU resources and simulation for this particle system. This can be used for accessing buffers, adding simulation passes, or other operations related to the runtime. The exact semantics of the runtime (e.g., whether it reflects pending changes that have not yet been applied to GPU resources) are defined by the implementation of the component and runtime.
  [[nodiscard]] xiiParticleSystemRuntime& GetRuntime() const { return m_Runtime; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiParticleSystemDescriptor      m_Descriptor; ///< The descriptor for the particle system, which contains the configuration for the simulation and rendering. This is set by the user of the component and is used to prepare the runtime resources and to provide information for rendering. The exact semantics of this descriptor (e.g., whether it reflects pending changes that have not yet been applied to the runtime) are defined by the implementation of the component and runtime.
  mutable xiiParticleSystemRuntime m_Runtime;    ///< The runtime for the particle system, which manages the GPU resources and simulation. This is mutable to allow for lazy initialization and resource preparation based on the current descriptor and graph settings. The exact semantics of the runtime (e.g., whether it reflects pending changes that have not yet been applied to GPU resources) are defined by the implementation of the component and runtime.
};
