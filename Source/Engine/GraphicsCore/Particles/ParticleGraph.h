/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

using xiiParticleGraphResourceHandle = xiiTypedResourceHandle<class xiiParticleGraphResource>;

/// \brief Attribute data format used by particle graph pins and GPU attribute streams.
struct XII_GRAPHICSCORE_DLL xiiParticleAttributeFormat
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Float = 0U,
    Float2,
    Float3,
    Float4,
    UInt,
    UInt2,
    UInt4,
    Int,
    Int2,
    Int4,

    ENUM_COUNT,

    Default = Float4
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleAttributeFormat);

/// \brief Built-in particle attribute semantics. Custom attributes are named by xiiParticleGraphPinDesc::m_sAttributeName.
struct XII_GRAPHICSCORE_DLL xiiParticleAttributeSemantic
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Custom = 0U,
    Position,
    PreviousPosition,
    Velocity,
    Acceleration,
    Mass,
    InverseMass,
    Radius,
    Color,
    Age,
    Lifetime,
    Orientation,
    AngularVelocity,
    Temperature,
    Density,
    Charge,
    MoleculeId,
    CellId,
    SortKey,

    ENUM_COUNT,

    Default = Custom
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleAttributeSemantic);

/// \brief High-level graph node category for editor grouping and compile scheduling.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphNodeCategory
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Emitter = 0U,
    Initialize,
    Simulation,
    Solver,
    Constraint,
    Collision,
    Event,
    Render,
    Utility,
    IO,
    Custom,

    ENUM_COUNT,

    Default = Simulation
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphNodeCategory);

/// \brief Where and how often a particle graph node executes.
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

/// \brief Node capabilities and compiler hints.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphNodeFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None                    = 0U,
    HasSideEffects          = XII_BIT(0),
    ReadsNeighborGrid       = XII_BIT(1),
    WritesNeighborGrid      = XII_BIT(2),
    EmitsEvents             = XII_BIT(3),
    ConsumesEvents          = XII_BIT(4),
    SupportsAsyncCompute    = XII_BIT(5),
    RequiresDeterminism     = XII_BIT(6),
    SupportsMolecularDomain = XII_BIT(7),
    ToolOnly                = XII_BIT(8),

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

/// \brief One input or output pin visible in the particle graph editor.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphPinDesc
{
  xiiHashedString                       m_sName;
  xiiHashedString                       m_sAttributeName;
  xiiEnum<xiiParticleAttributeSemantic> m_Semantic;
  xiiEnum<xiiParticleAttributeFormat>   m_Format;
  xiiVariant                            m_DefaultValue;
  bool                                  m_bRequired     = false;
  bool                                  m_bMultiConnect = false;
  bool                                  m_bHidden       = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphPinDesc);

/// \brief Artist/scientist-facing parameter exposed by a particle graph node.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphParameterDesc
{
  xiiHashedString m_sName;
  xiiString       m_sDisplayName;
  xiiString       m_sCategory;
  xiiString       m_sTooltip;
  xiiVariant      m_DefaultValue;
  xiiVariant      m_MinValue;
  xiiVariant      m_MaxValue;
  bool            m_bAnimatable = true;
  bool            m_bAdvanced   = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphParameterDesc);

/// \brief A particle graph node. Nodes map to compute kernels, tool-only annotations, or custom engine callbacks.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphNodeDesc
{
  xiiUuid                                m_NodeId;
  xiiHashedString                        m_sType;
  xiiString                              m_sDisplayName;
  xiiString                              m_sKernelPath;
  xiiString                              m_sEntryPoint;
  xiiEnum<xiiParticleGraphNodeCategory>  m_Category;
  xiiEnum<xiiParticleGraphSchedule>      m_Schedule;
  xiiBitflags<xiiParticleGraphNodeFlags> m_Flags;
  xiiVec2                                m_vEditorPosition   = xiiVec2::MakeZero();
  xiiColor                               m_DebugColor        = xiiColor::White;
  xiiUInt32                              m_uiThreadGroupSize = 64U;
  xiiUInt32                              m_uiEstimatedCost   = 1U;

  xiiDynamicArray<xiiParticleGraphPinDesc>       m_Inputs;
  xiiDynamicArray<xiiParticleGraphPinDesc>       m_Outputs;
  xiiDynamicArray<xiiParticleGraphParameterDesc> m_Parameters;
  xiiDynamicArray<xiiHashedString>               m_ReadAttributes;
  xiiDynamicArray<xiiHashedString>               m_WriteAttributes;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphNodeDesc);

/// \brief Directed connection between two particle graph pins.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphLinkDesc
{
  xiiUuid         m_SourceNode;
  xiiHashedString m_sSourcePin;
  xiiUuid         m_TargetNode;
  xiiHashedString m_sTargetPin;
  bool            m_bEnabled = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphLinkDesc);

/// \brief Editor annotation/grouping data for DCC-style graph tooling.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphGroupDesc
{
  xiiUuid                  m_GroupId;
  xiiString                m_sTitle;
  xiiColor                 m_Color     = xiiColor::White;
  xiiVec2                  m_vPosition = xiiVec2::MakeZero();
  xiiVec2                  m_vSize     = xiiVec2(320.0f, 180.0f);
  xiiDynamicArray<xiiUuid> m_Nodes;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphGroupDesc);

/// \brief Serializable particle graph descriptor. It is intentionally data-only so tools can round-trip it losslessly.
struct XII_GRAPHICSCORE_DLL xiiParticleGraphResourceDescriptor
{
  xiiUInt32 m_uiVersion = 1U;
  xiiString m_sGraphName;
  xiiString m_sDescription;
  xiiString m_sAuthoringTool;

  xiiDynamicArray<xiiParticleGraphNodeDesc>      m_Nodes;
  xiiDynamicArray<xiiParticleGraphLinkDesc>      m_Links;
  xiiDynamicArray<xiiParticleGraphGroupDesc>     m_Groups;
  xiiDynamicArray<xiiParticleGraphParameterDesc> m_ExposedParameters;

  xiiUuid AddNode(const xiiParticleGraphNodeDesc& node);
  bool    RemoveNode(const xiiUuid& nodeId);

  void AddLink(const xiiParticleGraphLinkDesc& link);
  void Clear();

  [[nodiscard]] const xiiParticleGraphNodeDesc* FindNode(const xiiUuid& nodeId) const;
  [[nodiscard]] xiiParticleGraphNodeDesc*       FindNode(const xiiUuid& nodeId);

  [[nodiscard]] xiiResult Validate(xiiStringBuilder* out_pError = nullptr) const;
  [[nodiscard]] xiiUInt64 ComputePipelineHash() const;

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleGraphResourceDescriptor);

/// \brief Resource wrapper around a DCC-authored GPU particle graph.
class XII_GRAPHICSCORE_DLL xiiParticleGraphResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleGraphResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiParticleGraphResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiParticleGraphResource, xiiParticleGraphResourceDescriptor);

public:
  xiiParticleGraphResource();

  [[nodiscard]] XII_ALWAYS_INLINE const xiiParticleGraphResourceDescriptor& GetDescriptor() const { return m_Descriptor; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64                                 GetPipelineHash() const { return m_uiPipelineHash; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiParticleGraphResourceDescriptor m_Descriptor;
  xiiUInt64                          m_uiPipelineHash = 0ULL;
};
