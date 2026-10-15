/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Lighting/EmissiveLightManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveAliasBuildConstants.h>
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightBuildConstants.h>

namespace
{
  struct EmissiveLightBuildPassData
  {
    xiiRenderGraphBufferHandle                m_hSceneInstances;
    xiiRenderGraphBufferHandle                m_hSkinningMatrices;
    xiiRenderGraphBufferHandle                m_hGeometry;
    xiiRenderGraphBufferHandle                m_hMeshlets;
    xiiRenderGraphBufferHandle                m_hSurfaceMaterials;
    xiiRenderGraphBufferHandle                m_hTriangles;
    xiiRenderGraphBufferHandle                m_hHeader;
    xiiRenderGraphBufferHandle                m_hConstants;
    xiiEmissiveLightManager::BuildDescription m_Description;
    xiiUInt64                                 m_uiFrameIndex       = 0U;
    xiiUInt32                                 m_uiTriangleCapacity = 0U;
    xiiUInt32                                 m_uiDispatchY        = 0U;
    xiiUInt32                                 m_uiDispatchZ        = 0U;
    bool                                      m_bExtract           = false;
  };

  struct EmissiveAliasBlockBuildPassData
  {
    xiiRenderGraphBufferHandle m_hTriangles;
    xiiRenderGraphBufferHandle m_hHeader;
    xiiRenderGraphBufferHandle m_hTriangleAliases;
    xiiRenderGraphBufferHandle m_hBlockPowers;
    xiiRenderGraphBufferHandle m_hProbabilities;
    xiiRenderGraphBufferHandle m_hSmall;
    xiiRenderGraphBufferHandle m_hLarge;
    xiiRenderGraphBufferHandle m_hConstants;
    xiiUInt32                  m_uiTriangleCapacity = 0U;
    xiiUInt32                  m_uiBlockCapacity    = 0U;
    xiiUInt32                  m_uiDispatchWidth    = 0U;
    xiiUInt32                  m_uiDispatchHeight   = 0U;
    xiiUInt32                  m_uiFrameIndex       = 0U;
  };

  struct EmissiveAliasGlobalBuildPassData
  {
    xiiRenderGraphBufferHandle m_hHeader;
    xiiRenderGraphBufferHandle m_hBlockPowers;
    xiiRenderGraphBufferHandle m_hProbabilities;
    xiiRenderGraphBufferHandle m_hSmall;
    xiiRenderGraphBufferHandle m_hLarge;
    xiiRenderGraphBufferHandle m_hBlockAliases;
    xiiRenderGraphBufferHandle m_hAliasHeader;
    xiiRenderGraphBufferHandle m_hConstants;
    xiiUInt32                  m_uiTriangleCapacity = 0U;
    xiiUInt32                  m_uiBlockCapacity    = 0U;
    xiiUInt32                  m_uiDispatchWidth    = 0U;
    xiiUInt32                  m_uiFrameIndex       = 0U;
  };

  static constexpr xiiUInt32 s_uiTrianglesPerAliasBlock = 256U;

  bool IsConfigurationValid(const xiiEmissiveLightSettings& settings)
  {
    return settings.m_uiMaxTriangles > 0U && settings.m_uiMaxTriangles <= 16U * 1024U * 1024U;
  }

