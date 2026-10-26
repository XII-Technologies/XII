/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Random.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Particles/ParticleSystemManager.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleSimulationSpace, 1)
  XII_ENUM_CONSTANT(xiiParticleSimulationSpace::World),
  XII_ENUM_CONSTANT(xiiParticleSimulationSpace::Local),
  XII_ENUM_CONSTANT(xiiParticleSimulationSpace::View),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiParticleSystemFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::None),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::GPUDriven),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::AsyncCompute),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::IndirectDraw),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::GPUCulling),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::SortByDepth),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::StableParticleIds),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::EnableEvents),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::EnableReadback),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::Deterministic),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::MolecularDynamics),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::NeighborSearch),
  XII_BITFLAGS_CONSTANT(xiiParticleSystemFlags::DoubleBufferedState),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiParticleSystemDescriptor, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiParticleSystemDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("Graph", m_hGraph)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_ParticleGraph", xiiDependencyFlags::Package)),
    XII_ENUM_MEMBER_PROPERTY("SimulationSpace", xiiParticleSimulationSpace, m_SimulationSpace),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiParticleSystemFlags, m_Flags),
    XII_ACCESSOR_PROPERTY("LocalBoundsCenter", GetLocalBoundsCenter, SetLocalBoundsCenter),
    XII_ACCESSOR_PROPERTY("LocalBoundsHalfExtents", GetLocalBoundsHalfExtents, SetLocalBoundsHalfExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3::MakeZero()), new xiiClampValueAttribute(xiiVec3::MakeZero(), xiiVariant())),
    XII_ACCESSOR_PROPERTY("LocalBoundsRadius", GetLocalBoundsRadius, SetLocalBoundsRadius)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("MaxParticles", m_uiMaxParticles)->AddAttributes(new xiiClampValueAttribute(1U, xiiParticleSystemConstants::s_uiMaxSupportedParticles), new xiiDefaultValueAttribute(xiiParticleSystemConstants::s_uiDefaultMaxParticles)),
    XII_MEMBER_PROPERTY("MaxEmitters", m_uiMaxEmitters)->AddAttributes(new xiiClampValueAttribute(1U, 65536U), new xiiDefaultValueAttribute(xiiParticleSystemConstants::s_uiDefaultMaxEmitters)),
    XII_MEMBER_PROPERTY("MaxEvents", m_uiMaxEvents)->AddAttributes(new xiiClampValueAttribute(0U, xiiParticleSystemConstants::s_uiMaxSupportedParticles), new xiiDefaultValueAttribute(xiiParticleSystemConstants::s_uiDefaultMaxEvents)),
    XII_MEMBER_PROPERTY("MaxNeighborPairs", m_uiMaxNeighborPairs)->AddAttributes(new xiiClampValueAttribute(0U, xiiParticleSystemConstants::s_uiMaxSupportedParticles * 8U)),
    XII_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    XII_MEMBER_PROPERTY("MaxSubSteps", m_uiMaxSubSteps)->AddAttributes(new xiiClampValueAttribute(1U, 16U), new xiiDefaultValueAttribute(1U)),
    XII_MEMBER_PROPERTY("FixedTimeStep", m_fFixedTimeStep)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiDefaultValueAttribute(1.0f / 60.0f)),
    XII_MEMBER_PROPERTY("NeighborCellSize", m_fNeighborCellSize)->AddAttributes(new xiiClampValueAttribute(0.0001f, 1000000.0f), new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("AlwaysVisible", m_bAlwaysVisible),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleRenderData, 1, xiiRTTIDefaultAllocator<xiiParticleRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiParticleSystemComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Descriptor", m_Descriptor),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Graphics/Particles"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

namespace
{
  enum class ParticleSystemDescriptorVersion : xiiUInt8
  {
    Version1 = 1U,

    ENUM_COUNT,

    Current = Version1
  };

  static xiiUInt32 ClampParticleCapacity(xiiUInt32 uiCapacity)
  {
    return xiiMath::Clamp(uiCapacity, 1U, xiiParticleSystemConstants::s_uiMaxSupportedParticles);
  }

  static xiiUInt64 SafeStructuredSize(xiiUInt64 uiElementCount, xiiUInt32 uiStride)
  {
    return xiiMath::Max<xiiUInt64>(1ULL, uiElementCount) * uiStride;
  }

  static void AddName(xiiStringBuilder& ref_name, xiiStringView sPrefix, xiiStringView sSuffix)
  {
    ref_name.Set(sPrefix);
    if (!ref_name.IsEmpty())
      ref_name.Append(".");
    ref_name.Append(sSuffix);
  }
} // namespace

void xiiParticleSystemDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8 uiVersion = (xiiUInt8)ParticleSystemDescriptorVersion::Current;

  ref_stream << uiVersion;
  ref_stream << m_hGraph;
  ref_stream << m_SimulationSpace;
  ref_stream << m_Flags;
  ref_stream << m_LocalBounds;
  ref_stream << m_uiMaxParticles;
  ref_stream << m_uiMaxEmitters;
  ref_stream << m_uiMaxEvents;
  ref_stream << m_uiMaxNeighborPairs;
  ref_stream << m_uiRandomSeed;
  ref_stream << m_uiMaxSubSteps;
  ref_stream << m_fFixedTimeStep;
  ref_stream << m_fNeighborCellSize;
  ref_stream << m_bAlwaysVisible;
}

