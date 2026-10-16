/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;

bool xiiBuiltinRenderPipelineRuntime::EnsureRayTracingReSTIRDIEmissiveVisibilityResources()
{
  auto& lightingPasses = m_Resources.m_LightingPasses;
  if (lightingPasses.m_pReSTIRDIEmissiveVisibilityPipeline != nullptr && lightingPasses.m_pReSTIRDIEmissiveVisibilityShaderBindingTable != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return false;

  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ReSTIRDIEmissiveVisibilityRT.xiiShader");
  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, false);
  if (!hPermutation.IsValid())
    return false;
  xiiResourceLock<xiiShaderPermutationResource> permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
  {
    xiiLog::Error("Failed to load the ReSTIR DI emissive visibility ray-tracing shader permutation.");
    return false;
  }

  xiiSharedPtr<xiiGALShader> pRayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  xiiSharedPtr<xiiGALShader> pRayMiss       = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  xiiSharedPtr<xiiGALShader> pClosestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  xiiSharedPtr<xiiGALShader> pAnyHit        = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
  if (pRayGeneration == nullptr || pRayMiss == nullptr || pClosestHit == nullptr || pAnyHit == nullptr)
  {
    xiiLog::Error("The ReSTIR DI emissive visibility permutation does not contain ray-generation, miss, closest-hit, and any-hit stages.");
    return false;
  }

  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1U;
  pipelineDescription.m_uiMaximumPayloadSize                     = sizeof(xiiUInt32);
  pipelineDescription.m_uiMaximumAttributeSize                   = sizeof(float) * 2U;

  auto& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("ReSTIREmissiveVisibilityRayGeneration");
  rayGeneration.m_pShader = pRayGeneration;
  auto& miss              = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("ReSTIREmissiveVisibilityMiss");
  miss.m_pShader = pRayMiss;
  auto& hit      = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("ReSTIREmissiveVisibilityTriangleHit");
  hit.m_pClosestHitShader = pClosestHit;
  hit.m_pAnyHitShader     = pAnyHit;

  lightingPasses.m_pReSTIRDIEmissiveVisibilityPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (lightingPasses.m_pReSTIRDIEmissiveVisibilityPipeline == nullptr)
  {
    xiiLog::Error("Failed to create the ReSTIR DI emissive visibility ray-tracing pipeline.");
    return false;
  }
  lightingPasses.m_pReSTIRDIEmissiveVisibilityPipeline->SetDebugName("ReSTIR DI Emissive Visibility Pipeline");

  const xiiGALRayTracingProperties& properties      = pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                   uiBaseAlignment = xiiMath::Max(1U, properties.m_uiShaderGroupBaseAlignment);
  const xiiUInt64                   uiRecordStride  = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(properties.m_uiShaderGroupHandleSize), uiBaseAlignment);
  if (uiRecordStride == 0U || (properties.m_uiMaxShaderRecordStride != 0U && uiRecordStride > properties.m_uiMaxShaderRecordStride))
  {
    xiiLog::Error("The device reported invalid ReSTIR DI emissive visibility shader binding table alignment properties.");
    lightingPasses.m_pReSTIRDIEmissiveVisibilityPipeline.Clear();
    return false;
  }

  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                                        = uiRecordStride * 3U;
  sbtDescription.m_BindFlags                                     = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                                         = xiiGALResourceUsage::Mutable;
  sbtDescription.m_Mode                                          = xiiGALBufferMode::Raw;
  lightingPasses.m_pReSTIRDIEmissiveVisibilityShaderBindingTable = pDevice->CreateBuffer(sbtDescription);
  if (lightingPasses.m_pReSTIRDIEmissiveVisibilityShaderBindingTable == nullptr)
  {
    xiiLog::Error("Failed to create the ReSTIR DI emissive visibility shader binding table.");
    lightingPasses.m_pReSTIRDIEmissiveVisibilityPipeline.Clear();
    return false;
  }

  lightingPasses.m_pReSTIRDIEmissiveVisibilityShaderBindingTable->SetDebugName("ReSTIR DI Emissive Visibility Shader Binding Table");
  lightingPasses.m_uiReSTIRDIEmissiveVisibilityShaderRecordStride = static_cast<xiiUInt32>(uiRecordStride);
  return true;
}

