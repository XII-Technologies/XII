/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Lighting/EmissiveLightManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightBuildConstants.h>

namespace
{
  struct EmissiveLightBuildPassData
  {
    xiiRenderGraphBufferHandle              m_hSceneInstances;
    xiiRenderGraphBufferHandle              m_hGeometry;
    xiiRenderGraphBufferHandle              m_hMeshlets;
    xiiRenderGraphBufferHandle              m_hSurfaceMaterials;
    xiiRenderGraphBufferHandle              m_hTriangles;
    xiiRenderGraphBufferHandle              m_hHeader;
    xiiRenderGraphBufferHandle              m_hConstants;
    xiiEmissiveLightManager::BuildDescription m_Description;
    xiiUInt64                               m_uiFrameIndex       = 0U;
    xiiUInt32                               m_uiTriangleCapacity = 0U;
    xiiUInt32                               m_uiDispatchY        = 0U;
    xiiUInt32                               m_uiDispatchZ        = 0U;
    bool                                    m_bExtract           = false;
  };

  bool IsConfigurationValid(const xiiEmissiveLightSettings& settings)
  {
    return settings.m_uiMaxTriangles > 0U && settings.m_uiMaxTriangles <= 16U * 1024U * 1024U;
  }

  xiiSharedPtr<xiiGALComputePipelineState> LoadBuildPipeline()
  {
    const xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/EmissiveTriangleBuild.xiiShader");
    xiiHashTable<xiiHashedString, xiiHashedString> variables(xiiTemporaryAllocator::Get());
    const xiiShaderPermutationResourceHandle hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, variables, true);
    xiiResourceLock<xiiShaderPermutationResource> permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    if (!permutation.IsValid())
      return nullptr;

    xiiGALComputePipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_pComputeShader             = permutation->GetGALShader(xiiGALShaderType::Compute);
    pipelineDescription.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
    return xiiGALPipelineCache::GetPipeline(pipelineDescription);
  }
} // namespace

class xiiEmissiveLightManager::State
{
public:
  xiiEmissiveLightSettings                   m_Settings;
  xiiEmissiveLightStats                      m_Stats;
  xiiSharedPtr<xiiGALComputePipelineState>   m_pBuildPipeline;
  bool                                       m_bEngineStarted = false;
};

xiiUniquePtr<xiiEmissiveLightManager::State> xiiEmissiveLightManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, EmissiveLightManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "BindlessResourceTable",
    "GeometryResidencyManager",
    "MaterialManager",
    "PipelineCache",
    "ShaderPermutationUtilities"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiEmissiveLightManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiEmissiveLightManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiEmissiveLightManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiEmissiveLightManager::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuEmissiveTriangle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuEmissiveTriangle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Position0AndArea", Position0AndArea),
    XII_MEMBER_PROPERTY("Position1AndPowerWeight", Position1AndPowerWeight),
    XII_MEMBER_PROPERTY("Position2AndInvArea", Position2AndInvArea),
    XII_MEMBER_PROPERTY("NormalAndTwoSided", NormalAndTwoSided),
    XII_MEMBER_PROPERTY("RadianceAndLuminance", RadianceAndLuminance),
    XII_MEMBER_PROPERTY("Metadata", Metadata),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuEmissiveLightHeader, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuEmissiveLightHeader>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TriangleCount", TriangleCount),
    XII_MEMBER_PROPERTY("OverflowCount", OverflowCount),
    XII_MEMBER_PROPERTY("CandidateCount", CandidateCount),
    XII_MEMBER_PROPERTY("FrameIndex", FrameIndex),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiEmissiveLightSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiEmissiveLightSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxTriangles", m_uiMaxTriangles)->AddAttributes(new xiiDefaultValueAttribute(262144U), new xiiClampValueAttribute(1U, 16777216U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiEmissiveLightStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiEmissiveLightStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TriangleCapacity", m_uiTriangleCapacity),
    XII_MEMBER_PROPERTY("ScheduledInstanceCount", m_uiScheduledInstanceCount),
    XII_MEMBER_PROPERTY("ScheduledMeshletsPerInstance", m_uiScheduledMeshletsPerInstance),
    XII_MEMBER_PROPERTY("LastScheduledFrame", m_uiLastScheduledFrame),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiEmissiveLightManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Emissive light manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
  s_pState->m_Settings = {};
  s_pState->m_Stats.m_uiTriangleCapacity = s_pState->m_Settings.m_uiMaxTriangles;
}