void xiiParticleSystemDescriptor::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = 0U;
  ref_stream >> uiVersion;
  XII_IGNORE_UNUSED(uiVersion);

  ref_stream >> m_hGraph;
  ref_stream >> m_SimulationSpace;
  ref_stream >> m_Flags;
  ref_stream >> m_LocalBounds;
  ref_stream >> m_uiMaxParticles;
  ref_stream >> m_uiMaxEmitters;
  ref_stream >> m_uiMaxEvents;
  ref_stream >> m_uiMaxNeighborPairs;
  ref_stream >> m_uiRandomSeed;
  ref_stream >> m_uiMaxSubSteps;
  ref_stream >> m_fFixedTimeStep;
  ref_stream >> m_fNeighborCellSize;
  ref_stream >> m_bAlwaysVisible;

  m_uiMaxParticles    = ClampParticleCapacity(m_uiMaxParticles);
  m_uiMaxEmitters     = xiiMath::Max(1U, m_uiMaxEmitters);
  m_uiMaxSubSteps     = xiiMath::Max<xiiUInt8>(1U, m_uiMaxSubSteps);
  m_fNeighborCellSize = xiiMath::Max(0.0001f, m_fNeighborCellSize);
}

xiiParticleSystemRuntime::xiiParticleSystemRuntime() = default;

xiiParticleSystemRuntime::~xiiParticleSystemRuntime()
{
  Shutdown();
}

xiiResult xiiParticleSystemRuntime::Initialize(xiiSharedPtr<xiiGALDevice> pDevice, const xiiParticleSystemDescriptor& descriptor)
{
  if (pDevice == nullptr)
    return XII_FAILURE;

  m_pDevice = std::move(pDevice);
  return EnsureCapacity(descriptor);
}

void xiiParticleSystemRuntime::Shutdown()
{
  m_pParticleStateBuffers[0].Clear();
  m_pParticleStateBuffers[1].Clear();
  m_pAliveIndexBuffer.Clear();
  m_pDeadIndexBuffer.Clear();
  m_pCountersBuffer.Clear();
  m_pEventBuffer.Clear();
  m_pSortKeyBuffer.Clear();
  m_pGridCellBuffer.Clear();
  m_pGridCellRangeBuffer.Clear();
  m_pNeighborPairBuffer.Clear();
  m_pDrawIndirectBuffer.Clear();
  m_pDispatchIndirectBuffer.Clear();
  m_pDevice.Clear();

  m_uiParticleCapacity     = 0U;
  m_uiEventCapacity        = 0U;
  m_uiNeighborPairCapacity = 0U;
  m_uiReadBufferIndex      = 0U;
}