  xiiSharedPtr<xiiGALComputePipelineState> LoadComputePipeline(xiiStringView sShaderPath)
  {
    const xiiShaderResourceHandle                  hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);
    xiiHashTable<xiiHashedString, xiiHashedString> variables(xiiTemporaryAllocator::Get());
    const xiiShaderPermutationResourceHandle       hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, variables, false);
    if (!hPermutation.IsValid())
      return nullptr;
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
  xiiMutex                                 m_Mutex;
  xiiSharedPtr<xiiGALSampler>              m_pMaterialSampler;
  xiiEmissiveLightSettings                 m_Settings;
  xiiEmissiveLightStats                    m_Stats;
  xiiSharedPtr<xiiGALComputePipelineState> m_pBuildPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_pAliasBlockBuildPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_pAliasGlobalBuildPipeline;
  bool                                     m_bEngineStarted = false;
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

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuEmissiveAliasEntry, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuEmissiveAliasEntry>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Probability", Probability),
    XII_MEMBER_PROPERTY("AliasIndex", AliasIndex),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuEmissiveAliasHeader, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuEmissiveAliasHeader>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BlockCount", BlockCount),
    XII_MEMBER_PROPERTY("TrianglesPerBlock", TrianglesPerBlock),
    XII_MEMBER_PROPERTY("TotalPower", TotalPower),
    XII_MEMBER_PROPERTY("FrameIndex", FrameIndex),
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
  s_pState                               = XII_DEFAULT_NEW(State);
  s_pState->m_Settings                   = {};
  s_pState->m_Stats.m_uiTriangleCapacity = s_pState->m_Settings.m_uiMaxTriangles;
}

void xiiEmissiveLightManager::EngineStartup()
{
  if (s_pState != nullptr)
  {
    const auto device = xiiGALDevice::GetDefaultDevice();
    if (!device)
      return;
    xiiGALSamplerCreationDescription sampler;
    sampler.m_MinFilter          = xiiGALFilterType::Linear;
    sampler.m_MagFilter          = xiiGALFilterType::Linear;
    sampler.m_MipFilter          = xiiGALFilterType::Linear;
    sampler.m_AddressU           = xiiGALTextureAddressMode::Wrap;
    sampler.m_AddressV           = xiiGALTextureAddressMode::Wrap;
    sampler.m_AddressW           = xiiGALTextureAddressMode::Wrap;
    s_pState->m_pMaterialSampler = device->CreateSampler(sampler);
    s_pState->m_bEngineStarted   = s_pState->m_pMaterialSampler != nullptr;
  }
}