void xiiEmissiveLightManager::EngineStartup()
{
  if (s_pState != nullptr)
    s_pState->m_bEngineStarted = true;
}

void xiiEmissiveLightManager::EngineShutdown()
{
  if (s_pState != nullptr)
  {
    s_pState->m_pBuildPipeline.Clear();
    s_pState->m_bEngineStarted = false;
  }
}

void xiiEmissiveLightManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiEmissiveLightManager::Configure(const xiiEmissiveLightSettings& settings)
{
  if (s_pState == nullptr || !IsConfigurationValid(settings))
    return XII_FAILURE;

  s_pState->m_Settings = settings;
  s_pState->m_Stats.m_uiTriangleCapacity = settings.m_uiMaxTriangles;
  return XII_SUCCESS;
}

const xiiEmissiveLightSettings& xiiEmissiveLightManager::GetConfiguration()
{
  XII_ASSERT_ALWAYS(s_pState != nullptr, "Emissive light manager subsystem is not started.");
  return s_pState->m_Settings;
}

xiiEmissiveLightStats xiiEmissiveLightManager::GetStats()
{
  return s_pState != nullptr ? s_pState->m_Stats : xiiEmissiveLightStats{};
}

bool xiiEmissiveLightManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiEmissiveLightManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted && xiiGALDevice::HasDefaultDevice() && xiiGALBindlessResourceTable::IsInitialized();
}