xiiResult xiiParticleSystemRuntime::EnsureCapacity(const xiiParticleSystemDescriptor& descriptor)
{
  if (m_pDevice == nullptr)
    return XII_FAILURE;

  const xiiUInt32 uiParticleCapacity     = ClampParticleCapacity(descriptor.m_uiMaxParticles);
  const xiiUInt32 uiEventCapacity        = xiiMath::Max(1U, descriptor.m_uiMaxEvents);
  const xiiUInt32 uiNeighborPairCapacity = descriptor.m_Flags.IsAnySet(xiiParticleSystemFlags::NeighborSearch | xiiParticleSystemFlags::MolecularDynamics) ? xiiMath::Max(1U, descriptor.m_uiMaxNeighborPairs) : 1U;

  if (m_uiParticleCapacity == uiParticleCapacity && m_uiEventCapacity == uiEventCapacity && m_uiNeighborPairCapacity == uiNeighborPairCapacity && IsInitialized())
  {
    m_Descriptor = descriptor;
    return XII_SUCCESS;
  }

  m_Descriptor             = descriptor;
  m_uiParticleCapacity     = uiParticleCapacity;
  m_uiEventCapacity        = uiEventCapacity;
  m_uiNeighborPairCapacity = uiNeighborPairCapacity;
  m_uiReadBufferIndex      = 0U;

  const xiiBitflags<xiiGALBindFlags> structuredReadWrite = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;

  m_pParticleStateBuffers[0] = CreateStructuredBuffer("ParticleStateA", m_uiParticleCapacity, sizeof(xiiParticleGPUState), structuredReadWrite);
  m_pParticleStateBuffers[1] = CreateStructuredBuffer("ParticleStateB", m_uiParticleCapacity, sizeof(xiiParticleGPUState), structuredReadWrite);
  m_pAliveIndexBuffer        = CreateStructuredBuffer("ParticleAliveIndices", m_uiParticleCapacity, sizeof(xiiUInt32), structuredReadWrite);
  m_pDeadIndexBuffer         = CreateStructuredBuffer("ParticleDeadIndices", m_uiParticleCapacity, sizeof(xiiUInt32), structuredReadWrite);
  m_pCountersBuffer          = CreateStructuredBuffer("ParticleCounters", 1U, sizeof(xiiParticleGPUCounters), structuredReadWrite);
  m_pEventBuffer             = CreateStructuredBuffer("ParticleEvents", m_uiEventCapacity, sizeof(xiiVec4), structuredReadWrite);
  m_pSortKeyBuffer           = CreateStructuredBuffer("ParticleSortKeys", m_uiParticleCapacity, sizeof(xiiVec4), structuredReadWrite);
  m_pGridCellBuffer          = CreateStructuredBuffer("ParticleGridCells", m_uiParticleCapacity, sizeof(xiiUInt32), structuredReadWrite);
  m_pGridCellRangeBuffer     = CreateStructuredBuffer("ParticleGridCellRanges", m_uiParticleCapacity, sizeof(xiiVec4), structuredReadWrite);
  m_pNeighborPairBuffer      = CreateStructuredBuffer("ParticleNeighborPairs", m_uiNeighborPairCapacity, sizeof(xiiVec4), structuredReadWrite);
  m_pDrawIndirectBuffer      = CreateRawBuffer("ParticleDrawIndirect", sizeof(xiiUInt32) * 4U, structuredReadWrite | xiiGALBindFlags::IndirectDrawArguments);
  m_pDispatchIndirectBuffer  = CreateRawBuffer("ParticleDispatchIndirect", sizeof(xiiUInt32) * 3U, structuredReadWrite | xiiGALBindFlags::IndirectDrawArguments);

  if (!IsInitialized())
  {
    Shutdown();
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

bool xiiParticleSystemRuntime::IsInitialized() const
{
  return m_pDevice != nullptr && m_pParticleStateBuffers[0] != nullptr && m_pParticleStateBuffers[1] != nullptr && m_pAliveIndexBuffer != nullptr && m_pDeadIndexBuffer != nullptr && m_pCountersBuffer != nullptr && m_pDrawIndirectBuffer != nullptr && m_pDispatchIndirectBuffer != nullptr;
}

void xiiParticleSystemRuntime::ResetSimulationState(xiiGALCommandList* pCommandList)
{
  if (pCommandList == nullptr || m_pCountersBuffer == nullptr)
    return;

  xiiParticleGPUCounters counters;
  counters.m_uiDeadCount = m_uiParticleCapacity;

  pCommandList->UpdateBuffer(m_pCountersBuffer.Borrow(), 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&counters), sizeof(counters)));
}

void xiiParticleSystemRuntime::SwapParticleStateBuffers()
{
  m_uiReadBufferIndex = 1U - m_uiReadBufferIndex;
}

xiiSharedPtr<xiiGALBuffer> xiiParticleSystemRuntime::GetCurrentParticleStateBuffer() const
{
  return m_pParticleStateBuffers[m_uiReadBufferIndex];
}

xiiSharedPtr<xiiGALBuffer> xiiParticleSystemRuntime::GetNextParticleStateBuffer() const
{
  return m_pParticleStateBuffers[1U - m_uiReadBufferIndex];
}

void xiiParticleSystemRuntime::SetNodeExecutor(xiiParticleGraphNodeExecutor executor)
{
  m_NodeExecutor = executor;
}

xiiResult xiiParticleSystemRuntime::AddSimulationPasses(xiiRenderGraph& ref_graph, xiiStringView sNamePrefix, const xiiParticleSystemDescriptor& descriptor)
{
  XII_SUCCEED_OR_RETURN(EnsureCapacity(descriptor));

  m_sGraphResourcePrefix = sNamePrefix;

  xiiStringBuilder sPassName;
  AddName(sPassName, sNamePrefix, "ParticleGraphSimulate");

  const xiiBitflags<xiiGALCommandQueueFlags> queueFlags = descriptor.m_Flags.IsSet(xiiParticleSystemFlags::AsyncCompute) ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics;
  ref_graph.AddPass<xiiParticleSimulationPassData>(sPassName, queueFlags, xiiMakeDelegate(&xiiParticleSystemRuntime::SetupSimulationPass, this), xiiMakeDelegate(&xiiParticleSystemRuntime::ExecuteSimulationPass, this), true);

  return XII_SUCCESS;
}

