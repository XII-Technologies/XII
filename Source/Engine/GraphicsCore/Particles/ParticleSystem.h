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

/// \brief Constants shared by the CPU runtime and shader-side particle layouts.
struct XII_GRAPHICSCORE_DLL xiiParticleSystemConstants
{
  static constexpr xiiUInt32 s_uiDefaultMaxParticles    = 1024U * 1024U;
  static constexpr xiiUInt32 s_uiDefaultMaxEmitters     = 1024U;
  static constexpr xiiUInt32 s_uiDefaultMaxEvents       = 1024U * 1024U;
  static constexpr xiiUInt32 s_uiDefaultThreadGroupSize = 64U;
  static constexpr xiiUInt32 s_uiMaxSupportedParticles  = 16U * 1024U * 1024U;
};

/// \brief Simulation space for particle data.
struct XII_GRAPHICSCORE_DLL xiiParticleSimulationSpace
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    World = 0U,
    Local,
    View,

    ENUM_COUNT,

    Default = World
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleSimulationSpace);

/// \brief Feature flags controlling runtime allocation and pass scheduling.
struct XII_GRAPHICSCORE_DLL xiiParticleSystemFlags
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    None                = 0U,
    GPUDriven           = XII_BIT(0),
    AsyncCompute        = XII_BIT(1),
    IndirectDraw        = XII_BIT(2),
    GPUCulling          = XII_BIT(3),
    SortByDepth         = XII_BIT(4),
    StableParticleIds   = XII_BIT(5),
    EnableEvents        = XII_BIT(6),
    EnableReadback      = XII_BIT(7),
    Deterministic       = XII_BIT(8),
    MolecularDynamics   = XII_BIT(9),
    NeighborSearch      = XII_BIT(10),
    DoubleBufferedState = XII_BIT(11),

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

/// \brief GPU particle state. Keep in sync with Data/Base/Shaders/Pipeline/GPUParticleSimulate.xiiShader.
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
  xiiUInt32 m_uiId      = 0U;
  xiiUInt32 m_uiFlags   = 0U;
  xiiUInt32 m_uiEmitter = 0U;
  xiiUInt32 m_uiCellId  = 0U;
};

static_assert((sizeof(xiiParticleGPUState) % 16U) == 0U);

/// \brief GPU counters consumed by emit, compact, simulate, render and readback passes.
struct XII_GRAPHICSCORE_DLL xiiParticleGPUCounters
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiAliveCount        = 0U;
  xiiUInt32 m_uiDeadCount         = 0U;
  xiiUInt32 m_uiSpawnRequestCount = 0U;
  xiiUInt32 m_uiEventCount        = 0U;
};

/// \brief Runtime descriptor for large GPU particle simulations.
struct XII_GRAPHICSCORE_DLL xiiParticleSystemDescriptor
{
  xiiParticleGraphResourceHandle      m_hGraph;
  xiiEnum<xiiParticleSimulationSpace> m_SimulationSpace;
  xiiBitflags<xiiParticleSystemFlags> m_Flags;
  xiiBoundingBoxSphere                m_LocalBounds        = xiiBoundingBoxSphere::MakeZero();
  xiiUInt32                           m_uiMaxParticles     = xiiParticleSystemConstants::s_uiDefaultMaxParticles;
  xiiUInt32                           m_uiMaxEmitters      = xiiParticleSystemConstants::s_uiDefaultMaxEmitters;
  xiiUInt32                           m_uiMaxEvents        = xiiParticleSystemConstants::s_uiDefaultMaxEvents;
  xiiUInt32                           m_uiMaxNeighborPairs = 0U;
  xiiUInt32                           m_uiRandomSeed       = 0U;
  xiiUInt8                            m_uiMaxSubSteps      = 1U;
  float                               m_fFixedTimeStep     = 1.0f / 60.0f;
  float                               m_fNeighborCellSize  = 1.0f;
  bool                                m_bAlwaysVisible     = false;

  xiiVec3 GetLocalBoundsCenter() const { return m_LocalBounds.m_vCenter; }
  void    SetLocalBoundsCenter(xiiVec3 vCenter) { m_LocalBounds.m_vCenter = vCenter; }

  xiiVec3 GetLocalBoundsHalfExtents() const { return m_LocalBounds.m_vBoxHalfExtents; }
  void    SetLocalBoundsHalfExtents(xiiVec3 vHalfExtents)
  {
    m_LocalBounds.m_vBoxHalfExtents = vHalfExtents.CompMax(xiiVec3::MakeZero());
    m_LocalBounds.m_fSphereRadius   = m_LocalBounds.m_vBoxHalfExtents.GetLength();
  }

  float GetLocalBoundsRadius() const { return m_LocalBounds.m_fSphereRadius; }
  void  SetLocalBoundsRadius(float fRadius) { m_LocalBounds.m_fSphereRadius = xiiMath::Max(0.0f, fRadius); }

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleSystemDescriptor);

/// \brief Renderer-facing packet for GPU particle systems.
class XII_GRAPHICSCORE_DLL xiiParticleRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleRenderData, xiiRenderData);

public:
  xiiParticleSystemDescriptor m_Descriptor;
  xiiUInt32                   m_uiUniqueID = 0U;

  xiiSharedPtr<xiiGALBuffer> m_pParticleStateBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pAliveIndexBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pCountersBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pDrawIndirectBuffer;

  xiiParticleSystemRuntime* m_pRuntime = nullptr;
};