xiiEmissiveLightManager::BuildOutputs xiiEmissiveLightManager::AddBuildPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const BuildDescription& description)
{
  BuildOutputs outputs;
  if (!IsInitialized())
    return outputs;

  const bool bExtract = description.m_Visibility.m_hSceneInstances.IsValid() &&
    description.m_Geometry.m_hGeometryMetadata.IsValid() && description.m_Geometry.m_hMeshletMetadata.IsValid() &&
    description.m_Materials.m_hSurfaceData.IsValid() && description.m_uiInstanceCount > 0U &&
    description.m_Geometry.m_uiMaximumResidentMeshletCount > 0U && description.m_uiVertexStride >= 12U;

  if (bExtract && s_pState->m_pBuildPipeline == nullptr)
    s_pState->m_pBuildPipeline = LoadBuildPipeline();
  if (bExtract && s_pState->m_pBuildPipeline == nullptr)
    return outputs;

  const xiiUInt32 uiMaxGroupCountY = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties.m_uiMaxThreadGroupCountY;
  const xiiUInt32 uiDispatchY      = bExtract ? xiiMath::Min(description.m_uiInstanceCount, uiMaxGroupCountY) : 0U;
  const xiiUInt32 uiDispatchZ      = bExtract ? (description.m_uiInstanceCount + uiDispatchY - 1U) / uiDispatchY : 0U;
  const xiiUInt32 uiMaxGroupCountZ = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties.m_uiMaxThreadGroupCountZ;
  if (uiDispatchZ > uiMaxGroupCountZ)
  {
    xiiLog::Error("Emissive triangle extraction requires {} dispatch layers, exceeding the device limit of {}.", uiDispatchZ, uiMaxGroupCountZ);
    return outputs;
  }

  const xiiUInt32 uiTriangleCapacity = s_pState->m_Settings.m_uiMaxTriangles;
  auto buildPass = graph.AddPass<EmissiveLightBuildPassData>(
    "GPU Emissive Triangle Build", xiiGALCommandQueueFlags::Compute,
    [&](EmissiveLightBuildPassData& data, xiiRenderGraphBuilder& builder) {
      if (bExtract)
      {
        data.m_hSceneInstances   = builder.ReadBuffer(description.m_Visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
        data.m_hGeometry         = builder.ReadBuffer(description.m_Geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
        data.m_hMeshlets         = builder.ReadBuffer(description.m_Geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
        data.m_hSurfaceMaterials = builder.ReadBuffer(description.m_Materials.m_hSurfaceData, xiiGALResourceStateFlags::ShaderResource);
      }

      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiSize              = static_cast<xiiUInt64>(uiTriangleCapacity) * sizeof(xiiGpuEmissiveTriangle);
      bufferDescription.m_uiElementByteStride = sizeof(xiiGpuEmissiveTriangle);
      bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Default;
      data.m_hTriangles                       = builder.WriteBuffer("GPU Emissive Triangles", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

      bufferDescription.m_uiSize              = sizeof(xiiGpuEmissiveLightHeader);
      bufferDescription.m_uiElementByteStride = sizeof(xiiGpuEmissiveLightHeader);
      data.m_hHeader                          = builder.WriteBuffer("GPU Emissive Light Header", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

      bufferDescription                       = {};
      bufferDescription.m_uiSize              = sizeof(xiiEmissiveLightBuildConstants);
      bufferDescription.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
      bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                       = builder.WriteBuffer("xiiEmissiveLightBuildConstants", bufferDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false);
    },
    [](const EmissiveLightBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      xiiGpuEmissiveLightHeader header = {};
      header.FrameIndex = static_cast<xiiUInt32>(data.m_uiFrameIndex);
      cmd.UpdateBuffer(context.GetBuffer(data.m_hHeader), 0U,
                       xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&header), sizeof(header)));
      if (!data.m_bExtract)
        return;

      {
        xiiGALMapHelper<xiiEmissiveLightBuildConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        constants->GeometryBaseIndex = data.m_Description.m_Geometry.m_uiGeometryBaseIndex;
        constants->MaterialBaseIndex = data.m_Description.m_Materials.m_uiSurfaceBaseIndex;
        constants->InstanceCount     = data.m_Description.m_uiInstanceCount;
        constants->MaxTriangleCount  = data.m_uiTriangleCapacity;
        constants->VertexStride      = data.m_Description.m_uiVertexStride;
        constants->TexCoordOffset    = data.m_Description.m_uiTexCoordOffset;
        constants->MaxMeshletCount   = data.m_Description.m_Geometry.m_uiMaximumResidentMeshletCount;
        constants->FrameIndex        = static_cast<xiiUInt32>(data.m_uiFrameIndex);
      }

      cmd.SetPipelineState(xiiEmissiveLightManager::s_pState->m_pBuildPipeline);
      cmd.ResolveAndSetConstantBuffer("xiiEmissiveLightBuildConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hSceneInstances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Meshlets", context.GetBuffer(data.m_hMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials", context.GetBuffer(data.m_hSurfaceMaterials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hTriangles)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hHeader)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_Buffers", xiiGALShaderType::Compute);
      xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_Textures", xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

      cmd.DispatchCompute({data.m_Description.m_Geometry.m_uiMaximumResidentMeshletCount, data.m_uiDispatchY, data.m_uiDispatchZ});
    });
  buildPass.first->m_Description        = description;
  buildPass.first->m_uiFrameIndex       = uiFrameIndex;
  buildPass.first->m_uiTriangleCapacity = uiTriangleCapacity;
  buildPass.first->m_uiDispatchY        = uiDispatchY;
  buildPass.first->m_uiDispatchZ        = uiDispatchZ;
  buildPass.first->m_bExtract           = bExtract;

  outputs.m_hTriangles        = buildPass.first->m_hTriangles;
  outputs.m_hHeader           = buildPass.first->m_hHeader;
  outputs.m_uiTriangleCapacity = uiTriangleCapacity;

  s_pState->m_Stats.m_uiTriangleCapacity              = uiTriangleCapacity;
  s_pState->m_Stats.m_uiScheduledInstanceCount        = bExtract ? description.m_uiInstanceCount : 0U;
  s_pState->m_Stats.m_uiScheduledMeshletsPerInstance  = bExtract ? description.m_Geometry.m_uiMaximumResidentMeshletCount : 0U;
  s_pState->m_Stats.m_uiLastScheduledFrame            = uiFrameIndex;
  return outputs;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_EmissiveLightManager);