void xiiParticleSystemRuntime::SetupSimulationPass(xiiParticleSimulationPassData& ref_data, xiiRenderGraphBuilder& ref_builder)
{
  xiiStringBuilder sResourceName;

  ref_data.m_hGraph             = m_Descriptor.m_hGraph;
  ref_data.m_uiParticleCapacity = m_uiParticleCapacity;
  ref_data.m_uiDispatchGroups   = (m_uiParticleCapacity + xiiParticleSystemConstants::s_uiDefaultThreadGroupSize - 1U) / xiiParticleSystemConstants::s_uiDefaultThreadGroupSize;

  AddName(sResourceName, m_sGraphResourcePrefix, "ParticleStateRead");
  ref_data.m_hParticleStateRead = ref_builder.ImportBuffer(sResourceName, GetCurrentParticleStateBuffer(), xiiGALResourceStateFlags::ShaderResource);
  ref_data.m_hParticleStateRead = ref_builder.ReadBuffer(ref_data.m_hParticleStateRead, xiiGALResourceStateFlags::ShaderResource);

  AddName(sResourceName, m_sGraphResourcePrefix, "ParticleStateWrite");
  ref_data.m_hParticleStateWrite = ref_builder.ImportBuffer(sResourceName, GetNextParticleStateBuffer(), xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hParticleStateWrite = ref_builder.WriteBuffer(ref_data.m_hParticleStateWrite, xiiGALResourceStateFlags::UnorderedAccess);

  AddName(sResourceName, m_sGraphResourcePrefix, "AliveIndexBuffer");
  ref_data.m_hAliveIndexBuffer = ref_builder.ImportBuffer(sResourceName, m_pAliveIndexBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hAliveIndexBuffer = ref_builder.WriteBuffer(ref_data.m_hAliveIndexBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  AddName(sResourceName, m_sGraphResourcePrefix, "DeadIndexBuffer");
  ref_data.m_hDeadIndexBuffer = ref_builder.ImportBuffer(sResourceName, m_pDeadIndexBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hDeadIndexBuffer = ref_builder.WriteBuffer(ref_data.m_hDeadIndexBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  AddName(sResourceName, m_sGraphResourcePrefix, "Counters");
  ref_data.m_hCountersBuffer = ref_builder.ImportBuffer(sResourceName, m_pCountersBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hCountersBuffer = ref_builder.WriteBuffer(ref_data.m_hCountersBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  AddName(sResourceName, m_sGraphResourcePrefix, "Events");
  ref_data.m_hEventBuffer = ref_builder.ImportBuffer(sResourceName, m_pEventBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hEventBuffer = ref_builder.WriteBuffer(ref_data.m_hEventBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  AddName(sResourceName, m_sGraphResourcePrefix, "SortKeys");
  ref_data.m_hSortKeyBuffer = ref_builder.ImportBuffer(sResourceName, m_pSortKeyBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hSortKeyBuffer = ref_builder.WriteBuffer(ref_data.m_hSortKeyBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  AddName(sResourceName, m_sGraphResourcePrefix, "NeighborPairs");
  ref_data.m_hNeighborPairBuffer = ref_builder.ImportBuffer(sResourceName, m_pNeighborPairBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hNeighborPairBuffer = ref_builder.WriteBuffer(ref_data.m_hNeighborPairBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  AddName(sResourceName, m_sGraphResourcePrefix, "DrawIndirect");
  ref_data.m_hDrawIndirectBuffer = ref_builder.ImportBuffer(sResourceName, m_pDrawIndirectBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hDrawIndirectBuffer = ref_builder.WriteBuffer(ref_data.m_hDrawIndirectBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  AddName(sResourceName, m_sGraphResourcePrefix, "DispatchIndirect");
  ref_data.m_hDispatchIndirectBuffer = ref_builder.ImportBuffer(sResourceName, m_pDispatchIndirectBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  ref_data.m_hDispatchIndirectBuffer = ref_builder.WriteBuffer(ref_data.m_hDispatchIndirectBuffer, xiiGALResourceStateFlags::UnorderedAccess);

  ref_builder.SetPassSideEffects(true);
  ref_builder.SetPassAllowMerge(false);
}

void xiiParticleSystemRuntime::ExecuteSimulationPass(const xiiParticleSimulationPassData& data, xiiRenderGraphPassContext& ref_context)
{
  xiiGALCommandList& cmd = ref_context.GetCommandList();
  cmd.BeginDebugGroup("ParticleGraphSimulate");

  if (data.m_hGraph.IsValid())
  {
    xiiResourceLock<xiiParticleGraphResource> pGraph(data.m_hGraph, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pGraph)
    {
      for (const xiiParticleGraphNodeDesc& node : pGraph->GetDescriptor().m_Nodes)
      {
        if (node.m_Flags.IsSet(xiiParticleGraphNodeFlags::ToolOnly))
          continue;

        if (m_NodeExecutor.IsValid())
        {
          m_NodeExecutor(node, data, ref_context);
        }
        else
        {
          xiiStringBuilder sLabel;
          if (node.m_sDisplayName.IsEmpty())
          {
            sLabel.SetFormat("ParticleNode: {0}", node.m_sType.GetString());
          }
          else
          {
            sLabel.SetFormat("ParticleNode: {0}", node.m_sDisplayName);
          }
          cmd.InsertDebugLabel(sLabel, node.m_DebugColor);
        }
      }
    }
  }
  else
  {
    cmd.InsertDebugLabel("Particle graph has no resource assigned.");
  }

  cmd.EndDebugGroup();
  SwapParticleStateBuffers();
}

xiiSharedPtr<xiiGALBuffer> xiiParticleSystemRuntime::CreateStructuredBuffer(xiiStringView sDebugName, xiiUInt64 uiElementCount, xiiUInt32 uiElementStride, xiiBitflags<xiiGALBindFlags> bindFlags) const
{
  xiiGALBufferCreationDescription description;
  description.m_uiSize              = SafeStructuredSize(uiElementCount, uiElementStride);
  description.m_uiElementByteStride = uiElementStride;
  description.m_BindFlags           = bindFlags;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Mutable;

  xiiSharedPtr<xiiGALBuffer> pBuffer = m_pDevice->CreateBuffer(description);
  if (pBuffer != nullptr)
  {
    pBuffer->SetDebugName(sDebugName);
  }

  return pBuffer;
}

xiiSharedPtr<xiiGALBuffer> xiiParticleSystemRuntime::CreateRawBuffer(xiiStringView sDebugName, xiiUInt64 uiByteSize, xiiBitflags<xiiGALBindFlags> bindFlags) const
{
  xiiGALBufferCreationDescription description;
  description.m_uiSize              = xiiMath::Max<xiiUInt64>(4ULL, uiByteSize);
  description.m_uiElementByteStride = 4U;
  description.m_BindFlags           = bindFlags;
  description.m_Mode                = xiiGALBufferMode::Raw;
  description.m_Usage               = xiiGALResourceUsage::Mutable;

  xiiSharedPtr<xiiGALBuffer> pBuffer = m_pDevice->CreateBuffer(description);
  if (pBuffer != nullptr)
  {
    pBuffer->SetDebugName(sDebugName);
  }

  return pBuffer;
}


struct xiiParticleSystemComponent::EmissionState
{
  struct Birth
  {
    xiiParticleGPUState m_Initial;
    xiiTime             m_Born;
    xiiComponentHandle  m_Emitter;
    xiiHashedString     m_Binding;
  };
  struct Emitter
  {
    xiiComponentHandle m_Owner;
    xiiUInt64          m_uiNextBirth    = 0;
    bool               m_bBurstEmitted  = false;
    double             m_fNextBirthTime = 0;
    float              m_fPreviousRate  = -1;
    xiiTime            m_ActivatedAt;
  };
  xiiDynamicArray<Birth, xiiAlignedAllocatorWrapper> m_Births;
  xiiDynamicArray<Emitter>                           m_Emitters;
  xiiTransform                                       m_PreviousTransform = xiiTransform::MakeIdentity();
  xiiTime                                            m_Time;
  xiiUInt64                                          m_uiNextId  = 1;
  xiiUInt32                                          m_uiDropped = 0;
  xiiString                                          m_Error;
};

void xiiParticleSystemComponent::Update()
{
  if (!m_pEmissionState)
  {
    m_pEmissionState                      = XII_DEFAULT_NEW(EmissionState);
    m_pEmissionState->m_PreviousTransform = GetOwner()->GetGlobalTransform();
  }
  auto&      state        = *m_pEmissionState;
  const auto previousTime = state.m_Time;
  const auto transform    = GetOwner()->GetGlobalTransform();
  if (GetWorld()->GetWorldSimulationEnabled())
    state.m_Time += GetWorld()->GetClock().GetTimeDiff();
  state.m_uiDropped = 0;
  state.m_Error.Clear();
  for (xiiUInt32 i = state.m_Births.GetCount(); i > 0; --i)
    if (state.m_Time - state.m_Births[i - 1].m_Born >= xiiTime::MakeFromSeconds(state.m_Births[i - 1].m_Initial.m_vVelocityLifetime.w))
      state.m_Births.RemoveAtAndSwap(i - 1);
  for (xiiUInt32 i = state.m_Emitters.GetCount(); i > 0; --i)
  {
    const xiiComponent* emitter = nullptr;
    if (!GetWorld()->TryGetComponent(state.m_Emitters[i - 1].m_Owner, emitter))
      state.m_Emitters.RemoveAtAndSwap(i - 1);
  }
  if (m_Descriptor.m_hGraph.IsValid())
  {
    state.m_Error = "Ordinary particle emission cannot execute a custom graph; select a supported graph executor explicitly.";
    return;
  }
  for (const auto* component : GetOwner()->GetComponents())
  {
    const auto* emitter = xiiDynamicCast<const xiiParticleEmitterComponent*>(component);
    if (!emitter || !emitter->IsActiveAndInitialized())
      continue;
    const auto settings = emitter->GetRenderSnapshot();
    if ((settings.m_Shape != xiiParticleEmitterShape::Point && settings.m_Shape != xiiParticleEmitterShape::Sphere && settings.m_Shape != xiiParticleEmitterShape::Box) || (settings.m_Mode != xiiParticleEmitterMode::Continuous && settings.m_Mode != xiiParticleEmitterMode::Burst) || settings.m_bInheritOwnerVelocity || (settings.m_Shape == xiiParticleEmitterShape::Box && settings.m_bSurfaceOnly))
    {
      state.m_Error = "Ordinary particle emission supports point, sphere and volume-box burst/continuous initial conditions; the requested emitter algorithm requires a separate implementation.";
      continue;
    }
    if (!xiiMath::IsFinite(settings.m_fSpawnRate) || settings.m_fSpawnRate < 0 || !xiiMath::IsFinite(settings.m_fLifetime) || settings.m_fLifetime <= 0 || !xiiMath::IsFinite(settings.m_fInitialSpeed) || settings.m_fInitialSpeed < 0 || !xiiMath::IsFinite(settings.m_fRadius) || settings.m_fRadius < 0 || !settings.m_BoxExtents.IsValid() || settings.m_BoxExtents.x < 0 || settings.m_BoxExtents.y < 0 || settings.m_BoxExtents.z < 0 || (m_Descriptor.m_SimulationSpace != xiiParticleSimulationSpace::World && m_Descriptor.m_SimulationSpace != xiiParticleSimulationSpace::Local))
    {
      state.m_Error = "Particle birth parameters require finite positive lifetime, nonnegative speed/rate/extents and world or local simulation space.";
      continue;
    }
    EmissionState::Emitter* source = nullptr;
    for (auto& item : state.m_Emitters)
      if (item.m_Owner == emitter->GetHandle())
      {
        source = &item;
        break;
      }
    if (!source)
    {
      if (state.m_Emitters.GetCount() >= m_Descriptor.m_uiMaxEmitters)
      {
        state.m_Error = "Particle emitter count exceeds the authored system capacity.";
        continue;
      }
      source                   = &state.m_Emitters.ExpandAndGetRef();
      source->m_Owner          = emitter->GetHandle();
      source->m_ActivatedAt    = previousTime;
      source->m_fNextBirthTime = previousTime.GetSeconds();
      source->m_fPreviousRate  = settings.m_fSpawnRate;
    }
    if (source->m_fPreviousRate != settings.m_fSpawnRate)
    {
      // A rate edit starts a new interval; never invent births in an earlier world frame.
      source->m_fNextBirthTime = previousTime.GetSeconds() + (settings.m_fSpawnRate > 0 ? 1.0 / settings.m_fSpawnRate : 0.0);
      source->m_fPreviousRate  = settings.m_fSpawnRate;
    }
    xiiUInt64 end = source->m_uiNextBirth;
    if (settings.m_Mode == xiiParticleEmitterMode::Burst)
    {
      if (!source->m_bBurstEmitted)
        end += settings.m_uiBurstCount;
      source->m_bBurstEmitted = true;
    }
    else if (settings.m_fSpawnRate > 0 && source->m_fNextBirthTime <= state.m_Time.GetSeconds())
    {
      const double next = (state.m_Time.GetSeconds() - source->m_fNextBirthTime) * settings.m_fSpawnRate;
      if (!xiiMath::IsFinite(next) || next > double(xiiMath::MaxValue<xiiUInt32>()))
      {
        state.m_Error = "Particle emission interval exceeds bounded birth capacity; reset the system.";
        continue;
      }
      const double oldest = (state.m_Time.GetSeconds() - settings.m_fLifetime - source->m_fNextBirthTime) * settings.m_fSpawnRate;
      if (oldest > 0)
      {
        const auto skipped = xiiUInt64(xiiMath::Floor(oldest));
        source->m_fNextBirthTime += double(skipped) / settings.m_fSpawnRate;
        source->m_uiNextBirth += skipped;
      }
      end = source->m_uiNextBirth + xiiUInt64(xiiMath::Floor((state.m_Time.GetSeconds() - source->m_fNextBirthTime) * settings.m_fSpawnRate)) + 1;
    }
    xiiUInt32 live = 0;
    for (const auto& birth : state.m_Births)
      live += birth.m_Emitter == source->m_Owner;
    const xiiUInt32 capacity  = settings.m_uiMaxParticles ? xiiMath::Min(settings.m_uiMaxParticles, m_Descriptor.m_uiMaxParticles) : m_Descriptor.m_uiMaxParticles;
    const auto      requested = end > source->m_uiNextBirth ? end - source->m_uiNextBirth : 0;
    const xiiUInt32 available = xiiMath::Min(capacity - xiiMath::Min(capacity, live), m_Descriptor.m_uiMaxParticles - xiiMath::Min(m_Descriptor.m_uiMaxParticles, state.m_Births.GetCount()));
    const auto      emitted   = xiiMath::Min<xiiUInt64>(requested, available);
    state.m_uiDropped += static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(requested - emitted, xiiMath::MaxValue<xiiUInt32>() - state.m_uiDropped));
    for (xiiUInt64 i = 0; i < emitted; ++i)
    {
      if (state.m_uiNextId > xiiMath::MaxValue<xiiUInt32>())
      {
        state.m_Error = "Particle identities are exhausted; reset the system.";
        break;
      }
      const auto ordinal = source->m_uiNextBirth + i;
      const auto born    = settings.m_Mode == xiiParticleEmitterMode::Burst ? source->m_ActivatedAt : xiiTime::MakeFromSeconds(source->m_fNextBirthTime + double(i) / settings.m_fSpawnRate);
      if (state.m_Time - born >= xiiTime::MakeFromSeconds(settings.m_fLifetime))
        continue;
      xiiRandom random;
      const auto seed = (xiiUInt64(m_Descriptor.m_uiRandomSeed ^ settings.m_uiRandomSeed) << 32) ^ (ordinal + 1);
      random.Initialize(seed ? seed : 1);
      xiiVec3 position = xiiVec3::MakeZero();
      if (settings.m_Shape == xiiParticleEmitterShape::Sphere)
        position = (settings.m_bSurfaceOnly ? xiiVec3::MakeRandomDirection(random) : xiiVec3::MakeRandomPointInSphere(random)) * settings.m_fRadius;
      else if (settings.m_Shape == xiiParticleEmitterShape::Box)
        position = xiiVec3(random.FloatMinMax(-1, 1), random.FloatMinMax(-1, 1), random.FloatMinMax(-1, 1)).CompMul(settings.m_BoxExtents);
      xiiVec3 velocity(0, 0, settings.m_fInitialSpeed);
      float   scale = 1;
      if (m_Descriptor.m_SimulationSpace == xiiParticleSimulationSpace::World)
      {
        const auto   duration = state.m_Time - previousTime;
        const float  alpha    = duration.IsPositive() ? xiiMath::Clamp(float((born - previousTime).GetSeconds() / duration.GetSeconds()), 0.0f, 1.0f) : 1.0f;
        xiiTransform birthTransform;
        birthTransform.m_vPosition = xiiMath::Lerp(state.m_PreviousTransform.m_vPosition, transform.m_vPosition, alpha);
        birthTransform.m_vScale    = xiiMath::Lerp(state.m_PreviousTransform.m_vScale, transform.m_vScale, alpha);
        birthTransform.m_qRotation = xiiQuat::MakeSlerp(state.m_PreviousTransform.m_qRotation, transform.m_qRotation, alpha);
        position                   = birthTransform.TransformPosition(position);
        velocity                   = birthTransform.m_qRotation * velocity;
        scale                      = birthTransform.GetMaxScale();
      }
      auto& birth = state.m_Births.ExpandAndGetRef();
      xiiMemoryUtils::ZeroFill(&birth.m_Initial, 1);
      birth.m_Initial.m_vPositionAge            = xiiVec4(position.x, position.y, position.z, 0);
      birth.m_Initial.m_vPreviousPositionRadius = xiiVec4(position.x, position.y, position.z, scale);
      birth.m_Initial.m_vVelocityLifetime       = xiiVec4(velocity.x, velocity.y, velocity.z, settings.m_fLifetime);
      birth.m_Initial.m_vColor                  = xiiVec4(1);
      birth.m_Initial.m_vForceInverseMass.w     = 1;
      birth.m_Initial.m_uiId                    = static_cast<xiiUInt32>(state.m_uiNextId++);
      birth.m_Born                              = born;
      birth.m_Emitter                           = source->m_Owner;
      birth.m_Binding                           = settings.m_SystemBinding;
    }
    if (settings.m_Mode == xiiParticleEmitterMode::Continuous && settings.m_fSpawnRate > 0)
      source->m_fNextBirthTime += double(requested) / settings.m_fSpawnRate;
    source->m_uiNextBirth = end;
  }
  state.m_PreviousTransform = transform;
}

xiiParticleSystemComponent::xiiParticleSystemComponent() = default;

xiiParticleSystemComponent::~xiiParticleSystemComponent()
{
  xiiParticleSystemManager::DestroyRuntime(m_hRuntime);
  m_hRuntime = {};
}

void xiiParticleSystemComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  m_Descriptor.Save(inout_stream.GetStream());
}

void xiiParticleSystemComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  m_Descriptor.Load(inout_stream.GetStream());
}

xiiResult xiiParticleSystemComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);

  ref_bAlwaysVisible = m_Descriptor.m_bAlwaysVisible;
  ref_bounds         = m_Descriptor.m_LocalBounds;

  return ref_bAlwaysVisible || ref_bounds.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

void xiiParticleSystemComponent::SetDescriptor(const xiiParticleSystemDescriptor& descriptor)
{
  m_pEmissionState.Clear();
  m_Descriptor                  = descriptor;
  m_ActivationTime              = GetOwningManager() ? GetWorld()->GetClock().GetAccumulatedTime() : xiiTime::MakeZero();
  ++m_uiSimulationRevision;
  m_Descriptor.m_uiMaxParticles = ClampParticleCapacity(m_Descriptor.m_uiMaxParticles);

  xiiParticleSystemManager::DestroyRuntime(m_hRuntime);
  m_hRuntime = {};
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

const xiiParticleSystemDescriptor& xiiParticleSystemComponent::GetDescriptor() const
{
  return m_Descriptor;
}

void xiiParticleSystemComponent::SetParticleGraph(const xiiParticleGraphResourceHandle& hGraph)
{
  if (m_Descriptor.m_hGraph == hGraph)
    return;

  m_pEmissionState.Clear();
  m_Descriptor.m_hGraph = hGraph;
  m_ActivationTime      = GetOwningManager() ? GetWorld()->GetClock().GetAccumulatedTime() : xiiTime::MakeZero();
  ++m_uiSimulationRevision;
  InvalidateCachedRenderData();
}

const xiiParticleGraphResourceHandle& xiiParticleSystemComponent::GetParticleGraph() const
{
  return m_Descriptor.m_hGraph;
}

xiiResult xiiParticleSystemComponent::PrepareRuntimeResources(xiiSharedPtr<xiiGALDevice> pDevice) const
{
  if (pDevice == nullptr)
    return XII_FAILURE;

  if (xiiParticleSystemRuntime* pRuntime = xiiParticleSystemManager::GetRuntime(m_hRuntime))
    return pRuntime->EnsureCapacity(m_Descriptor);

  m_hRuntime = xiiParticleSystemManager::CreateRuntime(std::move(pDevice), m_Descriptor);
  return m_hRuntime.IsInvalidated() ? XII_FAILURE : XII_SUCCESS;
}

xiiParticleSystemRuntime& xiiParticleSystemComponent::GetRuntime() const
{
  xiiParticleSystemRuntime* pRuntime = xiiParticleSystemManager::GetRuntime(m_hRuntime);
  XII_ASSERT_DEV(pRuntime != nullptr, "Particle runtime has not been prepared or was destroyed during subsystem shutdown.");
  return *pRuntime;
}

void xiiParticleSystemComponent::OnActivated()
{
  SUPER::OnActivated();
  m_pEmissionState.Clear();
  m_ActivationTime = GetWorld()->GetClock().GetAccumulatedTime();
  ++m_uiSimulationRevision;
}

void xiiParticleSystemComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  const xiiRenderWorldModule* pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiParticleRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiParticleRenderData>(this);
  pRenderData->m_Descriptor          = m_Descriptor;
  pRenderData->m_uiUniqueID          = GetUniqueIdForRendering();
  pRenderData->m_hRuntime            = m_hRuntime;

  pRenderData->m_ElapsedTime          = GetWorld()->GetClock().GetAccumulatedTime() - m_ActivationTime;
  pRenderData->m_uiSimulationRevision = m_uiSimulationRevision;
  pRenderData->m_uiWorldUpdate        = GetWorld()->GetUpdateCounter();
  for (const auto* component : GetOwner()->GetComponents())
  {
    if (!component->IsActiveAndInitialized())
      continue;
    if (const auto* emitter = xiiDynamicCast<const xiiParticleEmitterComponent*>(component))
      pRenderData->m_Emitters.PushBack(emitter->GetRenderSnapshot());
    else if (const auto* renderer = xiiDynamicCast<const xiiParticleRendererComponent*>(component))
      pRenderData->m_Renderers.PushBack(renderer->GetRenderSnapshot());
    else if (xiiDynamicCast<const xiiParticleForceFieldComponent*>(component) || xiiDynamicCast<const xiiParticleColliderComponent*>(component) || xiiDynamicCast<const xiiParticleSimulationDomainComponent*>(component) || xiiDynamicCast<const xiiParticleMolecularDynamicsComponent*>(component))
      pRenderData->m_bHasAdditionalSimulation = true;
  }

  if (m_pEmissionState)
  {
    pRenderData->m_ElapsedTime         = m_pEmissionState->m_Time;
    pRenderData->m_sEmissionError      = m_pEmissionState->m_Error;
    pRenderData->m_uiDroppedSpawnCount = m_pEmissionState->m_uiDropped;
    for (const auto& initial : m_pEmissionState->m_Births)
    {
      auto& birth                           = pRenderData->m_Births.ExpandAndGetRef();
      birth.m_InitialState                  = initial.m_Initial;
      birth.m_InitialState.m_vPositionAge.w = static_cast<float>((m_pEmissionState->m_Time - initial.m_Born).GetSeconds());
      birth.m_SystemBinding                 = initial.m_Binding;
    }
  }

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds    = m_Descriptor.m_LocalBounds;
  pRenderData->m_GlobalBounds.Transform(pRenderData->m_GlobalTransform.GetAsMat4());

  const xiiParticleSystemRuntime* pRuntime = xiiParticleSystemManager::GetRuntime(m_hRuntime);
  if (pRuntime != nullptr && pRuntime->IsInitialized())
  {
    pRenderData->m_pParticleStateBuffer = pRuntime->GetCurrentParticleStateBuffer();
    pRenderData->m_pAliveIndexBuffer    = pRuntime->GetAliveIndexBuffer();
    pRenderData->m_pCountersBuffer      = pRuntime->GetCountersBuffer();
    pRenderData->m_pDrawIndirectBuffer  = pRuntime->GetDrawIndirectBuffer();
  }

  pRenderData->m_uiSortingKey = (static_cast<xiiUInt64>(m_Descriptor.m_uiMaxParticles) << 32U) ^ pRenderData->m_uiUniqueID;

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Particles_Implementation_ParticleSystem);