bool xiiBuiltinRenderPipelineRuntime::EnsureRayTracingShadowResources()
{
  auto& shadowPasses = m_Resources.m_ShadowPasses;
  if (shadowPasses.m_pRayTracedShadowPipeline != nullptr && shadowPasses.m_pRayTracedShadowShaderBindingTable != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return false;

  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RTShadow.xiiShader");
  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, false);
  if (!hPermutation.IsValid())
    return false;
  xiiResourceLock<xiiShaderPermutationResource> permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
  {
    xiiLog::Error("Failed to load the hardware ray-traced shadow shader permutation.");
    return false;
  }

  xiiSharedPtr<xiiGALShader> pRayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  xiiSharedPtr<xiiGALShader> pRayMiss       = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  xiiSharedPtr<xiiGALShader> pClosestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  xiiSharedPtr<xiiGALShader> pAnyHit        = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
  if (pRayGeneration == nullptr || pRayMiss == nullptr || pClosestHit == nullptr || pAnyHit == nullptr)
  {
    xiiLog::Error("The hardware shadow permutation does not contain ray-generation, miss, closest-hit, and any-hit stages.");
    return false;
  }

  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1U;
  pipelineDescription.m_uiMaximumPayloadSize                     = sizeof(xiiUInt32);
  pipelineDescription.m_uiMaximumAttributeSize                   = sizeof(float) * 2U;

  auto& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("RTShadowRayGeneration");
  rayGeneration.m_pShader = pRayGeneration;
  auto& miss              = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("RTShadowMiss");
  miss.m_pShader = pRayMiss;
  auto& hit      = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("RTShadowTriangleHit");
  hit.m_pClosestHitShader = pClosestHit;
  hit.m_pAnyHitShader     = pAnyHit;

  shadowPasses.m_pRayTracedShadowPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (shadowPasses.m_pRayTracedShadowPipeline == nullptr)
  {
    xiiLog::Error("Failed to create the hardware ray-traced shadow pipeline.");
    return false;
  }
  shadowPasses.m_pRayTracedShadowPipeline->SetDebugName("RT Shadow Pipeline");

  const xiiGALRayTracingProperties& properties      = pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                   uiBaseAlignment = xiiMath::Max(1U, properties.m_uiShaderGroupBaseAlignment);
  const xiiUInt64                   uiRecordStride  = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(properties.m_uiShaderGroupHandleSize), uiBaseAlignment);
  if (uiRecordStride == 0U || (properties.m_uiMaxShaderRecordStride != 0U && uiRecordStride > properties.m_uiMaxShaderRecordStride))
  {
    xiiLog::Error("The device reported invalid shadow shader binding table alignment properties.");
    shadowPasses.m_pRayTracedShadowPipeline.Clear();
    return false;
  }

  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                           = uiRecordStride * 3U;
  sbtDescription.m_BindFlags                        = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                            = xiiGALResourceUsage::Mutable;
  sbtDescription.m_Mode                             = xiiGALBufferMode::Raw;
  shadowPasses.m_pRayTracedShadowShaderBindingTable = pDevice->CreateBuffer(sbtDescription);
  if (shadowPasses.m_pRayTracedShadowShaderBindingTable == nullptr)
  {
    xiiLog::Error("Failed to create the hardware shadow shader binding table.");
    shadowPasses.m_pRayTracedShadowPipeline.Clear();
    return false;
  }

  shadowPasses.m_pRayTracedShadowShaderBindingTable->SetDebugName("RT Shadow Shader Binding Table");
  shadowPasses.m_uiRayTracedShadowShaderRecordStride = static_cast<xiiUInt32>(uiRecordStride);
  return true;
}

