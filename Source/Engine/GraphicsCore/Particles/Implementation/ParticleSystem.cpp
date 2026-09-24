/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
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
    new xiiCategoryAttribute("RenderWorld/Particles"),
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

xiiParticleSystemRuntime::xiiParticleSystemRuntime()  = default;
xiiParticleSystemRuntime::~xiiParticleSystemRuntime() = default;

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

xiiParticleSystemComponent::xiiParticleSystemComponent()  = default;
xiiParticleSystemComponent::~xiiParticleSystemComponent() = default;

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
  m_Descriptor                  = descriptor;
  m_Descriptor.m_uiMaxParticles = ClampParticleCapacity(m_Descriptor.m_uiMaxParticles);

  m_Runtime.Shutdown();
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

  m_Descriptor.m_hGraph = hGraph;
  InvalidateCachedRenderData();
}

const xiiParticleGraphResourceHandle& xiiParticleSystemComponent::GetParticleGraph() const
{
  return m_Descriptor.m_hGraph;
}

xiiResult xiiParticleSystemComponent::PrepareRuntimeResources(xiiSharedPtr<xiiGALDevice> pDevice) const
{
  return m_Runtime.Initialize(std::move(pDevice), m_Descriptor);
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
  pRenderData->m_pRuntime            = &m_Runtime;

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds    = m_Descriptor.m_LocalBounds;
  pRenderData->m_GlobalBounds.Transform(pRenderData->m_GlobalTransform.GetAsMat4());

  if (m_Runtime.IsInitialized())
  {
    pRenderData->m_pParticleStateBuffer = m_Runtime.GetCurrentParticleStateBuffer();
    pRenderData->m_pAliveIndexBuffer    = m_Runtime.GetAliveIndexBuffer();
    pRenderData->m_pCountersBuffer      = m_Runtime.GetCountersBuffer();
    pRenderData->m_pDrawIndirectBuffer  = m_Runtime.GetDrawIndirectBuffer();
  }

  pRenderData->m_uiSortingKey = (static_cast<xiiUInt64>(m_Descriptor.m_uiMaxParticles) << 32U) ^ pRenderData->m_uiUniqueID;

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Particles_Implementation_ParticleSystem);