void xiiEmissiveLightManager::EngineShutdown()
{
  if (s_pState != nullptr)
  {
    s_pState->m_pBuildPipeline.Clear();
    s_pState->m_pAliasBlockBuildPipeline.Clear();
    s_pState->m_pAliasGlobalBuildPipeline.Clear();
    s_pState->m_pMaterialSampler.Clear();
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

  XII_LOCK(s_pState->m_Mutex);
  s_pState->m_Settings                   = settings;
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
  if (s_pState == nullptr)
    return {};
  XII_LOCK(s_pState->m_Mutex);
  return s_pState->m_Stats;
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

  XII_LOCK(s_pState->m_Mutex);

  const bool bExtract = description.m_Visibility.m_hSceneInstances.IsValid() && description.m_Visibility.m_hSkinningMatrices.IsValid() &&
    description.m_Geometry.m_hGeometryMetadata.IsValid() && description.m_Geometry.m_hMeshletMetadata.IsValid() &&
    description.m_Materials.m_hSurfaceData.IsValid() && description.m_uiInstanceCount > 0U &&
    description.m_Geometry.m_uiMaximumResidentMeshletCount > 0U && description.m_uiVertexStride >= 12U;

  if (bExtract && s_pState->m_pBuildPipeline == nullptr)
    s_pState->m_pBuildPipeline = LoadComputePipeline("Shaders/Pipeline/EmissiveTriangleBuild.xiiShader");
  if (s_pState->m_pAliasBlockBuildPipeline == nullptr)
    s_pState->m_pAliasBlockBuildPipeline = LoadComputePipeline("Shaders/Pipeline/EmissiveAliasBlockBuild.xiiShader");
  if (s_pState->m_pAliasGlobalBuildPipeline == nullptr)
    s_pState->m_pAliasGlobalBuildPipeline = LoadComputePipeline("Shaders/Pipeline/EmissiveAliasGlobalBuild.xiiShader");
  if ((bExtract && s_pState->m_pBuildPipeline == nullptr) || s_pState->m_pAliasBlockBuildPipeline == nullptr || s_pState->m_pAliasGlobalBuildPipeline == nullptr)
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

  const xiiUInt32 uiTriangleCapacity    = s_pState->m_Settings.m_uiMaxTriangles;
  auto            buildPass             = graph.AddPass<EmissiveLightBuildPassData>("GPU Emissive Triangle Build", xiiGALCommandQueueFlags::Compute, [&](EmissiveLightBuildPassData& data, xiiRenderGraphBuilder& builder) {
      if (bExtract)
      {
        data.m_hSceneInstances   = builder.ReadBuffer(description.m_Visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
        data.m_hSkinningMatrices = builder.ReadBuffer(description.m_Visibility.m_hSkinningMatrices, xiiGALResourceStateFlags::ShaderResource);
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

      bufferDescription                  = {};
      bufferDescription.m_uiSize         = sizeof(xiiEmissiveLightBuildConstants);
      bufferDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      bufferDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      bufferDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                  = builder.WriteBuffer("xiiEmissiveLightBuildConstants", bufferDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false); }, [](const EmissiveLightBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList&        cmd    = context.GetCommandList();
      xiiGpuEmissiveLightHeader header = {};
      header.FrameIndex                = static_cast<xiiUInt32>(data.m_uiFrameIndex);
      cmd.UpdateBuffer(context.GetBuffer(data.m_hHeader), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&header), sizeof(header)));
      xiiGALStateTransitionDescription headerTransition;
      headerTransition.m_pResource       = context.GetBuffer(data.m_hHeader);
      headerTransition.m_NewState        = xiiGALResourceStateFlags::UnorderedAccess;
      headerTransition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      cmd.TransitionResourceStates(xiiMakeArrayPtr(&headerTransition, 1));
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
      cmd.ResolveAndSetShaderResourceBufferView("g_SkinningMatrices", context.GetBuffer(data.m_hSkinningMatrices)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Meshlets", context.GetBuffer(data.m_hMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials", context.GetBuffer(data.m_hSurfaceMaterials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hTriangles)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hHeader)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_Buffers", xiiGALShaderType::Compute);
      xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_Textures", xiiGALShaderType::Compute);
      cmd.ResolveAndSetSampler("g_sLinearWrap", xiiEmissiveLightManager::s_pState->m_pMaterialSampler.Borrow(), xiiGALShaderType::Compute);
      if (cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
        return;

      cmd.DispatchCompute({data.m_Description.m_Geometry.m_uiMaximumResidentMeshletCount, data.m_uiDispatchY, data.m_uiDispatchZ}); });
  buildPass.first->m_Description        = description;
  buildPass.first->m_uiFrameIndex       = uiFrameIndex;
  buildPass.first->m_uiTriangleCapacity = uiTriangleCapacity;
  buildPass.first->m_uiDispatchY        = uiDispatchY;
  buildPass.first->m_uiDispatchZ        = uiDispatchZ;
  buildPass.first->m_bExtract           = bExtract;

  const xiiUInt32 uiBlockCapacity       = (uiTriangleCapacity + s_uiTrianglesPerAliasBlock - 1U) / s_uiTrianglesPerAliasBlock;
  const xiiUInt32 uiMaxGroupCountX      = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties.m_uiMaxThreadGroupCountX;
  const xiiUInt32 uiAliasDispatchWidth  = xiiMath::Min(uiBlockCapacity, uiMaxGroupCountX);
  const xiiUInt32 uiAliasDispatchHeight = (uiBlockCapacity + uiAliasDispatchWidth - 1U) / uiAliasDispatchWidth;
  if (uiAliasDispatchHeight > uiMaxGroupCountY)
  {
    xiiLog::Error("Emissive alias construction requires a {} by {} dispatch, exceeding the device limits of {} by {}.", uiAliasDispatchWidth, uiAliasDispatchHeight, uiMaxGroupCountX, uiMaxGroupCountY);
    return outputs;
  }

  auto aliasBlockPass                        = graph.AddPass<EmissiveAliasBlockBuildPassData>("GPU Emissive Alias Block Build", xiiGALCommandQueueFlags::Compute, [&](EmissiveAliasBlockBuildPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hTriangles = builder.ReadBuffer(buildPass.first->m_hTriangles, xiiGALResourceStateFlags::ShaderResource);
      data.m_hHeader    = builder.ReadBuffer(buildPass.first->m_hHeader, xiiGALResourceStateFlags::ShaderResource);

      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiSize              = static_cast<xiiUInt64>(uiTriangleCapacity) * sizeof(xiiGpuEmissiveAliasEntry);
      bufferDescription.m_uiElementByteStride = sizeof(xiiGpuEmissiveAliasEntry);
      bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Default;
      data.m_hTriangleAliases                 = builder.WriteBuffer("GPU Emissive Triangle Aliases", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

      bufferDescription.m_uiSize              = static_cast<xiiUInt64>(uiBlockCapacity) * sizeof(float);
      bufferDescription.m_uiElementByteStride = sizeof(float);
      data.m_hBlockPowers                     = builder.WriteBuffer("GPU Emissive Alias Block Powers", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

      bufferDescription.m_uiSize              = static_cast<xiiUInt64>(uiTriangleCapacity) * sizeof(float);
      bufferDescription.m_uiElementByteStride = sizeof(float);
      data.m_hProbabilities                   = builder.WriteBuffer("GPU Emissive Alias Probabilities", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hSmall                           = builder.WriteBuffer("GPU Emissive Alias Small Stack", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hLarge                           = builder.WriteBuffer("GPU Emissive Alias Large Stack", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

      bufferDescription                  = {};
      bufferDescription.m_uiSize         = sizeof(xiiEmissiveAliasBuildConstants);
      bufferDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      bufferDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      bufferDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                  = builder.WriteBuffer("xiiEmissiveAliasBlockBuildConstants", bufferDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false); }, [](const EmissiveAliasBlockBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      {
        xiiGALMapHelper<xiiEmissiveAliasBuildConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        constants->TriangleCapacity   = data.m_uiTriangleCapacity;
        constants->BlockCapacity      = data.m_uiBlockCapacity;
        constants->TrianglesPerBlock  = s_uiTrianglesPerAliasBlock;
        constants->BlockDispatchWidth = data.m_uiDispatchWidth;
        constants->FrameIndex         = data.m_uiFrameIndex;
        constants->_Padding           = xiiVec3U32::MakeZero();
      }

      cmd.SetPipelineState(xiiEmissiveLightManager::s_pState->m_pAliasBlockBuildPipeline);
      cmd.ResolveAndSetConstantBuffer("xiiEmissiveAliasBuildConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hTriangles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_TriangleAliases", context.GetBuffer(data.m_hTriangleAliases)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_BlockPowers", context.GetBuffer(data.m_hBlockPowers)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_AliasProbabilities", context.GetBuffer(data.m_hProbabilities)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_AliasSmall", context.GetBuffer(data.m_hSmall)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_AliasLarge", context.GetBuffer(data.m_hLarge)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      if (cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
        return;
      cmd.DispatchCompute({data.m_uiDispatchWidth, data.m_uiDispatchHeight, 1U}); });
  aliasBlockPass.first->m_uiTriangleCapacity = uiTriangleCapacity;
  aliasBlockPass.first->m_uiBlockCapacity    = uiBlockCapacity;
  aliasBlockPass.first->m_uiDispatchWidth    = uiAliasDispatchWidth;
  aliasBlockPass.first->m_uiDispatchHeight   = uiAliasDispatchHeight;
  aliasBlockPass.first->m_uiFrameIndex       = static_cast<xiiUInt32>(uiFrameIndex);

  auto aliasGlobalPass                        = graph.AddPass<EmissiveAliasGlobalBuildPassData>("GPU Emissive Alias Global Build", xiiGALCommandQueueFlags::Compute, [&](EmissiveAliasGlobalBuildPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hHeader        = builder.ReadBuffer(buildPass.first->m_hHeader, xiiGALResourceStateFlags::ShaderResource);
      data.m_hBlockPowers   = builder.ReadBuffer(aliasBlockPass.first->m_hBlockPowers, xiiGALResourceStateFlags::ShaderResource);
      data.m_hProbabilities = builder.ReadWriteBuffer(aliasBlockPass.first->m_hProbabilities, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hSmall         = builder.ReadWriteBuffer(aliasBlockPass.first->m_hSmall, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hLarge         = builder.ReadWriteBuffer(aliasBlockPass.first->m_hLarge, xiiGALResourceStateFlags::UnorderedAccess);

      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiSize              = static_cast<xiiUInt64>(uiBlockCapacity) * sizeof(xiiGpuEmissiveAliasEntry);
      bufferDescription.m_uiElementByteStride = sizeof(xiiGpuEmissiveAliasEntry);
      bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Default;
      data.m_hBlockAliases                    = builder.WriteBuffer("GPU Emissive Block Aliases", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

      bufferDescription.m_uiSize              = sizeof(xiiGpuEmissiveAliasHeader);
      bufferDescription.m_uiElementByteStride = sizeof(xiiGpuEmissiveAliasHeader);
      data.m_hAliasHeader                     = builder.WriteBuffer("GPU Emissive Alias Header", bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

      bufferDescription                  = {};
      bufferDescription.m_uiSize         = sizeof(xiiEmissiveAliasBuildConstants);
      bufferDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      bufferDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      bufferDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                  = builder.WriteBuffer("xiiEmissiveAliasGlobalBuildConstants", bufferDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassAllowMerge(false); }, [](const EmissiveAliasGlobalBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      {
        xiiGALMapHelper<xiiEmissiveAliasBuildConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        constants->TriangleCapacity   = data.m_uiTriangleCapacity;
        constants->BlockCapacity      = data.m_uiBlockCapacity;
        constants->TrianglesPerBlock  = s_uiTrianglesPerAliasBlock;
        constants->BlockDispatchWidth = data.m_uiDispatchWidth;
        constants->FrameIndex         = data.m_uiFrameIndex;
        constants->_Padding           = xiiVec3U32::MakeZero();
      }

      cmd.SetPipelineState(xiiEmissiveLightManager::s_pState->m_pAliasGlobalBuildPipeline);
      cmd.ResolveAndSetConstantBuffer("xiiEmissiveAliasBuildConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_BlockPowers", context.GetBuffer(data.m_hBlockPowers)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_BlockAliases", context.GetBuffer(data.m_hBlockAliases)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_EmissiveAliasHeader", context.GetBuffer(data.m_hAliasHeader)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_AliasProbabilities", context.GetBuffer(data.m_hProbabilities)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_AliasSmall", context.GetBuffer(data.m_hSmall)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_AliasLarge", context.GetBuffer(data.m_hLarge)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      if (cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
        return;
      cmd.DispatchCompute({1U, 1U, 1U}); });
  aliasGlobalPass.first->m_uiTriangleCapacity = uiTriangleCapacity;
  aliasGlobalPass.first->m_uiBlockCapacity    = uiBlockCapacity;
  aliasGlobalPass.first->m_uiDispatchWidth    = uiAliasDispatchWidth;
  aliasGlobalPass.first->m_uiFrameIndex       = static_cast<xiiUInt32>(uiFrameIndex);

  outputs.m_hTriangles         = buildPass.first->m_hTriangles;
  outputs.m_hHeader            = buildPass.first->m_hHeader;
  outputs.m_hTriangleAliases   = aliasBlockPass.first->m_hTriangleAliases;
  outputs.m_hBlockAliases      = aliasGlobalPass.first->m_hBlockAliases;
  outputs.m_hAliasHeader       = aliasGlobalPass.first->m_hAliasHeader;
  outputs.m_uiTriangleCapacity = uiTriangleCapacity;

  s_pState->m_Stats.m_uiTriangleCapacity             = uiTriangleCapacity;
  s_pState->m_Stats.m_uiScheduledInstanceCount       = bExtract ? description.m_uiInstanceCount : 0U;
  s_pState->m_Stats.m_uiScheduledMeshletsPerInstance = bExtract ? description.m_Geometry.m_uiMaximumResidentMeshletCount : 0U;
  s_pState->m_Stats.m_uiLastScheduledFrame           = uiFrameIndex;
  return outputs;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_EmissiveLightManager);