bool xiiBuiltinRenderPipelineRuntime::EnsureRayTracingGlobalIlluminationResources()
{
  auto& lightingPasses = m_Resources.m_LightingPasses;
  if (lightingPasses.m_pRTGIPipeline != nullptr && lightingPasses.m_pRTGIShaderBindingTable != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return false;

  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RTGIFinalGather.xiiShader");
  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, false);
  if (!hPermutation.IsValid())
    return false;
  xiiResourceLock<xiiShaderPermutationResource> permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
  {
    xiiLog::Error("Failed to load the hardware ray-traced GI shader permutation.");
    return false;
  }

  xiiSharedPtr<xiiGALShader> pRayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  xiiSharedPtr<xiiGALShader> pRayMiss       = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  xiiSharedPtr<xiiGALShader> pClosestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  xiiSharedPtr<xiiGALShader> pAnyHit        = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
  if (pRayGeneration == nullptr || pRayMiss == nullptr || pClosestHit == nullptr || pAnyHit == nullptr)
  {
    xiiLog::Error("The hardware GI permutation does not contain ray-generation, miss, closest-hit, and any-hit stages.");
    return false;
  }

  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  const xiiUInt32 uiDeviceRecursionDepth                         = xiiMath::Max(1U, pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiMaxRecursionDepth);
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = xiiMath::Clamp(xiiMath::Min(m_Resources.m_LightingSystem.GetSettings().m_uiRTGIMaxPathHits, 2U) + 1U, 1U, xiiMath::Min(3U, uiDeviceRecursionDepth));
  pipelineDescription.m_uiMaximumPayloadSize                     = 40U;
  pipelineDescription.m_uiMaximumAttributeSize                   = sizeof(float) * 2U;

  auto& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("RTGIRayGeneration");
  rayGeneration.m_pShader = pRayGeneration;
  auto& miss              = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("RTGIMiss");
  miss.m_pShader = pRayMiss;
  auto& hit      = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("RTGITriangleHit");
  hit.m_pClosestHitShader = pClosestHit;
  hit.m_pAnyHitShader     = pAnyHit;

  lightingPasses.m_pRTGIPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (lightingPasses.m_pRTGIPipeline == nullptr)
  {
    xiiLog::Error("Failed to create the hardware ray-traced GI pipeline.");
    return false;
  }
  lightingPasses.m_pRTGIPipeline->SetDebugName("RT GI Pipeline");

  const xiiGALRayTracingProperties& properties      = pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                   uiBaseAlignment = xiiMath::Max(1U, properties.m_uiShaderGroupBaseAlignment);
  const xiiUInt64                   uiRecordStride  = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(properties.m_uiShaderGroupHandleSize), uiBaseAlignment);
  if (uiRecordStride == 0U || (properties.m_uiMaxShaderRecordStride != 0U && uiRecordStride > properties.m_uiMaxShaderRecordStride))
  {
    xiiLog::Error("The device reported invalid GI shader binding table alignment properties.");
    lightingPasses.m_pRTGIPipeline.Clear();
    return false;
  }

  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                  = uiRecordStride * 3U;
  sbtDescription.m_BindFlags               = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                   = xiiGALResourceUsage::Mutable;
  sbtDescription.m_Mode                    = xiiGALBufferMode::Raw;
  lightingPasses.m_pRTGIShaderBindingTable = pDevice->CreateBuffer(sbtDescription);
  if (lightingPasses.m_pRTGIShaderBindingTable == nullptr)
  {
    xiiLog::Error("Failed to create the hardware GI shader binding table.");
    lightingPasses.m_pRTGIPipeline.Clear();
    return false;
  }

  lightingPasses.m_pRTGIShaderBindingTable->SetDebugName("RT GI Shader Binding Table");
  lightingPasses.m_uiRTGIShaderRecordStride = static_cast<xiiUInt32>(uiRecordStride);
  return true;
}