/// \brief Render graph pass data used by xiiParticleSystemRuntime::AddSimulationPasses().
struct XII_GRAPHICSCORE_DLL xiiParticleSimulationPassData
{
  xiiRGBufferHandle m_hParticleStateRead;
  xiiRGBufferHandle m_hParticleStateWrite;
  xiiRGBufferHandle m_hAliveIndexBuffer;
  xiiRGBufferHandle m_hDeadIndexBuffer;
  xiiRGBufferHandle m_hCountersBuffer;
  xiiRGBufferHandle m_hEventBuffer;
  xiiRGBufferHandle m_hSortKeyBuffer;
  xiiRGBufferHandle m_hNeighborPairBuffer;
  xiiRGBufferHandle m_hDrawIndirectBuffer;
  xiiRGBufferHandle m_hDispatchIndirectBuffer;

  xiiParticleGraphResourceHandle m_hGraph;
  xiiUInt32                      m_uiParticleCapacity = 0U;
  xiiUInt32                      m_uiDispatchGroups   = 0U;
};

using xiiParticleGraphNodeExecutor = xiiDelegate<void(const xiiParticleGraphNodeDesc&, const xiiParticleSimulationPassData&, xiiRGPassContext&)>;

/// \brief Persistent GPU runtime for one particle system. Owns large buffers and registers graph passes.
class XII_GRAPHICSCORE_DLL xiiParticleSystemRuntime
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiParticleSystemRuntime);

public:
  xiiParticleSystemRuntime();
  ~xiiParticleSystemRuntime();

  [[nodiscard]] xiiResult Initialize(xiiSharedPtr<xiiGALDevice> pDevice, const xiiParticleSystemDescriptor& descriptor);
  void                    Shutdown();

  [[nodiscard]] xiiResult EnsureCapacity(const xiiParticleSystemDescriptor& descriptor);
  [[nodiscard]] bool      IsInitialized() const;

  void ResetSimulationState(xiiGALCommandList* pCommandList = nullptr);
  void SwapParticleStateBuffers();

  [[nodiscard]] xiiUInt32 GetParticleCapacity() const { return m_uiParticleCapacity; }
  [[nodiscard]] xiiUInt32 GetEventCapacity() const { return m_uiEventCapacity; }
  [[nodiscard]] xiiUInt32 GetNeighborPairCapacity() const { return m_uiNeighborPairCapacity; }

  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetCurrentParticleStateBuffer() const;
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetNextParticleStateBuffer() const;
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetAliveIndexBuffer() const { return m_pAliveIndexBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetDeadIndexBuffer() const { return m_pDeadIndexBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetCountersBuffer() const { return m_pCountersBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetEventBuffer() const { return m_pEventBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetSortKeyBuffer() const { return m_pSortKeyBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetGridCellBuffer() const { return m_pGridCellBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetNeighborPairBuffer() const { return m_pNeighborPairBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetDrawIndirectBuffer() const { return m_pDrawIndirectBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetDispatchIndirectBuffer() const { return m_pDispatchIndirectBuffer; }

  void SetNodeExecutor(xiiParticleGraphNodeExecutor executor);

  [[nodiscard]] xiiResult AddSimulationPasses(xiiRenderGraph& ref_graph, xiiStringView sNamePrefix, const xiiParticleSystemDescriptor& descriptor);

private:
  void SetupSimulationPass(xiiParticleSimulationPassData& ref_data, xiiRGBuilder& ref_builder);
  void ExecuteSimulationPass(const xiiParticleSimulationPassData& data, xiiRGPassContext& ref_context);

  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> CreateStructuredBuffer(xiiStringView sDebugName, xiiUInt64 uiElementCount, xiiUInt32 uiElementStride, xiiBitflags<xiiGALBindFlags> bindFlags) const;
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> CreateRawBuffer(xiiStringView sDebugName, xiiUInt64 uiByteSize, xiiBitflags<xiiGALBindFlags> bindFlags) const;

private:
  xiiSharedPtr<xiiGALDevice> m_pDevice;

  xiiSharedPtr<xiiGALBuffer> m_pParticleStateBuffers[2];
  xiiSharedPtr<xiiGALBuffer> m_pAliveIndexBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pDeadIndexBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pCountersBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pEventBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pSortKeyBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pGridCellBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pGridCellRangeBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pNeighborPairBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pDrawIndirectBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pDispatchIndirectBuffer;

  xiiParticleGraphNodeExecutor m_NodeExecutor;
  xiiParticleSystemDescriptor  m_Descriptor;
  xiiString                    m_sGraphResourcePrefix;

  xiiUInt32 m_uiParticleCapacity     = 0U;
  xiiUInt32 m_uiEventCapacity        = 0U;
  xiiUInt32 m_uiNeighborPairCapacity = 0U;
  xiiUInt32 m_uiReadBufferIndex      = 0U;
};

/// \brief Scene component that exposes a GPU particle system to extraction and tooling.
class XII_GRAPHICSCORE_DLL xiiParticleSystemComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleSystemComponent, xiiRenderComponent, xiiParticleSystemComponentManager);

public:
  xiiParticleSystemComponent();
  ~xiiParticleSystemComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  void                               SetDescriptor(const xiiParticleSystemDescriptor& descriptor);
  const xiiParticleSystemDescriptor& GetDescriptor() const;

  void                                  SetParticleGraph(const xiiParticleGraphResourceHandle& hGraph);
  const xiiParticleGraphResourceHandle& GetParticleGraph() const;

  [[nodiscard]] xiiResult                 PrepareRuntimeResources(xiiSharedPtr<xiiGALDevice> pDevice) const;
  [[nodiscard]] xiiParticleSystemRuntime& GetRuntime() const { return m_Runtime; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiParticleSystemDescriptor      m_Descriptor;
  mutable xiiParticleSystemRuntime m_Runtime;
};