bool xiiBuiltinRenderPipelineRuntime::EnsureRayTracingReflectionResources()
{
  auto& lightingPasses = m_Resources.m_LightingPasses;
  if (lightingPasses.m_pRTReflectionPipeline != nullptr && lightingPasses.m_pRTReflectionShaderBindingTable != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr || pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return false;

  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RTReflection.xiiShader");
  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, false);
  if (!hPermutation.IsValid())
    return false;
  xiiResourceLock<xiiShaderPermutationResource> permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
  {
    xiiLog::Error("Failed to load the hardware ray-tracing reflection shader permutation.");
    return false;
  }

  xiiSharedPtr<xiiGALShader> pRayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  xiiSharedPtr<xiiGALShader> pRayMiss       = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  xiiSharedPtr<xiiGALShader> pClosestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  xiiSharedPtr<xiiGALShader> pAnyHit        = permutation->GetGALShader(xiiGALShaderType::RayAnyHit);
  if (pRayGeneration == nullptr || pRayMiss == nullptr || pClosestHit == nullptr || pAnyHit == nullptr)
  {
    xiiLog::Error("The hardware reflection permutation does not contain ray-generation, miss, closest-hit, and any-hit stages.");
    return false;
  }

  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  const xiiUInt32 uiDeviceRecursionDepth                         = xiiMath::Max(1U, pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiMaxRecursionDepth);
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = xiiMath::Clamp(xiiMath::Min(m_Resources.m_LightingSystem.GetSettings().m_uiRTReflectionMaxPathHits, 2U) + 1U, 1U, xiiMath::Min(3U, uiDeviceRecursionDepth));
  pipelineDescription.m_uiMaximumPayloadSize                     = 40U;
  pipelineDescription.m_uiMaximumAttributeSize                   = 8U;

  xiiGALRayTracingGeneralShaderGroupDescription& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("RTReflectionRayGeneration");
  rayGeneration.m_pShader = pRayGeneration;

  xiiGALRayTracingGeneralShaderGroupDescription& miss = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("RTReflectionMiss");
  miss.m_pShader = pRayMiss;

  xiiGALRayTracingTriangleHitShaderGroupDescription& hit = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("RTReflectionTriangleHit");
  hit.m_pClosestHitShader = pClosestHit;
  hit.m_pAnyHitShader     = pAnyHit;

  lightingPasses.m_pRTReflectionPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (lightingPasses.m_pRTReflectionPipeline == nullptr)
  {
    xiiLog::Error("Failed to create the hardware ray-tracing reflection pipeline.");
    return false;
  }
  lightingPasses.m_pRTReflectionPipeline->SetDebugName("RT Reflection Pipeline");

  const xiiGALRayTracingProperties& rayTracingProperties = pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                   uiBaseAlignment      = xiiMath::Max(1U, rayTracingProperties.m_uiShaderGroupBaseAlignment);
  const xiiUInt64                   uiRecordStride       = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(rayTracingProperties.m_uiShaderGroupHandleSize), uiBaseAlignment);
  if (uiRecordStride == 0U || (rayTracingProperties.m_uiMaxShaderRecordStride != 0U && uiRecordStride > rayTracingProperties.m_uiMaxShaderRecordStride))
  {
    xiiLog::Error("The device reported invalid shader binding table alignment properties.");
    lightingPasses.m_pRTReflectionPipeline.Clear();
    return false;
  }

  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                          = uiRecordStride * 3U;
  sbtDescription.m_BindFlags                       = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                           = xiiGALResourceUsage::Mutable;
  sbtDescription.m_Mode                            = xiiGALBufferMode::Raw;
  lightingPasses.m_pRTReflectionShaderBindingTable = pDevice->CreateBuffer(sbtDescription);
  if (lightingPasses.m_pRTReflectionShaderBindingTable == nullptr)
  {
    xiiLog::Error("Failed to create the hardware reflection shader binding table.");
    lightingPasses.m_pRTReflectionPipeline.Clear();
    return false;
  }

  lightingPasses.m_pRTReflectionShaderBindingTable->SetDebugName("RT Reflection Shader Binding Table");
  lightingPasses.m_uiRTReflectionShaderRecordStride = static_cast<xiiUInt32>(uiRecordStride);
  return true;
}

xiiSharedPtr<xiiGALComputePipelineState> xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(xiiSharedPtr<xiiGALComputePipelineState>& inout_pPipeline, xiiStringView sShaderPath)
{
  if (inout_pPipeline != nullptr)
    return inout_pPipeline;

  XII_SCOPE_EXIT(if (inout_pPipeline == nullptr) {
    m_StageSetupResult = XII_FAILURE;
    xiiLog::Error("Failed to create built-in compute pipeline '{}'.", sShaderPath);
  });
  if (!xiiGALDevice::HasDefaultDevice())
    return nullptr;

  xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);

  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, false);
  if (!hPermutation.IsValid())
    return {};

  xiiResourceLock<xiiShaderPermutationResource> pPermutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!pPermutation.IsValid() || !pPermutation->IsShaderValid())
    return nullptr;

  xiiGALComputePipelineStateCreationDescription description;
  description.m_pComputeShader             = pPermutation->GetGALShader(xiiGALShaderType::Compute);
  description.m_pPipelineResourceSignature = pPermutation->GetPipelineResourceSignature();

  inout_pPipeline = xiiGALPipelineCache::GetPipeline(description);
  return inout_pPipeline;
}

// static
xiiSharedPtr<xiiGALGraphicsPipelineState> xiiBuiltinRenderPipelineRuntime::EnsureGraphicsPipeline(xiiSharedPtr<xiiGALGraphicsPipelineState>& pipeline, xiiStringView shader, const xiiSharedPtr<xiiGALRenderPass>& renderPass, xiiUInt32 subpass)
{
  return xiiRenderPassUtils::LoadGraphicsPipeline(pipeline, shader, renderPass, subpass);
}
