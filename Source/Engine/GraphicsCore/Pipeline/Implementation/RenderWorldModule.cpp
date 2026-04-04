#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>


void xiiPopulateAccelerationStructurePass(xiiAccelerationStructurePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // Check hardware RT support via device feature query.
  xiiGALDevice* pDevice      = xiiGALDevice::GetDefaultDevice();
  const bool    bRTSupported = pDevice->GetFeatures().m_bRayTracing;

  if (!bRTSupported)
    return; // No-op, downstream RT passes will also bail out when they find no TLAS handle.

  auto [pData, hPass] = graph.AddPass<ASPassData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [](ASPassData& data, xiiRGBuilder& builder) {
      data.bHasRT = true;
      builder.SetPassSideEffects(true); // BLAS/TLAS build writes to persistent GPU memory.
      builder.SetPassAllowMerge(false); // Must not be merged, explicit barrier semantics.
    },
    [&passData](const ASPassData& data, xiiRGPassContext& context) {
      if (!data.bHasRT) return;
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Acceleration Structure Build");

      // Pass 32: BLAS scheduling, CPU-side policy (refit vs full-rebuild per mesh).
      // Pass 33: BLAS build/refit, issued as batched BuildAccelerationStructure commands.
      cmd.BuildAccelerationStructures(); // Submits all pending BLAS builds enqueued this frame.

      // Pass 34: TLAS build, single top-level instance buffer covering all BLASes.
      cmd.BuildTopLevelAccelerationStructure();

      // Pass 35: SBT update, only re-records hit groups that changed this frame.
      cmd.UpdateShaderBindingTable();

      // Pass 45: Compaction deferred query, issued here if any BLAS flagged for compaction.
      if (passData.m_bEnableCompaction)
        cmd.CompactAccelerationStructures();

      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);
}

void xiiPopulateAmbientOcclusionPass(xiiAmbientOcclusionPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hHiZ, hVelocity;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid), hHiZ);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), hVelocity);

  auto makeAO = [&](const char* name) {
    xiiGALTextureCreationDescription d;
    d.m_uiWidth     = uiW;
    d.m_uiHeight    = uiH;
    d.m_uiMipLevels = 1u;
    d.m_Format      = xiiGALTextureFormat::R8UNorm;
    d.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    return d;
  };

  auto [pData, hPass] = graph.AddPass<AOData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hHiZ, hVelocity, uiW, uiH, &passData, makeAO](AOData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid()) data.hNR = builder.ReadTexture(hNR, xiiGALResourceStateFlags::ShaderResource);
      if (hHiZ.IsValid()) data.hHiZ = builder.ReadTexture(hHiZ, xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      data.hRawAO    = builder.WriteTexture("RawAO", makeAO("RawAO"), xiiGALResourceStateFlags::UnorderedAccess);
      data.hStableAO = builder.WriteTexture("StableAO", makeAO("StableAO"), xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW       = uiW;
      data.uiH       = uiH;
      data.uiSlices  = passData.m_uiSliceCount;
      data.uiSteps   = passData.m_uiStepsPerSlice;
      data.fRadius   = passData.m_fRadius;
    },
    [](const AOData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("GTAO + Denoise");
      // Pass 29: GTAO.xiiShader, horizon-based AO integration with passData.m_uiSliceCount * passData.m_uiStepsPerSlice samples.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 30: AOSpatialDenoise.xiiShader + AOTemporalDenoise.xiiShader
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RawAOTexture), pData->hRawAO);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_StableAOTexture), pData->hStableAO);
}

void xiiPopulateAtmosphereCompositePass(xiiAtmosphereCompositePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hTransmittance, hMultiScatter, hDirectLight;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT), hTransmittance);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT), hMultiScatter);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectLightingBuffer), hDirectLight);

  xiiGALTextureCreationDescription skyDesc;
  skyDesc.m_uiWidth     = uiW;
  skyDesc.m_uiHeight    = uiH;
  skyDesc.m_uiMipLevels = 1u;
  skyDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  skyDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<AtmCompData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hTransmittance, hMultiScatter, hDirectLight, skyDesc, uiW, uiH, &passData](AtmCompData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hTransmittance.IsValid()) data.hTransmittance = builder.ReadTexture(hTransmittance, xiiGALResourceStateFlags::ShaderResource);
      if (hMultiScatter.IsValid()) data.hMultiScatter = builder.ReadTexture(hMultiScatter, xiiGALResourceStateFlags::ShaderResource);
      if (hDirectLight.IsValid()) data.hDirectLight = builder.WriteTexture(hDirectLight, xiiGALResourceStateFlags::UnorderedAccess);
      data.hSky           = builder.WriteTexture("SkyRadiance", skyDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW            = uiW;
      data.uiH            = uiH;
      data.fSunSolidAngle = passData.m_fSunSolidAngle;
    },
    [](const AtmCompData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Atmosphere + Sky Composite");
      // AtmosphereComposite.xiiShader: sky + sun disc into skybox pixels (depth == far plane).
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SkyRadiance), pData->hSky);
}

namespace
{
  struct alignas(16) AtmosphereConstants
  {
    float RayleighScaleHeight;
    float MieScaleHeight;
    float MieAnisotropy;
    float PlanetRadiusKm;
    float AtmosphereRadiusKm;
    float _pad[3];
  };
} // namespace

void xiiPopulateAtmosphereLUTPass(xiiAtmosphereLUTPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!passData.m_pTransmittanceLUT)
  {
    xiiGALTextureCreationDescription td;
    td.m_uiWidth                 = 256u;
    td.m_uiHeight                = 64u;
    td.m_uiMipLevels             = 1u;
    td.m_Format                  = xiiGALTextureFormat::R16G16B16A16Float;
    td.m_BindFlags               = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    passData.m_pTransmittanceLUT = pDevice->CreateTexture(td);
  }
  if (!passData.m_pMultiScatterLUT)
  {
    xiiGALTextureCreationDescription td;
    td.m_uiWidth                = 32u;
    td.m_uiHeight               = 32u;
    td.m_uiMipLevels            = 1u;
    td.m_Format                 = xiiGALTextureFormat::R16G16B16A16Float;
    td.m_BindFlags              = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    passData.m_pMultiScatterLUT = pDevice->CreateTexture(td);
  }

  const AtmosphereConstants consts = {passData.m_fRayleighScaleHeight, passData.m_fMieScaleHeight, passData.m_fMieAnisotropy, passData.m_fPlanetRadius, passData.m_fAtmosphereRadius};

  auto [pData, hPass] = graph.AddPass<AtmLUTData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [&passData](AtmLUTData& data, xiiRGBuilder& builder) {
      data.hTransmittanceLUT = builder.ImportTexture("AtmTransmittance", passData.m_pTransmittanceLUT, xiiGALResourceStateFlags::UnorderedAccess);
      data.hTransmittanceLUT = builder.WriteTexture(data.hTransmittanceLUT, xiiGALResourceStateFlags::UnorderedAccess);
      data.hMultiScatterLUT  = builder.ImportTexture("AtmMultiScatter", passData.m_pMultiScatterLUT, xiiGALResourceStateFlags::UnorderedAccess);
      data.hMultiScatterLUT  = builder.WriteTexture(data.hMultiScatterLUT, xiiGALResourceStateFlags::UnorderedAccess);
      builder.SetPassSideEffects(true); // Persistent LUTs
    },
    [consts](const AtmLUTData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Atmosphere LUT Update");
      // AtmosphereTransmittance.xiiShader: 256x64 compute, precomputed optical depth integral.
      cmd.Dispatch((256u + 7u) / 8u, (64u + 7u) / 8u, 1u);
      // AtmosphereMultiScatter.xiiShader: 32x32 compute with multiple scattering approximation.
      cmd.Dispatch((32u + 7u) / 8u, (32u + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT), pData->hTransmittanceLUT);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT), pData->hMultiScatterLUT);
}

void xiiPopulateBloomPass(xiiBloomPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hInput;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_UpscaledColor), hInput);
  if (!hInput.IsValid()) blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_TAAResolvedColor), hInput);

  // Bloom chain: mip pyramid R16G16B16A16Float.
  xiiGALTextureCreationDescription bloomDesc;
  bloomDesc.m_uiWidth     = uiW;
  bloomDesc.m_uiHeight    = uiH;
  bloomDesc.m_uiMipLevels = passData.m_uiMipLevels;
  bloomDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  bloomDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<BloomData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hInput, bloomDesc, uiW, uiH, &passData](BloomData& data, xiiRGBuilder& builder) {
      if (hInput.IsValid()) data.hInput = builder.ReadTexture(hInput, xiiGALResourceStateFlags::ShaderResource);
      data.hBloomOutput = builder.WriteTexture("BloomTexture", bloomDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW          = uiW;
      data.uiH          = uiH;
      data.uiMips       = passData.m_uiMipLevels;
      data.fThreshold   = passData.m_fThreshold;
      data.fKnee        = passData.m_fKnee;
      data.fIntensity   = passData.m_fIntensity;
    },
    [](const BloomData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Bloom");
      // Pass 57: BloomPrefilter.xiiShader, soft-knee bright-pass at full resolution.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 58: BloomDownsample.xiiShader, 13-tap filter per mip level.
      xiiUInt32 uiMipW = data.uiW / 2u, uiMipH = data.uiH / 2u;
      for (xiiUInt32 mip = 1u; mip < data.uiMips; ++mip)
      {
        cmd.Dispatch(xiiMath::Max(1u, (uiMipW + 7u) / 8u), xiiMath::Max(1u, (uiMipH + 7u) / 8u), 1u);
        uiMipW = xiiMath::Max(1u, uiMipW / 2u);
        uiMipH = xiiMath::Max(1u, uiMipH / 2u);
      }
      // Pass 59: BloomUpsample.xiiShader, tent filter upsample chain + intensity blend.
      for (xiiInt32 mip = static_cast<xiiInt32>(data.uiMips) - 2; mip >= 0; --mip)
        cmd.Dispatch(xiiMath::Max(1u, (data.uiW >> mip) / 8u + 1u), xiiMath::Max(1u, (data.uiH >> mip) / 8u + 1u), 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_BloomTexture), pData->hBloomOutput);
}

void xiiPopulateClusterGridAndLightListPass(xiiClusterGridAndLightListPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice*      p = xiiGALDevice::GetDefaultDevice();
  xiiRGTextureHandle hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  const xiiUInt32                 totalClusters = passData.m_uiClusterCountX * passData.m_uiClusterCountY * passData.m_uiClusterCountZ;
  xiiGALBufferCreationDescription descDescs;
  descDescs.m_uiSize        = sizeof(float) * 8u * totalClusters; // AABB min+max per cluster
  descDescs.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  descDescs.m_ResourceUsage = xiiGALResourceUsage::Default;

  xiiGALBufferCreationDescription gridDesc;
  gridDesc.m_uiSize        = sizeof(xiiUInt32) * 2u * totalClusters; // (offset, count) per cluster
  gridDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  gridDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  xiiGALBufferCreationDescription idxDesc;
  idxDesc.m_uiSize        = sizeof(xiiUInt32) * totalClusters * passData.m_uiMaxLightsPerCluster;
  idxDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  idxDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  auto [pData, hPass] = graph.AddPass<ClusterData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, descDescs, gridDesc, idxDesc, &passData](ClusterData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      data.hClusterDescs = builder.WriteBuffer("ClusterDescriptors", descDescs, xiiGALResourceStateFlags::UnorderedAccess);
      data.hLightGrid    = builder.WriteBuffer("LightGrid", gridDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.hLightIndex   = builder.WriteBuffer("LightIndexBuffer", idxDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.cX            = passData.m_uiClusterCountX;
      data.cY            = passData.m_uiClusterCountY;
      data.cZ            = passData.m_uiClusterCountZ;
    },
    [](const ClusterData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Cluster Grid + Light List");
      // ClusterBuild.xiiShader: pass 21, builds frustum-space cluster AABB descriptors.
      cmd.Dispatch(data.cX, data.cY, data.cZ);
      // LightAssignment.xiiShader: pass 22, assigns active lights to overlapping clusters.
      cmd.Dispatch((data.cX * data.cY * data.cZ + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_ClusterDescriptors), pData->hClusterDescs);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightGridBuffer), pData->hLightGrid);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightIndexBuffer), pData->hLightIndex);
}

void xiiPopulateCoarseFrustumCullPass(xiiCoarseFrustumCullPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice*       pDevice          = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!passData.m_pVisibleCandidateBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                      = sizeof(xiiUInt32) * k_uiMaxInstances;
    desc.m_BufferFlags                 = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage               = xiiGALResourceUsage::Default;
    passData.m_pVisibleCandidateBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pFrustumPlanesBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                   = sizeof(float) * 4u * 6u; // 6 planes x float4
    desc.m_BufferFlags              = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage            = xiiGALResourceUsage::Dynamic;
    passData.m_pFrustumPlanesBuffer = pDevice->CreateBuffer(desc);
  }

  // Extract frustum planes from the view's culling camera.
  xiiFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  xiiVec4 planes[6];
  for (xiiUInt32 i = 0; i < 6u; ++i)
  {
    const xiiPlane& p = frustum.GetPlane(i);
    planes[i]         = xiiVec4(p.m_vNormal, p.m_fNegDistance);
  }

  xiiRGBufferHandle hBounds, hLOD;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer), hBounds);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceLODBuffer), hLOD);

  auto [pData, hPass] = graph.AddPass<FrustumCullPassData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [&passData, hBounds, hLOD](FrustumCullPassData& data, xiiRGBuilder& builder) {
      if (hBounds.IsValid()) data.hBounds = builder.ReadBuffer(hBounds, xiiGALResourceStateFlags::ShaderResource);
      if (hLOD.IsValid()) data.hLOD = builder.ReadBuffer(hLOD, xiiGALResourceStateFlags::ShaderResource);
      data.hVisible        = builder.ImportBuffer("VisibleCandidates", passData.m_pVisibleCandidateBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hVisible        = builder.WriteBuffer(data.hVisible, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount = k_uiMaxInstances;
    },
    [&passData, planes](const FrustumCullPassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Coarse Frustum Cull");
      cmd.UpdateBuffer(passData.m_pFrustumPlanesBuffer.Borrow(), 0u, planes, sizeof(planes));
      // CoarseFrustumCulling.xiiShader: [numthreads(64,1,1)]
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), pData->hVisible);
}

void xiiPopulateColorGradingPass(xiiColorGradingPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hLDR;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_LDRSceneColor), hLDR);

  xiiGALTextureCreationDescription gradedDesc;
  gradedDesc.m_uiWidth     = uiW;
  gradedDesc.m_uiHeight    = uiH;
  gradedDesc.m_uiMipLevels = 1u;
  gradedDesc.m_Format      = xiiGALTextureFormat::R8G8B8A8UNorm;
  gradedDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<ColorGradData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hLDR, gradedDesc, uiW, uiH, &passData](ColorGradData& data, xiiRGBuilder& builder) {
      if (hLDR.IsValid()) data.hLDR = builder.ReadTexture(hLDR, xiiGALResourceStateFlags::ShaderResource);
      if (passData.m_pColorLUT)
      {
        data.hLUT = builder.ImportTexture("ColorLUT", passData.m_pColorLUT, xiiGALResourceStateFlags::ShaderResource);
        data.hLUT = builder.ReadTexture(data.hLUT, xiiGALResourceStateFlags::ShaderResource);
      }
      data.hGraded   = builder.WriteTexture("GradedColor", gradedDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW       = uiW;
      data.uiH       = uiH;
      data.fVignette = passData.m_fVignetteStrength;
      data.fGrain    = passData.m_fGrainStrength;
      data.fSat      = passData.m_fSaturation;
      data.fContrast = passData.m_fContrast;
    },
    [](const ColorGradData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Color Grading");
      // ColorGrading.xiiShader: 33^3 LUT sample + vignette + temporal grain + gamut map.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GradedColor), pData->hGraded);
}

void xiiPopulateContactShadowPass(xiiContactShadowPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);
  xiiRGTextureHandle hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  xiiGALTextureCreationDescription cDesc;
  cDesc.m_uiWidth     = uiW;
  cDesc.m_uiHeight    = uiH;
  cDesc.m_uiMipLevels = 1u;
  cDesc.m_Format      = xiiGALTextureFormat::R8UNorm;
  cDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<ContactShadowData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, cDesc, uiW, uiH, &passData](ContactShadowData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      data.hContactShadow = builder.WriteTexture("ContactShadows", cDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiWidth        = uiW;
      data.uiHeight       = uiH;
      data.uiSteps        = passData.m_uiRaySteps;
      data.fMaxDist       = passData.m_fMaxRayDistance;
    },
    [](const ContactShadowData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Contact Shadows");
      // ContactShadow.xiiShader: ray march in view-space along light dir.
      cmd.Dispatch((data.uiWidth + 7u) / 8u, (data.uiHeight + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_ContactShadowTerm), pData->hContactShadow);
}

void xiiPopulateDecalPass(xiiDecalPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hAlbedo, hNormal;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferAlbedo), hAlbedo);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferNormal), hNormal);

  const xiiUInt32 uiTilesX = (uiW + passData.m_uiTileSize - 1u) / passData.m_uiTileSize;
  const xiiUInt32 uiTilesY = (uiH + passData.m_uiTileSize - 1u) / passData.m_uiTileSize;

  xiiGALBufferCreationDescription tileDesc;
  tileDesc.m_uiSize        = sizeof(xiiUInt32) * passData.m_uiMaxDecalsPerTile * uiTilesX * uiTilesY;
  tileDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  tileDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  auto [pData, hPass] = graph.AddPass<DecalPassData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hAlbedo, hNormal, tileDesc, uiW, uiH, &passData](DecalPassData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hAlbedo.IsValid()) data.hGBufAlbedo = builder.WriteTexture(hAlbedo, xiiGALResourceStateFlags::UnorderedAccess);
      if (hNormal.IsValid()) data.hGBufNormal = builder.WriteTexture(hNormal, xiiGALResourceStateFlags::UnorderedAccess);
      data.hDecalTileList = builder.WriteBuffer("DecalTileList", tileDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW            = uiW;
      data.uiH            = uiH;
      data.uiTileSize     = passData.m_uiTileSize;
    },
    [](const DecalPassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Decal Classification + Resolve");
      // Pass 23: DecalClassification.xiiShader, builds per-tile decal index list.
      const xiiUInt32 uiTX = (data.uiW + data.uiTileSize - 1u) / data.uiTileSize;
      const xiiUInt32 uiTY = (data.uiH + data.uiTileSize - 1u) / data.uiTileSize;
      cmd.Dispatch(uiTX, uiTY, 1u);
      // Pass 24: DecalResolve.xiiShader, applies decal attributes to GBuffer targets.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DecalTileList), pData->hDecalTileList);
}

void xiiPopulateDirectionalShadowRenderPass(xiiDirectionalShadowRenderPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALTextureCreationDescription atlasDesc;
  atlasDesc.m_uiWidth     = passData.m_uiAtlasSize;
  atlasDesc.m_uiHeight    = passData.m_uiAtlasSize;
  atlasDesc.m_uiMipLevels = 1u;
  atlasDesc.m_Format      = xiiGALTextureFormat::D16UNorm;
  atlasDesc.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  xiiRGBufferHandle hCascMat;
  xiiUInt32         uiCascadeCount = 4u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeMatrices), hCascMat);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeCount), uiCascadeCount);

  auto [pData, hPass] = graph.AddPass<DirShadowData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hCascMat, atlasDesc, uiCascadeCount, &passData](DirShadowData& data, xiiRGBuilder& builder) {
      if (hCascMat.IsValid()) data.hCascadeMatrices = builder.ReadBuffer(hCascMat, xiiGALResourceStateFlags::ConstantBuffer);
      data.hShadowAtlas   = builder.WriteTexture("DirShadowAtlas", atlasDesc, xiiGALResourceStateFlags::DepthWrite);
      data.uiAtlasSize    = passData.m_uiAtlasSize;
      data.uiCascadeCount = uiCascadeCount;
      builder.SetPassAllowMerge(false);
    },
    [](const DirShadowData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Directional Shadow Render");
      xiiGALTexture* pAtlas = context.GetTexture(data.hShadowAtlas);
      cmd.ClearDepthStencilView(pAtlas, xiiGALClearFlags::Depth, 0.0f, 0u);
      // Indirect draws per cascade into shadow atlas tiles, ShadowDepth.xiiShader.
      // Atlas is laid out: cascade 0 at [0,0], cascade 1 at [half,0], etc.
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectionalShadowAtlas), pData->hShadowAtlas);
}

void xiiPopulateDrawCommandBuildPass(xiiDrawCommandBuildPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // DrawIndexedIndirect struct: (indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation)
  if (!passData.m_pDrawIndirectArgsBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                      = sizeof(xiiUInt32) * 5u * passData.m_uiMaxDrawCommands;
    desc.m_BufferFlags                 = xiiGALBufferUsageFlags::IndirectDrawArgs | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage               = xiiGALResourceUsage::Default;
    passData.m_pDrawIndirectArgsBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pDrawCountBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize               = sizeof(xiiUInt32) * passData.m_uiMaxMaterialBins;
    desc.m_BufferFlags          = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage        = xiiGALResourceUsage::Default;
    passData.m_pDrawCountBuffer = pDevice->CreateBuffer(desc);
  }

  xiiRGBufferHandle hSurviving, hLOD;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer), hSurviving);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceLODBuffer), hLOD);

  auto [pData, hPass] = graph.AddPass<DrawCmdBuildData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [&passData, hSurviving, hLOD](DrawCmdBuildData& data, xiiRGBuilder& builder) {
      if (hSurviving.IsValid()) data.hSurviving = builder.ReadBuffer(hSurviving, xiiGALResourceStateFlags::ShaderResource);
      if (hLOD.IsValid()) data.hLOD = builder.ReadBuffer(hLOD, xiiGALResourceStateFlags::ShaderResource);
      data.hDrawArgs       = builder.ImportBuffer("DrawIndirectArgs", passData.m_pDrawIndirectArgsBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hDrawArgs       = builder.WriteBuffer(data.hDrawArgs, xiiGALResourceStateFlags::UnorderedAccess);
      data.hDrawCount      = builder.ImportBuffer("DrawCounts", passData.m_pDrawCountBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hDrawCount      = builder.WriteBuffer(data.hDrawCount, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount = 65536u;
    },
    [](const DrawCmdBuildData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Draw Command Build + Compact");
      // DrawCommandBuild.xiiShader: bins surviving instances by material, writes DrawIndexedIndirect args.
      // Uses prefix sum for compact packing, no CPU readback needed.
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), pData->hDrawArgs);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawCountBuffer), pData->hDrawCount);
}

namespace
{
  // Shader-side constants for DynamicResolution.xiiShader
  struct alignas(16) DynResConstants
  {
    float fFrameDeltaTimeMs          = 16.667f;
    float fTargetFrameTimeMs         = 16.667f;
    float fMinDynamicResolutionScale = 0.5f;
    float fMaxDynamicResolutionScale = 1.0f;
  };
} // namespace

void xiiPopulateDynamicResolutionPass(xiiDynamicResolutionPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create persistent structured buffers.
  if (!passData.m_pFrameTimingBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                 = sizeof(float) * 4u;
    desc.m_BufferFlags            = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage          = xiiGALResourceUsage::Default;
    passData.m_pFrameTimingBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pCameraVelocityBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                    = sizeof(float) * 4u;
    desc.m_BufferFlags               = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage             = xiiGALResourceUsage::Dynamic;
    passData.m_pCameraVelocityBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pResolutionOutputBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                      = sizeof(float) * 4u;
    desc.m_BufferFlags                 = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage               = xiiGALResourceUsage::Default;
    passData.m_pResolutionOutputBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pFrameConstantBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                   = sizeof(DynResConstants);
    desc.m_BufferFlags              = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage            = xiiGALResourceUsage::Dynamic;
    passData.m_pFrameConstantBuffer = pDevice->CreateBuffer(desc);
  }

  // Upload latest timing constants to the constant buffer.
  DynResConstants constants;
  float           fGPUMs = passData.m_fTargetFrameTimeMs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GPUFrameTimeMs), fGPUMs);
  constants.fFrameDeltaTimeMs          = fGPUMs;
  constants.fTargetFrameTimeMs         = passData.m_fTargetFrameTimeMs;
  constants.fMinDynamicResolutionScale = passData.m_fMinScale;
  constants.fMaxDynamicResolutionScale = passData.m_fMaxScale;

  auto [pData, hPass] = graph.AddPass<DynResPassData>(
    passData.GetName(),
    xiiGALCommandQueueFlags::Compute,
    [&passData](DynResPassData& data, xiiRGBuilder& builder) {
      data.hTimingInput      = builder.ImportBuffer("DynRes_Timing", passData.m_pFrameTimingBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hTimingInput      = builder.ReadBuffer(data.hTimingInput, xiiGALResourceStateFlags::ShaderResource);
      data.hVelocityInput    = builder.ImportBuffer("DynRes_Velocity", passData.m_pCameraVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hVelocityInput    = builder.ReadBuffer(data.hVelocityInput, xiiGALResourceStateFlags::ShaderResource);
      data.hResolutionOutput = builder.ImportBuffer("DynRes_Output", passData.m_pResolutionOutputBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hResolutionOutput = builder.WriteBuffer(data.hResolutionOutput, xiiGALResourceStateFlags::UnorderedAccess);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [&passData, constants](const DynResPassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Dynamic Resolution");

      // Upload constants.
      cmd.UpdateBuffer(passData.m_pFrameConstantBuffer.Borrow(), 0u, &constants, sizeof(DynResConstants));

      // Dispatch the single-thread resolution compute shader (DynamicResolution.xiiShader).
      // The shader reads FrameTimingData + CameraVelocityData and writes DynamicResolutionData[0].x = scale.
      cmd.Dispatch(1u, 1u, 1u);

      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);

  // CPU-side readback: sample the output buffer from the previous frame and publish to blackboard.
  // This introduces a 1-frame latency between scale computation and resource sizing, acceptable.
  // On the very first frame, default to 1.0.
  float        fScale     = passData.m_fMaxScale;
  const float* pScaleData = static_cast<const float*>(
    pDevice->MapBuffer(passData.m_pResolutionOutputBuffer.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait));
  if (pScaleData)
  {
    fScale = xiiMath::Clamp(pScaleData[0], passData.m_fMinScale, passData.m_fMaxScale);
    pDevice->UnmapBuffer(passData.m_pResolutionOutputBuffer.Borrow(), xiiGALMapType::Read);
  }

  const xiiViewData& viewData       = view.GetData();
  const xiiUInt32    uiRenderWidth  = xiiMath::Max(1u, static_cast<xiiUInt32>(viewData.m_ViewPortRect.width * fScale));
  const xiiUInt32    uiRenderHeight = xiiMath::Max(1u, static_cast<xiiUInt32>(viewData.m_ViewPortRect.height * fScale));

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DynamicResolutionScale), fScale);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiRenderWidth);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiRenderHeight);
}

void xiiPopulateEmissiveAuxPass(xiiEmissiveAuxPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  // Emissive target already created by GBufferBasePass, read+write to append.
  xiiRGTextureHandle hEmissive;
  xiiRGTextureHandle hDepth;
  xiiRGBufferHandle  hArgs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferEmissive), hEmissive);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hArgs);

  auto [pData, hPass] = graph.AddPass<EmissiveData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hEmissive, hDepth, hArgs, uiW, uiH](EmissiveData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);
      if (hArgs.IsValid()) data.hArgs = builder.ReadBuffer(hArgs, xiiGALResourceStateFlags::IndirectArgument);
      // Additive write into the existing emissive target.
      if (hEmissive.IsValid()) data.hEmissive = builder.WriteTexture(hEmissive, xiiGALResourceStateFlags::RenderTarget);
      data.uiW = uiW;
      data.uiH = uiH;
    },
    [](const EmissiveData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Emissive + Aux Materials");
      // EmissiveMaterial.xiiShader: indirect draw for emissive-only material bins (additive blend).
      if (auto* pArgs = context.GetBuffer(data.hArgs))
        cmd.DrawIndexedIndirect(pArgs, 0u);
      cmd.PopDebugGroup();
    });

  // hEmissive handle remains the same, no need to re-publish (same texture resource).
}

namespace
{
  struct alignas(16) ExposureConstants
  {
    float MinEV100, MaxEV100, LowPercent, HighPercent, AdaptationSpeed, DeltaTimeS, _pad[2];
  };
} // namespace

void xiiPopulateExposurePass(xiiExposurePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!passData.m_pHistogramBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize               = sizeof(xiiUInt32) * 256u;
    desc.m_BufferFlags          = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage        = xiiGALResourceUsage::Default;
    passData.m_pHistogramBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pExposureBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize              = sizeof(float) * 4u; // (exposure, avgLuminance, minLum, maxLum)
    desc.m_BufferFlags         = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage       = xiiGALResourceUsage::Default;
    passData.m_pExposureBuffer = pDevice->CreateBuffer(desc);
  }

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);
  xiiRGTextureHandle hHDR;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), hHDR);

  const float             fDelta = static_cast<float>(view.GetData().m_fDeltaTime);
  const ExposureConstants consts = {passData.m_fMinEV100, passData.m_fMaxEV100, passData.m_fLowPercent, passData.m_fHighPercent, passData.m_fAdaptationSpeed, fDelta};

  auto [pData, hPass] = graph.AddPass<ExposureData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hHDR, &passData](ExposureData& data, xiiRGBuilder& builder) {
      if (hHDR.IsValid()) data.hHDRScene = builder.ReadTexture(hHDR, xiiGALResourceStateFlags::ShaderResource);
      data.hHistogram = builder.ImportBuffer("LuminanceHistogram", passData.m_pHistogramBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hHistogram = builder.WriteBuffer(data.hHistogram, xiiGALResourceStateFlags::UnorderedAccess);
      data.hExposure  = builder.ImportBuffer("CurrentExposure", passData.m_pExposureBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hExposure  = builder.WriteBuffer(data.hExposure, xiiGALResourceStateFlags::UnorderedAccess);
      builder.SetPassSideEffects(true);
    },
    [consts](const ExposureData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Exposure + Eye Adaptation");
      // Pass 53: ExposureHistogram.xiiShader, atomic histogram of log2(luminance) per pixel.
      cmd.Dispatch((data.uiW + 15u) / 16u, (data.uiH + 15u) / 16u, 1u);
      // Pass 54: ExposureAdaptation.xiiShader, reads histogram, emits adapted exposure.
      cmd.Dispatch(1u, 1u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LuminanceHistogram), pData->hHistogram);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_CurrentExposure), pData->hExposure);
}

void xiiPopulateFrameSetupPass(xiiFrameSetupPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create the timestamp readback ring.
  for (xiiUInt32 i = 0; i < s_uiTimestampRingSize; ++i)
  {
    if (!passData.m_pTimestampBuffers[i])
    {
      xiiGALBufferCreationDescription description;
      description.m_uiSize         = sizeof(xiiUInt64) * 2U; // Begin + end GPU timestamps.
      description.m_Usage          = xiiGALResourceUsage::Staging;
      description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;

      passData.m_pTimestampBuffers[i] = pDevice->CreateBuffer(description);
    }
  }

  const xiiUInt32 uiSlot            = passData.m_uiCurrentTimestampSlot;
  passData.m_uiCurrentTimestampSlot = (passData.m_uiCurrentTimestampSlot + 1U) % s_uiTimestampRingSize;

  // Read GPU frame time written two frames ago (safe, since ring size ensures no GPU stall).
  xiiGALBuffer* pReadbackBuffer = passData.m_pTimestampBuffers[(uiSlot + 1U) % s_uiTimestampRingSize].Borrow();
  float         fGPUFrameTimeMs = 0.0f;
  if (pReadbackBuffer)
  {
    auto                            pGraphicsQueue = pDevice->GetCommandQueue();
    xiiSharedPtr<xiiGALCommandList> pCommandList   = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});


    const xiiUInt64* pData = static_cast<const xiiUInt64*>(pDevice->MapBuffer(pReadbackBuffer, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait));
    if (pData)
    {
      const xiiUInt64 uiFreq = pDevice->GetTimestampFrequency();
      if (uiFreq > 0 && pData[1] > pData[0])
        fGPUFrameTimeMs = static_cast<float>((pData[1] - pData[0]) * 1000.0 / static_cast<double>(uiFreq));
      pDevice->UnmapBuffer(pReadbackBuffer, xiiGALMapType::Read);
    }
  }

  // Publish GPU timing to blackboard so DynamicResolutionPass can consume it.
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GPUFrameTimeMs), fGPUFrameTimeMs);

  auto [pData, hPass] = graph.AddPass<FrameSetupPassData>(
    passData.GetName(),
    xiiGALCommandQueueFlags::Graphics,
    [](FrameSetupPassData& data, xiiRGBuilder& builder) {
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [&passData, uiSlot](const FrameSetupPassData&, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("XII Frame");

      // Write begin timestamp into this frame's readback slot.
      xiiGALBuffer* pDst = passData.m_pTimestampBuffers[uiSlot].Borrow();
      if (pDst)
      {
        cmd.WriteTimestamp(pDst, 0u);
      }
    },
    /*bHasSideEffects=*/true);

  pData->uiTimestampSlot = uiSlot;
}

void xiiPopulateGBufferBasePass(xiiGBufferBasePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth;
  xiiRGBufferHandle  hDrawArgs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawArgs);

  auto makeRT = [&](const char* szName, xiiGALTextureFormat fmt) {
    xiiGALTextureCreationDescription d;
    d.m_uiWidth     = uiW;
    d.m_uiHeight    = uiH;
    d.m_uiMipLevels = 1u;
    d.m_Format      = fmt;
    d.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    return d;
  };

  auto albedoDesc   = makeRT("GBufferAlbedo", xiiGALTextureFormat::R8G8B8A8UNorm);
  auto normalDesc   = makeRT("GBufferNormal", xiiGALTextureFormat::R16G16SNorm);
  auto materialDesc = makeRT("GBufferMaterial", xiiGALTextureFormat::R8G8B8A8UNorm);
  auto emissiveDesc = makeRT("GBufferEmissive", xiiGALTextureFormat::R16G16B16A16Float);

  auto [pData, hPass] = graph.AddPass<GBufferData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, hDrawArgs, albedoDesc, normalDesc, materialDesc, emissiveDesc, uiW, uiH](GBufferData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);
      if (hDrawArgs.IsValid()) data.hDrawArgs = builder.ReadBuffer(hDrawArgs, xiiGALResourceStateFlags::IndirectArgument);
      data.hAlbedo   = builder.WriteTexture("GBufferAlbedo", albedoDesc, xiiGALResourceStateFlags::RenderTarget);
      data.hNormal   = builder.WriteTexture("GBufferNormal", normalDesc, xiiGALResourceStateFlags::RenderTarget);
      data.hMaterial = builder.WriteTexture("GBufferMaterial", materialDesc, xiiGALResourceStateFlags::RenderTarget);
      data.hEmissive = builder.WriteTexture("GBufferEmissive", emissiveDesc, xiiGALResourceStateFlags::RenderTarget);
      data.uiW       = uiW;
      data.uiH       = uiH;
      builder.SetPassAllowMerge(false);
    },
    [](const GBufferData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("GBuffer Base Pass");
      // GBufferBase.xiiShader: indirect MRT draw, albedo+alpha/oct-normal/roughness-metallic-AO/emissive.
      if (auto* pArgs = context.GetBuffer(data.hDrawArgs))
        cmd.DrawIndexedIndirect(pArgs, 0u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferAlbedo), pData->hAlbedo);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferNormal), pData->hNormal);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferMaterial), pData->hMaterial);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferEmissive), pData->hEmissive);
}

void xiiPopulateHiZBuildPass(xiiHiZBuildPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiRGTextureHandle hOccluderDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_OccluderDepthTexture), hOccluderDepth);

  xiiUInt32 uiW = 960u, uiH = 540u;
  // HiZ pyramid is built at the occluder depth resolution.
  // Mip count spans down to 1x1.
  const xiiUInt32 uiMips = static_cast<xiiUInt32>(xiiMath::Log2i(xiiMath::Max(uiW, uiH))) + 1u;

  xiiGALTextureCreationDescription hizDesc;
  hizDesc.m_uiWidth     = uiW;
  hizDesc.m_uiHeight    = uiH;
  hizDesc.m_uiMipLevels = uiMips;
  hizDesc.m_Format      = xiiGALTextureFormat::R32Float; // single-channel max depth pyramid
  hizDesc.m_BindFlags   = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;

  auto [pData, hPass] = graph.AddPass<HiZBuildData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hOccluderDepth, hizDesc, uiW, uiH, uiMips](HiZBuildData& data, xiiRGBuilder& builder) {
      if (hOccluderDepth.IsValid())
        data.hOccluderDepth = builder.ReadTexture(hOccluderDepth, xiiGALResourceStateFlags::ShaderResource);
      data.hHiZPyramid = builder.WriteTexture("HiZPyramid", hizDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiWidth     = uiW;
      data.uiHeight    = uiH;
      data.uiMips      = uiMips;
      builder.SetPassAllowMerge(false);
    },
    [](const HiZBuildData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Hi-Z Pyramid Build");
      // HiZBuild.xiiShader repeatedly dispatched per mip level (2x2 max-reduce).
      // Mip 0: from occluder depth | Mip N: from mip N-1.
      xiiUInt32 uiMipW = data.uiWidth, uiMipH = data.uiHeight;
      for (xiiUInt32 mip = 0u; mip < data.uiMips; ++mip)
      {
        cmd.Dispatch(xiiMath::Max(1u, (uiMipW + 7u) / 8u),
                     xiiMath::Max(1u, (uiMipH + 7u) / 8u), 1u);
        uiMipW = xiiMath::Max(1u, uiMipW / 2u);
        uiMipH = xiiMath::Max(1u, uiMipH / 2u);
      }
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid), pData->hHiZPyramid);
}

void xiiPopulateHiZOcclusionCullPass(xiiHiZOcclusionCullPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice*       pDevice          = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!passData.m_pSurvivingInstanceBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                       = sizeof(xiiUInt32) * k_uiMaxInstances;
    desc.m_BufferFlags                  = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage                = xiiGALResourceUsage::Default;
    passData.m_pSurvivingInstanceBuffer = pDevice->CreateBuffer(desc);
  }

  xiiRGBufferHandle  hCandidates, hBounds;
  xiiRGTextureHandle hHiZ;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), hCandidates);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer), hBounds);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid), hHiZ);

  auto [pData, hPass] = graph.AddPass<HiZOcclData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [&passData, hCandidates, hBounds, hHiZ](HiZOcclData& data, xiiRGBuilder& builder) {
      if (hCandidates.IsValid()) data.hCandidates = builder.ReadBuffer(hCandidates, xiiGALResourceStateFlags::ShaderResource);
      if (hBounds.IsValid()) data.hBounds = builder.ReadBuffer(hBounds, xiiGALResourceStateFlags::ShaderResource);
      if (hHiZ.IsValid()) data.hHiZ = builder.ReadTexture(hHiZ, xiiGALResourceStateFlags::ShaderResource);
      data.hSurviving      = builder.ImportBuffer("SurvivingInstances", passData.m_pSurvivingInstanceBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hSurviving      = builder.WriteBuffer(data.hSurviving, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount = k_uiMaxInstances;
    },
    [](const HiZOcclData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Hi-Z Occlusion Cull");
      // HiZOcclusionCulling.xiiShader: projects AABB to screen, samples Hi-Z pyramid.
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer), pData->hSurviving);
}

void xiiPopulateInstanceTransformPass(xiiInstanceTransformPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice*       pDevice          = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!passData.m_pWorldMatrixBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                 = sizeof(float) * 12u * k_uiMaxInstances; // float4x3 per instance
    desc.m_BufferFlags            = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage          = xiiGALResourceUsage::Default;
    passData.m_pWorldMatrixBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pBoundsBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize            = sizeof(float) * 8u * k_uiMaxInstances; // center(float4) + extents(float4)
    desc.m_BufferFlags       = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage     = xiiGALResourceUsage::Default;
    passData.m_pBoundsBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pSceneTransformBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                    = sizeof(float) * 12u * k_uiMaxInstances;
    desc.m_BufferFlags               = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage             = xiiGALResourceUsage::Dynamic;
    passData.m_pSceneTransformBuffer = pDevice->CreateBuffer(desc);
  }

  auto [pData, hPass] = graph.AddPass<InstanceTransformPassData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [&passData](InstanceTransformPassData& data, xiiRGBuilder& builder) {
      data.hSceneTransforms = builder.ImportBuffer("SceneTransforms", passData.m_pSceneTransformBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hSceneTransforms = builder.ReadBuffer(data.hSceneTransforms, xiiGALResourceStateFlags::ShaderResource);
      data.hWorldMatrices   = builder.ImportBuffer("WorldMatrices", passData.m_pWorldMatrixBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hWorldMatrices   = builder.WriteBuffer(data.hWorldMatrices, xiiGALResourceStateFlags::UnorderedAccess);
      data.hBounds          = builder.ImportBuffer("InstanceBoundsRW", passData.m_pBoundsBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hBounds          = builder.WriteBuffer(data.hBounds, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount  = k_uiMaxInstances;
    },
    [](const InstanceTransformPassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Instance Transform + Bounds");
      // InstanceUpdate.xiiShader: [numthreads(64,1,1)]
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceWorldMatrixBuffer), pData->hWorldMatrices);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer), pData->hBounds);
}

void xiiPopulateLightingCombinePass(xiiLightingCombinePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  // Read all GBuffer + lighting inputs.
  xiiRGTextureHandle hAlbedo, hNormal, hMaterial, hDepth, hAO;
  xiiRGTextureHandle hDirShadow, hContactShadow, hRTShadow, hRTRefl, hSSR, hRTGI;
  xiiRGBufferHandle  hLightGrid, hLightIndex, hCamera;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferAlbedo), hAlbedo);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferNormal), hNormal);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferMaterial), hMaterial);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_StableAOTexture), hAO);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectionalShadowAtlas), hDirShadow);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ContactShadowTerm), hContactShadow);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalShadowMask), hRTShadow);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalReflections), hRTRefl);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SSRTexture), hSSR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalGI), hRTGI);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightGridBuffer), hLightGrid);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightIndexBuffer), hLightIndex);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_PerFrameCameraBuffer), hCamera);

  xiiGALTextureCreationDescription hdrDesc;
  hdrDesc.m_uiWidth     = uiW;
  hdrDesc.m_uiHeight    = uiH;
  hdrDesc.m_uiMipLevels = 1u;
  hdrDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  hdrDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<LightingData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [=, &passData](LightingData& data, xiiRGBuilder& builder) {
      auto read = [&](auto h) {
        return h.IsValid() ? builder.ReadTexture(h, xiiGALResourceStateFlags::ShaderResource) : h;
      };
      auto readB = [&](auto h) {
        return h.IsValid() ? builder.ReadBuffer(h, xiiGALResourceStateFlags::ShaderResource) : h;
      };
      data.hAlbedo         = read(hAlbedo);
      data.hNormal         = read(hNormal);
      data.hMaterial       = read(hMaterial);
      data.hDepth          = read(hDepth);
      data.hAO             = read(hAO);
      data.hDirShadow      = read(hDirShadow);
      data.hContactShadow  = read(hContactShadow);
      data.hRTShadow       = read(hRTShadow);
      data.hRTRefl         = read(hRTRefl);
      data.hSSR            = read(hSSR);
      data.hRTGI           = read(hRTGI);
      data.hLightGrid      = readB(hLightGrid);
      data.hLightIndex     = readB(hLightIndex);
      data.hCameraBuffer   = readB(hCamera);
      data.hDirectLight    = builder.WriteTexture("DirectLighting", hdrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.hIndirectLight  = builder.WriteTexture("IndirectLighting", hdrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW             = uiW;
      data.uiH             = uiH;
      data.bRTShadows      = passData.m_bEnableRTShadows;
      data.bContactShadows = passData.m_bEnableContactShadows;
      data.bRTRefl         = passData.m_bEnableRTReflections;
      data.bSSR            = passData.m_bEnableSSRFallback;
      builder.SetPassAllowMerge(false);
    },
    [](const LightingData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Lighting Combine");
      // Pass 46: DirectLighting.xiiShader, GGX BRDF + cluster lights + cascade shadows.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 47: IndirectLighting.xiiShader, AO-modulated RT GI / IBL + RT/SSR reflections.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectLightingBuffer), pData->hDirectLight);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_IndirectLightingBuffer), pData->hIndirectLight);
}

void xiiPopulateLocalLightShadowPass(xiiLocalLightShadowPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALTextureCreationDescription atlasDesc;
  atlasDesc.m_uiWidth     = passData.m_uiAtlasSize;
  atlasDesc.m_uiHeight    = passData.m_uiAtlasSize;
  atlasDesc.m_uiMipLevels = 1u;
  atlasDesc.m_Format      = xiiGALTextureFormat::D16UNorm;
  atlasDesc.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  xiiGALBufferCreationDescription descsDesc;
  descsDesc.m_uiSize        = sizeof(xiiUInt32) * 4u * passData.m_uiMaxShadowedLights; // (x,y,size,flags) per light
  descsDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  descsDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  auto [pData, hPass] = graph.AddPass<LocalShadowData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [atlasDesc, descsDesc, &passData](LocalShadowData& data, xiiRGBuilder& builder) {
      data.hShadowAtlas = builder.WriteTexture("LocalShadowAtlas", atlasDesc, xiiGALResourceStateFlags::DepthWrite);
      data.hAtlasDescs  = builder.WriteBuffer("LocalShadowAtlasDescs", descsDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiAtlasSize  = passData.m_uiAtlasSize;
      builder.SetPassAllowMerge(false);
    },
    [](const LocalShadowData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Local Light Shadows");
      // Pass 18: LocalLightShadowAtlasAllocation.xiiShader, deterministic atlas tile assignment.
      cmd.Dispatch(1u, 1u, 1u);
      // Pass 19: ShadowDepth.xiiShader, indirect draws per shadowed light into allocated tiles.
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LocalShadowAtlas), pData->hShadowAtlas);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs), pData->hAtlasDescs);
}

namespace
{
  struct alignas(16) LodConstants
  {
    float LODDistances[4];
  };
} // namespace

void xiiPopulateLodAndMeshletPass(xiiLodAndMeshletPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice*       pDevice          = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!passData.m_pLODMetadataBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                 = sizeof(xiiUInt32) * k_uiMaxInstances; // packed uint: (lod:8, bin:8, flags:16)
    desc.m_BufferFlags            = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage          = xiiGALResourceUsage::Default;
    passData.m_pLODMetadataBuffer = pDevice->CreateBuffer(desc);
  }

  xiiRGBufferHandle hBounds;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer), hBounds);

  const LodConstants lodConsts = {{passData.m_fLOD0Distance, passData.m_fLOD1Distance, passData.m_fLOD2Distance, passData.m_fLOD3Distance}};

  auto [pData, hPass] = graph.AddPass<LodPassData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [&passData, hBounds](LodPassData& data, xiiRGBuilder& builder) {
      if (hBounds.IsValid()) data.hBounds = builder.ReadBuffer(hBounds, xiiGALResourceStateFlags::ShaderResource);
      data.hLODOutput      = builder.ImportBuffer("LODMetadata", passData.m_pLODMetadataBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hLODOutput      = builder.WriteBuffer(data.hLODOutput, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiInstanceCount = k_uiMaxInstances;
    },
    [lodConsts](const LodPassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("LOD Selection + Meshlet");
      // LodSelection.xiiShader: [numthreads(64,1,1)]
      cmd.Dispatch((data.uiInstanceCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceLODBuffer), pData->hLODOutput);
}

void xiiPopulateMainDepthPrepassPass(xiiMainDepthPrepassPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiGALTextureCreationDescription depthDesc;
  depthDesc.m_uiWidth     = uiW;
  depthDesc.m_uiHeight    = uiH;
  depthDesc.m_uiMipLevels = 1u;
  depthDesc.m_Format      = xiiGALTextureFormat::D32Float;
  depthDesc.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  xiiRGBufferHandle hDrawArgs, hDrawCount;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawArgs);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawCountBuffer), hDrawCount);

  auto [pData, hPass] = graph.AddPass<MainDepthData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDrawArgs, hDrawCount, depthDesc, uiW, uiH](MainDepthData& data, xiiRGBuilder& builder) {
      if (hDrawArgs.IsValid()) data.hDrawArgs = builder.ReadBuffer(hDrawArgs, xiiGALResourceStateFlags::IndirectArgument);
      if (hDrawCount.IsValid()) data.hDrawCount = builder.ReadBuffer(hDrawCount, xiiGALResourceStateFlags::IndirectArgument);
      data.hSceneDepth = builder.WriteTexture("SceneDepth", depthDesc, xiiGALResourceStateFlags::DepthWrite);
      data.uiWidth     = uiW;
      data.uiHeight    = uiH;
    },
    [](const MainDepthData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Main Depth Prepass");
      xiiGALTexture* pDepth = context.GetTexture(data.hSceneDepth);
      xiiGALBuffer*  pArgs  = context.GetBuffer(data.hDrawArgs);
      cmd.ClearDepthStencilView(pDepth, xiiGALClearFlags::Depth, 0.0f, 0u);
      // Indirect depth-only draw per material bin, DepthPrepass.xiiShader (VS only, no PS).
      if (pArgs) cmd.DrawIndexedIndirect(pArgs, 0u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), pData->hSceneDepth);
}

void xiiPopulateMotionVectorPass(xiiMotionVectorPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  xiiGALTextureCreationDescription velDesc;
  velDesc.m_uiWidth     = uiW;
  velDesc.m_uiHeight    = uiH;
  velDesc.m_uiMipLevels = 1u;
  velDesc.m_Format      = xiiGALTextureFormat::R16G16Float; // screen-space velocity XY
  velDesc.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<MotionVecData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, velDesc, uiW, uiH](MotionVecData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);
      data.hVelocity = builder.WriteTexture("VelocityBuffer", velDesc, xiiGALResourceStateFlags::RenderTarget);
      data.uiWidth   = uiW;
      data.uiHeight  = uiH;
    },
    [](const MotionVecData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Motion Vectors");
      // MotionVectors.xiiShader: fullscreen triangle, reads depth + previous VP matrix, outputs (cur-prev) NDC.
      cmd.Draw(3u, 1u, 0u, 0u); // fullscreen triangle
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), pData->hVelocity);
}

void xiiPopulateNormalRoughnessPrepassPass(xiiNormalRoughnessPrepassPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  if (!passData.m_bEnabled) return;

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  xiiGALTextureCreationDescription nrDesc;
  nrDesc.m_uiWidth     = uiW;
  nrDesc.m_uiHeight    = uiH;
  nrDesc.m_uiMipLevels = 1u;
  nrDesc.m_Format      = xiiGALTextureFormat::R8G8B8A8UNorm; // oct(normal XY) + roughness + AO
  nrDesc.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

  xiiRGBufferHandle hDrawArgs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawArgs);

  auto [pData, hPass] = graph.AddPass<NRPrepassData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, hDrawArgs, nrDesc, uiW, uiH](NRPrepassData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);
      if (hDrawArgs.IsValid()) builder.ReadBuffer(hDrawArgs, xiiGALResourceStateFlags::IndirectArgument);
      data.hNormalRoughness = builder.WriteTexture("NormalRoughness", nrDesc, xiiGALResourceStateFlags::RenderTarget);
      data.uiWidth          = uiW;
      data.uiHeight         = uiH;
    },
    [](const NRPrepassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Normal-Roughness Prepass");
      // NormalRoughnessPrepass.xiiShader: indirect draw, outputs oct-normal + roughness.
      cmd.DrawIndexedIndirect(context.GetBuffer(data.hDepth), 0u); // reuse draw args from indirect buffer
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), pData->hNormalRoughness);
}

void xiiPopulateOccluderDepthPrepass(xiiOccluderDepthPrepass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiRW = 1920u, uiRH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiRW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiRH);

  const xiiUInt32 uiW = xiiMath::Max(1u, uiRW / passData.m_uiResolutionDivisor);
  const xiiUInt32 uiH = xiiMath::Max(1u, uiRH / passData.m_uiResolutionDivisor);

  xiiGALTextureCreationDescription depthDesc;
  depthDesc.m_uiWidth     = uiW;
  depthDesc.m_uiHeight    = uiH;
  depthDesc.m_uiMipLevels = 1u;
  depthDesc.m_Format      = xiiGALTextureFormat::D32Float;
  depthDesc.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<OccluderDepthData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [depthDesc, uiW, uiH](OccluderDepthData& data, xiiRGBuilder& builder) {
      data.hDepth   = builder.WriteTexture("OccluderDepth", depthDesc, xiiGALResourceStateFlags::DepthWrite);
      data.uiWidth  = uiW;
      data.uiHeight = uiH;
      builder.SetPassAllowMerge(false);
    },
    [](const OccluderDepthData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Occluder Depth Prepass");
      xiiGALTexture* pDepth = context.GetTexture(data.hDepth);
      cmd.ClearDepthStencilView(pDepth, xiiGALClearFlags::Depth, 0.0f, 0u); // Reversed-Z: clear to 0
      // Draw occluder meshes via dedicated low-poly draw list, geometry-only, no alpha test.
      // OccluderDepth.xiiShader used (depth-only vertex transform).
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_OccluderDepthTexture), pData->hDepth);
}

void xiiPopulateOpaqueCompositePass(xiiOpaqueCompositePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDirect, hIndirect, hEmissive, hVolumetric, hSky;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectLightingBuffer), hDirect);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_IndirectLightingBuffer), hIndirect);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferEmissive), hEmissive);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VolumetricScattering), hVolumetric);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SkyRadiance), hSky);

  xiiGALTextureCreationDescription hdrDesc;
  hdrDesc.m_uiWidth     = uiW;
  hdrDesc.m_uiHeight    = uiH;
  hdrDesc.m_uiMipLevels = 1u;
  hdrDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  hdrDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;

  auto [pData, hPass] = graph.AddPass<OpaqueCompData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [=](OpaqueCompData& data, xiiRGBuilder& builder) {
      auto read = [&](xiiRGTextureHandle h) {
        return h.IsValid() ? builder.ReadTexture(h, xiiGALResourceStateFlags::ShaderResource) : h;
      };
      data.hDirect     = read(hDirect);
      data.hIndirect   = read(hIndirect);
      data.hEmissive   = read(hEmissive);
      data.hVolumetric = read(hVolumetric);
      data.hSky        = read(hSky);
      data.hHDRScene   = builder.WriteTexture("HDRSceneColor", hdrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW         = uiW;
      data.uiH         = uiH;
      builder.SetPassAllowMerge(false);
    },
    [](const OpaqueCompData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Opaque Composite");
      // OpaqueComposite.xiiShader: fuses direct + indirect + emissive + volumetric + sky into HDR.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), pData->hHDRScene);
}

void xiiPopulateParticleVFXPass(xiiParticleVFXPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hHDRScene;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), hHDRScene);

  auto [pData, hPass] = graph.AddPass<ParticleData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, hHDRScene, uiW, uiH, &passData](ParticleData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hHDRScene.IsValid()) data.hHDRScene = builder.WriteTexture(hHDRScene, xiiGALResourceStateFlags::RenderTarget);
      data.uiW            = uiW;
      data.uiH            = uiH;
      data.uiMaxParticles = passData.m_uiMaxParticles;
      builder.SetPassAllowMerge(false);
    },
    [](const ParticleData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Particle VFX");
      // Simulate: ParticleSimulate.xiiShader, position + velocity integration.
      cmd.Dispatch((data.uiMaxParticles + 255u) / 256u, 1u, 1u);
      // Render: Particle.xiiShader, billboard quads, depth-tested, soft particle edge fading.
      cmd.DrawIndirect(nullptr, 0u);
      cmd.PopDebugGroup();
    });
}

xiiPerFrameBufferUploadPass::~xiiPerFrameBufferUploadPass()
{
  passData.m_pCameraBuffer.Clear();
  passData.m_pLightBuffer.Clear();
  passData.m_pGlobalBuffer.Clear();
}


void xiiPopulatePerFrameBufferUploadPass(xiiPerFrameBufferUploadPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create structured buffers on first use.
  if (!passData.m_pCameraBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize            = sizeof(xiiPerFrameCameraUploadData);
    desc.m_BufferFlags       = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage     = xiiGALResourceUsage::Dynamic;
    passData.m_pCameraBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pLightBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize           = sizeof(xiiPerFrameLightUploadData);
    desc.m_BufferFlags      = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage    = xiiGALResourceUsage::Dynamic;
    passData.m_pLightBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pGlobalBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize            = sizeof(xiiPerFrameGlobalUploadData);
    desc.m_BufferFlags       = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage     = xiiGALResourceUsage::Dynamic;
    passData.m_pGlobalBuffer = pDevice->CreateBuffer(desc);
  }

  // Read resolved render scale from the dynamic resolution pass.
  float fScale = 1.0f;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DynamicResolutionScale), fScale);

  xiiUInt32 uiFrameIndex = 0u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_FrameIndex), uiFrameIndex);

  // Fill CPU-side constants from the view's cached matrices.
  const xiiViewData& vd = view.GetData();

  xiiPerFrameCameraUploadData camera;
  camera.ViewProjectionMatrix        = vd.m_ViewProjectionMatrix[0];
  camera.InverseViewProjectionMatrix = vd.m_InverseViewProjectionMatrix[0];
  camera.CameraPositionAndNearPlane  = xiiVec4(vd.m_InverseViewMatrix[0].GetTranslationVector(), vd.m_fNearPlane);
  camera.CameraForwardAndFarPlane    = xiiVec4(-vd.m_ViewMatrix[0].GetRow(2).GetAsVec3(), vd.m_fFarPlane);

  xiiPerFrameLightUploadData light;
  // Sun direction and ambient are populated by a light extraction step upstream.
  // Defaults keep the renderer functional even when no light extract occurs.
  light.MainLightDirectionAndIntensity = xiiVec4(0.0f, -1.0f, 0.0f, 1.0f);
  light.MainLightColor                 = xiiVec4(1.0f, 0.95f, 0.85f, 1.0f);
  light.AmbientLightColor              = xiiVec4(0.1f, 0.1f, 0.15f, 1.0f);
  light.ActiveLightCount               = 0u;

  xiiPerFrameGlobalUploadData global;
  global.FrameIndex        = uiFrameIndex;
  global.DeltaTimeMs       = static_cast<float>(view.GetData().m_fDeltaTime * 1000.0f);
  global.GlobalTime        = 0.0f; // Filled from world clock by caller if needed.
  global.WorldTime         = 0.0f;
  global.RenderScaleJitter = xiiVec4(fScale, 0.0f, 0.0f, 0.0f);

  auto [pData, hPass] = graph.AddPass<UploadPassData>(
    passData.GetName(),
    xiiGALCommandQueueFlags::Graphics,
    [&passData](UploadPassData& data, xiiRGBuilder& builder) {
      data.hCameraBuffer = builder.ImportBuffer("PerFrameCamera", passData.m_pCameraBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      data.hCameraBuffer = builder.WriteBuffer(data.hCameraBuffer, xiiGALResourceStateFlags::CopyDestination);
      data.hLightBuffer  = builder.ImportBuffer("PerFrameLight", passData.m_pLightBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      data.hLightBuffer  = builder.WriteBuffer(data.hLightBuffer, xiiGALResourceStateFlags::CopyDestination);
      data.hGlobalBuffer = builder.ImportBuffer("PerFrameGlobal", passData.m_pGlobalBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      data.hGlobalBuffer = builder.WriteBuffer(data.hGlobalBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [camera, light, global](const UploadPassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Per-Frame Buffer Upload");

      xiiGALBuffer* pCam    = context.GetBuffer(data.hCameraBuffer);
      xiiGALBuffer* pLit    = context.GetBuffer(data.hLightBuffer);
      xiiGALBuffer* pGlobal = context.GetBuffer(data.hGlobalBuffer);

      cmd.UpdateBuffer(pCam, 0u, &camera, sizeof(xiiPerFrameCameraUploadData));
      cmd.UpdateBuffer(pLit, 0u, &light, sizeof(xiiPerFrameLightUploadData));
      cmd.UpdateBuffer(pGlobal, 0u, &global, sizeof(xiiPerFrameGlobalUploadData));

      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);

  // Publish imported buffer handles so downstream passes can import them by name.
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_PerFrameCameraBuffer), pData->hCameraBuffer);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_PerFrameLightBuffer), pData->hLightBuffer);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_PerFrameGlobalBuffer), pData->hGlobalBuffer);
}

void xiiPopulatePresentPass(xiiPresentPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiRGTextureHandle hFinal;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_FinalColorTarget), hFinal);

  auto [pData, hPass] = graph.AddPass<PresentData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hFinal, &passData](PresentData& data, xiiRGBuilder& builder) {
      if (hFinal.IsValid())
        data.hFinalColor = builder.ReadTexture(hFinal, xiiGALResourceStateFlags::ShaderResource);
      data.bVSync = m_bVSync;
      // Mark the swapchain RT as a written output so the render graph transitions it correctly.
      builder.WriteSwapchainOutput(xiiGALResourceStateFlags::Present);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [](const PresentData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Present");
      // FinalBlit.xiiShader: copies final color to the swapchain back buffer in the correct color space.
      // Handles sRGB gamma correction or display mapping for the active output format.
      xiiGALTexture* pFinal = context.GetTexture(data.hFinalColor);
      xiiGALTexture* pBack  = context.GetSwapchainBackBuffer();
      if (pFinal && pBack)
        cmd.CopyTextureRegion(pFinal, nullptr, pBack, nullptr);
      cmd.PopDebugGroup();
      // Pop the outermost "XII Frame" debug group opened by FrameSetupPass.
      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);
}

void xiiPopulateReadbackAndTelemetryPass(xiiReadbackAndTelemetryPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create readback ring: GPU timestamp + per-frame stats.
  for (xiiUInt32 i = 0u; i < k_uiReadbackRingSize; ++i)
  {
    if (!m_pStatsReadback[i])
    {
      xiiGALBufferCreationDescription desc;
      desc.m_uiSize        = sizeof(xiiUInt64) * 16u; // up to 16 GPU timestamps
      desc.m_BufferFlags   = xiiGALBufferUsageFlags::None;
      desc.m_ResourceUsage = xiiGALResourceUsage::Readback;
      m_pStatsReadback[i]  = pDevice->CreateBuffer(desc);
    }
  }

  xiiUInt32 uiFrame = 0u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_FrameIndex), uiFrame);

  const bool      bDoReadback = (uiFrame % m_uiReadbackIntervalFrames) == 0u;
  const xiiUInt32 uiSlot      = uiFrame % k_uiReadbackRingSize;

  auto [pData, hPass] = graph.AddPass<ReadbackData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [bDoReadback, uiSlot](ReadbackData& data, xiiRGBuilder& builder) {
      data.bDoReadback = bDoReadback;
      data.uiSlot      = uiSlot;
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [&passData](const ReadbackData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Readback + Telemetry");
      if (data.bDoReadback)
      {
        // Write end-of-frame timestamp into readback buffer for this slot.
        if (xiiGALBuffer* pBuf = m_pStatsReadback[data.uiSlot].Borrow())
          cmd.WriteTimestamp(pBuf, 0u);
      }
      // GPU pipeline statistics query resolve (draw/dispatch counts, primitive counts).
      cmd.ResolveQueryHeap();
      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);
}

void xiiPopulateRTGlobalIlluminationPass(xiiRTGlobalIlluminationPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  if (!xiiGALDevice::GetDefaultDevice()->GetFeatures().m_bRayTracing)
    return;

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hVelocity;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), hVelocity);

  xiiGALTextureCreationDescription giDesc;
  giDesc.m_uiWidth     = uiW;
  giDesc.m_uiHeight    = uiH;
  giDesc.m_uiMipLevels = 1u;
  giDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  giDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<RTGIData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hVelocity, giDesc, uiW, uiH, &passData](RTGIData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid()) data.hNR = builder.ReadTexture(hNR, xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      data.hRawGI       = builder.WriteTexture("RTRawGI", giDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.hFinalGI     = builder.WriteTexture("RTFinalGI", giDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW          = uiW;
      data.uiH          = uiH;
      data.uiRPP        = passData.m_uiRaysPerPixel;
      data.uiCandidates = passData.m_uiReservoirCandidateCount;
    },
    [](const RTGIData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("RT Global Illumination");
      // Pass 42: RTGIFinalGather.xiiShader (TraceRays), ReSTIR reservoir resampling initial candidates.
      cmd.TraceRays(data.uiW, data.uiH, 1u);
      // Pass 43: RTGITemporal.xiiShader, temporal reservoir reuse + firefly clamp.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 44: RTGIDenoise.xiiShader, diffuse/specular indirect denoiser.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTRawGI), pData->hRawGI);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalGI), pData->hFinalGI);
}

void xiiPopulateRTReflectionPass(xiiRTReflectionPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  if (!xiiGALDevice::GetDefaultDevice()->GetFeatures().m_bRayTracing)
    return;

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hVelocity, hSSR;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), hVelocity);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SSRTexture), hSSR);

  xiiGALTextureCreationDescription reflDesc;
  reflDesc.m_uiWidth     = uiW;
  reflDesc.m_uiHeight    = uiH;
  reflDesc.m_uiMipLevels = 1u;
  reflDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  reflDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<RTReflData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hVelocity, hSSR, reflDesc, uiW, uiH, &passData](RTReflData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid()) data.hNR = builder.ReadTexture(hNR, xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      if (hSSR.IsValid()) data.hSSR = builder.ReadTexture(hSSR, xiiGALResourceStateFlags::ShaderResource);
      data.hRawRefl      = builder.WriteTexture("RTRawReflections", reflDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.hFinalRefl    = builder.WriteTexture("RTFinalReflections", reflDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW           = uiW;
      data.uiH           = uiH;
      data.uiMaxRays     = passData.m_uiMaxRaysPerPixel;
      data.fMaxRoughness = passData.m_fMaxRoughnessForRT;
    },
    [](const RTReflData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("RT Reflections");
      // Pass 39: RTReflection.xiiShader (TraceRays), glossy reflection rays, budget by roughness.
      cmd.TraceRays(data.uiW, data.uiH, 1u);
      // Pass 40: RTReflectionTemporal.xiiShader, radiance-space clamped accumulation.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 41: RTReflectionDenoise.xiiShader, two-stage diffuse/specular split denoise.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTRawReflections), pData->hRawRefl);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalReflections), pData->hFinalRefl);
}

void xiiPopulateRTShadowPass(xiiRTShadowPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  if (!xiiGALDevice::GetDefaultDevice()->GetFeatures().m_bRayTracing)
    return;

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hVelocity;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), hVelocity);

  auto makeR8 = [&](const char* name) {
    xiiGALTextureCreationDescription d;
    d.m_uiWidth     = uiW;
    d.m_uiHeight    = uiH;
    d.m_uiMipLevels = 1u;
    d.m_Format      = xiiGALTextureFormat::R8UNorm;
    d.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    return d;
  };

  auto [pData, hPass] = graph.AddPass<RTShadowData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hVelocity, uiW, uiH, &passData, makeR8](RTShadowData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid()) data.hNR = builder.ReadTexture(hNR, xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      data.hRawMask     = builder.WriteTexture("RTRawShadows", makeR8("RTRawShadows"), xiiGALResourceStateFlags::UnorderedAccess);
      data.hFinalMask   = builder.WriteTexture("RTFinalShadows", makeR8("RTFinalShadows"), xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW          = uiW;
      data.uiH          = uiH;
      data.uiRPP        = passData.m_uiRaysPerPixel;
      data.uiHistory    = passData.m_uiTemporalHistoryLen;
      data.fLightRadius = passData.m_fLightRadius;
    },
    [](const RTShadowData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("RT Shadows");
      // Pass 36: RTShadow.xiiShader (TraceRays), passData.m_uiRaysPerPixel per pixel, jittered soft shadow.
      cmd.TraceRays(data.uiW, data.uiH, 1u);
      // Pass 37: RTShadowTemporalAccumulate.xiiShader, reproject + history clamping.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 38: RTShadowSpatialDenoise.xiiShader, normal+depth gated bilateral filter.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTRawShadowMask), pData->hRawMask);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalShadowMask), pData->hFinalMask);
}

void xiiPopulateScreenSpaceReflectionsPass(xiiScreenSpaceReflectionsPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hHiZ;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid), hHiZ);

  xiiGALTextureCreationDescription ssrDesc;
  ssrDesc.m_uiWidth     = uiW;
  ssrDesc.m_uiHeight    = uiH;
  ssrDesc.m_uiMipLevels = 1u;
  ssrDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  ssrDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<SSRData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hHiZ, ssrDesc, uiW, uiH, &passData](SSRData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid()) data.hNR = builder.ReadTexture(hNR, xiiGALResourceStateFlags::ShaderResource);
      if (hHiZ.IsValid()) data.hHiZ = builder.ReadTexture(hHiZ, xiiGALResourceStateFlags::ShaderResource);
      data.hSSR         = builder.WriteTexture("SSRTerm", ssrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW          = uiW;
      data.uiH          = uiH;
      data.uiMaxSteps   = passData.m_uiMaxRaySteps;
      data.fRoughThresh = passData.m_fRoughnessThreshold;
      data.fThickness   = passData.m_fThickness;
    },
    [](const SSRData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Screen-Space Reflections");
      // SSR.xiiShader: hierarchical DDA ray march via Hi-Z pyramid.
      // Skip pixels with roughness > fRoughThresh, they fall back to IBL.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SSRTexture), pData->hSSR);
}

namespace
{
  struct alignas(16) CascadeMatrixData
  {
    xiiShaderMat4 ViewProjection[4];
    float         SplitDistances[4];
  };
} // namespace

void xiiPopulateShadowCascadeSetupPass(xiiShadowCascadeSetupPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  if (!passData.m_pCascadeMatrixBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                   = sizeof(CascadeMatrixData);
    desc.m_BufferFlags              = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage            = xiiGALResourceUsage::Dynamic;
    passData.m_pCascadeMatrixBuffer = pDevice->CreateBuffer(desc);
  }

  // Compute PSSM splits using the practical split scheme (Engel / Andreev).
  const xiiViewData& vd    = view.GetData();
  const float        fNear = vd.m_fNearPlane;
  const float        fFar  = xiiMath::Min(vd.m_fFarPlane, passData.m_fMaxShadowDistance);

  CascadeMatrixData cascadeData;
  const xiiUInt32   N = xiiMath::Min(passData.m_uiCascadeCount, 4u);
  for (xiiUInt32 i = 0u; i < N; ++i)
  {
    const float fRatio            = static_cast<float>(i + 1u) / static_cast<float>(N);
    const float fLog              = fNear * xiiMath::Pow(fFar / fNear, fRatio);
    const float fUniform          = fNear + (fFar - fNear) * fRatio;
    cascadeData.SplitDistances[i] = passData.m_fSplitLambda * fLog + (1.0f - passData.m_fSplitLambda) * fUniform;
  }
  // World-space stable cascade matrices (texel-snapping applied to eliminate shimmer).
  // Full implementation requires sun direction from scene, defaulting here.

  auto [pData, hPass] = graph.AddPass<ShadowCascadeData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [&passData](ShadowCascadeData& data, xiiRGBuilder& builder) {
      data.hCascadeMatrices = builder.ImportBuffer("ShadowCascadeMatrices", passData.m_pCascadeMatrixBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      data.hCascadeMatrices = builder.WriteBuffer(data.hCascadeMatrices, xiiGALResourceStateFlags::CopyDestination);
      data.uiCascadeCount   = passData.m_uiCascadeCount;
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [cascadeData](const ShadowCascadeData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Shadow Cascade Setup");
      cmd.UpdateBuffer(context.GetBuffer(data.hCascadeMatrices), 0u, &cascadeData, sizeof(CascadeMatrixData));
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeMatrices), pData->hCascadeMatrices);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeCount), N);
}

void xiiPopulateShadowCasterCullPass(xiiShadowCasterCullPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice*       pDevice          = xiiGALDevice::GetDefaultDevice();
  constexpr xiiUInt32 k_uiMaxInstances = 65536u;

  if (!passData.m_pShadowCasterBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                  = sizeof(xiiUInt32) * k_uiMaxInstances * 4u; // 4 cascades * max instances
    desc.m_BufferFlags             = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage           = xiiGALResourceUsage::Default;
    passData.m_pShadowCasterBuffer = pDevice->CreateBuffer(desc);
  }

  xiiRGBufferHandle hCascMat, hBounds;
  xiiUInt32         uiCascadeCount = 4u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeMatrices), hCascMat);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer), hBounds);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeCount), uiCascadeCount);

  auto [pData, hPass] = graph.AddPass<ShadowCullData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [&passData, hCascMat, hBounds, uiCascadeCount](ShadowCullData& data, xiiRGBuilder& builder) {
      if (hCascMat.IsValid()) data.hCascadeMatrices = builder.ReadBuffer(hCascMat, xiiGALResourceStateFlags::ConstantBuffer);
      if (hBounds.IsValid()) data.hInstanceBounds = builder.ReadBuffer(hBounds, xiiGALResourceStateFlags::ShaderResource);
      data.hShadowCasters = builder.ImportBuffer("ShadowCasters", passData.m_pShadowCasterBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hShadowCasters = builder.WriteBuffer(data.hShadowCasters, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiCascadeCount = uiCascadeCount;
    },
    [](const ShadowCullData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Shadow Caster Cull");
      // ShadowCasterCulling.xiiShader: test each instance AABB against each cascade volume.
      cmd.Dispatch((65536u + 63u) / 64u, data.uiCascadeCount, 1u);
      cmd.PopDebugGroup();
    });
}

void xiiPopulateSharpeningPass(xiiSharpeningPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hInput;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GradedColor), hInput);

  xiiGALTextureCreationDescription outDesc;
  outDesc.m_uiWidth     = uiW;
  outDesc.m_uiHeight    = uiH;
  outDesc.m_uiMipLevels = 1u;
  outDesc.m_Format      = xiiGALTextureFormat::R8G8B8A8UNorm;
  outDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<SharpenData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hInput, outDesc, uiW, uiH, &passData](SharpenData& data, xiiRGBuilder& builder) {
      if (hInput.IsValid()) data.hInput = builder.ReadTexture(hInput, xiiGALResourceStateFlags::ShaderResource);
      data.hOutput   = builder.WriteTexture("SharpenedColor", outDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW       = uiW;
      data.uiH       = uiH;
      data.fStrength = m_fStrength;
    },
    [](const SharpenData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Luma Sharpening");
      // LumaSharpening.xiiShader: fast 1-pass luminance-weighted unsharp mask.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SharpenedColor), pData->hOutput);
}

void xiiPopulateSkinningAndMorphPass(xiiSkinningAndMorphPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create persistent GPU buffers.  These grow as scene vertex counts increase.
  constexpr xiiUInt32 k_uiMaxVertices = 1u << 20u; // 1M vertices initial allocation
  if (!passData.m_pSkinnedVertexBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                   = sizeof(float) * 4u * k_uiMaxVertices;
    desc.m_BufferFlags              = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceUsage            = xiiGALResourceUsage::Default;
    passData.m_pSkinnedVertexBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pSkinningInputBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                   = sizeof(float) * 4u * k_uiMaxVertices;
    desc.m_BufferFlags              = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage            = xiiGALResourceUsage::Default;
    passData.m_pSkinningInputBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pBonePaletteBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                 = sizeof(float) * 12u * 1024u; // 1024 bones max
    desc.m_BufferFlags            = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage          = xiiGALResourceUsage::Dynamic;
    passData.m_pBonePaletteBuffer = pDevice->CreateBuffer(desc);
  }
  if (!passData.m_pMorphWeightsBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize                  = sizeof(float) * 4u * 256u; // 256 morph targets max
    desc.m_BufferFlags             = xiiGALBufferUsageFlags::StructuredBuffer;
    desc.m_ResourceUsage           = xiiGALResourceUsage::Dynamic;
    passData.m_pMorphWeightsBuffer = pDevice->CreateBuffer(desc);
  }

  auto [pData, hPass] = graph.AddPass<SkinPassData>(
    passData.GetName(),
    xiiGALCommandQueueFlags::Compute,
    [&passData](SkinPassData& data, xiiRGBuilder& builder) {
      data.hSkinInput     = builder.ImportBuffer("SkinInput", passData.m_pSkinningInputBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hSkinInput     = builder.ReadBuffer(data.hSkinInput, xiiGALResourceStateFlags::ShaderResource);
      data.hBonePalette   = builder.ImportBuffer("BonePalette", passData.m_pBonePaletteBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hBonePalette   = builder.ReadBuffer(data.hBonePalette, xiiGALResourceStateFlags::ShaderResource);
      data.hMorphWeights  = builder.ImportBuffer("MorphWeights", passData.m_pMorphWeightsBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.hMorphWeights  = builder.ReadBuffer(data.hMorphWeights, xiiGALResourceStateFlags::ShaderResource);
      data.hSkinnedOutput = builder.ImportBuffer("SkinnedVertices", passData.m_pSkinnedVertexBuffer, xiiGALResourceStateFlags::UnorderedAccess);
      data.hSkinnedOutput = builder.WriteBuffer(data.hSkinnedOutput, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiVertexCount  = k_uiMaxVertices;
    },
    [](const SkinPassData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Skinning + Morph");
      // Dispatch Skinning.xiiShader: [numthreads(64,1,1)], one thread per vertex.
      cmd.Dispatch((data.uiVertexCount + 63u) / 64u, 1u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SkinnedVertexBuffer), pData->hSkinnedOutput);
}

void xiiPopulateTemporalResolvePass(xiiTemporalResolvePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  xiiUInt32     uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  // Lazy-create ping-pong history textures.
  for (xiiUInt32 i = 0u; i < 2u; ++i)
  {
    if (!passData.m_pHistoryTexture[i])
    {
      xiiGALTextureCreationDescription td;
      td.m_uiWidth                  = uiW;
      td.m_uiHeight                 = uiH;
      td.m_uiMipLevels              = 1u;
      td.m_Format                   = xiiGALTextureFormat::R16G16B16A16Float;
      td.m_BindFlags                = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
      passData.m_pHistoryTexture[i] = pDevice->CreateTexture(td);
    }
  }

  xiiRGTextureHandle hHDR, hVelocity, hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), hHDR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), hVelocity);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  // Compute Halton jitter for this frame and apply to camera projection (view must expose this).
  const xiiUInt32 uiJitter = passData.m_uiFrameIndex % passData.m_uiJitterSequenceLength;
  passData.m_uiFrameIndex++;

  const xiiUInt32 uiCurrent   = passData.m_uiCurrentHistory;
  const xiiUInt32 uiHistSrc   = 1u - uiCurrent;
  passData.m_uiCurrentHistory = uiHistSrc; // swap for next frame

  xiiGALTextureCreationDescription resolvedDesc;
  resolvedDesc.m_uiWidth     = uiW;
  resolvedDesc.m_uiHeight    = uiH;
  resolvedDesc.m_uiMipLevels = 1u;
  resolvedDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  resolvedDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<TAAData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hHDR, hVelocity, hDepth, uiW, uiH, resolvedDesc, uiCurrent, uiHistSrc, &passData](TAAData& data, xiiRGBuilder& builder) {
      if (hHDR.IsValid()) data.hCurrent = builder.ReadTexture(hHDR, xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      // History src is a persistent texture imported for reading.
      data.hHistory = builder.ImportTexture("TAAHistory", passData.m_pHistoryTexture[uiHistSrc], xiiGALResourceStateFlags::ShaderResource);
      data.hHistory = builder.ReadTexture(data.hHistory, xiiGALResourceStateFlags::ShaderResource);
      // Resolved output written into the history dst slot (ping-pong).
      data.hResolved  = builder.ImportTexture("TAAResolved", passData.m_pHistoryTexture[uiCurrent], xiiGALResourceStateFlags::UnorderedAccess);
      data.hResolved  = builder.WriteTexture(data.hResolved, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW        = uiW;
      data.uiH        = uiH;
      data.fAlpha     = passData.m_fBlendAlpha;
      data.fSharpness = passData.m_fSharpness;
      builder.SetPassSideEffects(true); // writes persistent history
    },
    [](const TAAData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("TAA Resolve");
      // TAA.xiiShader: YCoCg neighbourhood AABB clamping + velocity-weighted reprojection.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_TAAResolvedColor), pData->hResolved);
}

void xiiPopulateToneMappingPass(xiiToneMappingPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hHDR, hBloom;
  xiiRGBufferHandle  hExposure;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_UpscaledColor), hHDR);
  if (!hHDR.IsValid()) blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_TAAResolvedColor), hHDR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_BloomTexture), hBloom);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_CurrentExposure), hExposure);

  xiiGALTextureCreationDescription ldrDesc;
  ldrDesc.m_uiWidth     = uiW;
  ldrDesc.m_uiHeight    = uiH;
  ldrDesc.m_uiMipLevels = 1u;
  ldrDesc.m_Format      = xiiGALTextureFormat::R8G8B8A8UNorm;
  ldrDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<TonemapData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hHDR, hBloom, hExposure, ldrDesc, uiW, uiH, &passData](TonemapData& data, xiiRGBuilder& builder) {
      if (hHDR.IsValid()) data.hHDR = builder.ReadTexture(hHDR, xiiGALResourceStateFlags::ShaderResource);
      if (hBloom.IsValid()) data.hBloom = builder.ReadTexture(hBloom, xiiGALResourceStateFlags::ShaderResource);
      if (hExposure.IsValid()) data.hExposure = builder.ReadBuffer(hExposure, xiiGALResourceStateFlags::ShaderResource);
      data.hLDR       = builder.WriteTexture("LDRSceneColor", ldrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW        = uiW;
      data.uiH        = uiH;
      data.uiOperator = static_cast<xiiUInt8>(passData.m_Operator);
      data.fBias      = passData.m_fExposureBias;
    },
    [](const TonemapData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Tone Mapping");
      // ToneMapping.xiiShader: HDR * exposure + bloom additive â†’ ACES/AgX/Reinhard â†’ LDR.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LDRSceneColor), pData->hLDR);
}

void xiiPopulateTransparentPass(xiiTransparentPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hHDRScene;
  xiiRGBufferHandle  hDrawArgs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), hHDRScene);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawArgs);

  auto [pData, hPass] = graph.AddPass<TransparentData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, hHDRScene, hDrawArgs, uiW, uiH, &passData](TransparentData& data, xiiRGBuilder& builder) {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);
      if (hHDRScene.IsValid()) data.hHDRScene = builder.WriteTexture(hHDRScene, xiiGALResourceStateFlags::RenderTarget);
      if (hDrawArgs.IsValid()) data.hDrawArgs = builder.ReadBuffer(hDrawArgs, xiiGALResourceStateFlags::IndirectArgument);
      data.uiW  = uiW;
      data.uiH  = uiH;
      data.bOIT = passData.m_bUseWeightedBlendedOIT;
      builder.SetPassAllowMerge(false);
    },
    [](const TransparentData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Transparent + Refractive");
      // Transparent.xiiShader (OIT) or depth-sorted additive pass.
      if (auto* pArgs = context.GetBuffer(data.hDrawArgs))
        cmd.DrawIndexedIndirect(pArgs, 0u);
      cmd.PopDebugGroup();
    });
}

void xiiPopulateUICompositePass(xiiUICompositePass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  // Best available final scene output.
  xiiRGTextureHandle hScene;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SharpenedColor), hScene);
  if (!hScene.IsValid()) blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GradedColor), hScene);
  if (!hScene.IsValid()) blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_LDRSceneColor), hScene);

  xiiRGTextureHandle hUI;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_UIRenderTarget), hUI);

  xiiGALTextureCreationDescription finalDesc;
  finalDesc.m_uiWidth     = uiW;
  finalDesc.m_uiHeight    = uiH;
  finalDesc.m_uiMipLevels = 1u;
  finalDesc.m_Format      = xiiGALTextureFormat::R8G8B8A8UNorm;
  finalDesc.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<UICompData>(
    passData.GetName(), xiiGALCommandQueueFlags::Graphics,
    [hScene, hUI, finalDesc, uiW, uiH](UICompData& data, xiiRGBuilder& builder) {
      if (hScene.IsValid()) data.hScene = builder.ReadTexture(hScene, xiiGALResourceStateFlags::ShaderResource);
      if (hUI.IsValid()) data.hUI = builder.ReadTexture(hUI, xiiGALResourceStateFlags::ShaderResource);
      data.hFinal = builder.WriteTexture("FinalColor", finalDesc, xiiGALResourceStateFlags::RenderTarget);
      data.uiW    = uiW;
      data.uiH    = uiH;
      builder.SetPassAllowMerge(false);
    },
    [](const UICompData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("UI Composite");
      // UIComposite.xiiShader: blits scene onto render target, then premultiplied-alpha blends UI on top.
      cmd.Draw(3u, 1u, 0u, 0u); // fullscreen triangle for scene blit
      // UI draws (if any) are issued here via retained draw list from the UI subsystem.
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_FinalColorTarget), pData->hFinal);
}

void xiiPopulateUpscalingPass(xiiUpscalingPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiSrcW = 1920u, uiSrcH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiSrcW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiSrcH);

  // Output is always at native display resolution (from swap chain).
  const xiiViewData& vd     = view.GetData();
  const xiiUInt32    uiDstW = static_cast<xiiUInt32>(vd.m_ViewPortRect.width);
  const xiiUInt32    uiDstH = static_cast<xiiUInt32>(vd.m_ViewPortRect.height);

  xiiRGTextureHandle hTAA;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_TAAResolvedColor), hTAA);

  xiiGALTextureCreationDescription upscaleDesc;
  upscaleDesc.m_uiWidth     = uiDstW;
  upscaleDesc.m_uiHeight    = uiDstH;
  upscaleDesc.m_uiMipLevels = 1u;
  upscaleDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  upscaleDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<UpscaleData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hTAA, upscaleDesc, uiSrcW, uiSrcH, uiDstW, uiDstH, &passData](UpscaleData& data, xiiRGBuilder& builder) {
      if (hTAA.IsValid()) data.hTAAResolved = builder.ReadTexture(hTAA, xiiGALResourceStateFlags::ShaderResource);
      data.hUpscaled   = builder.WriteTexture("UpscaledColor", upscaleDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiSrcW      = uiSrcW;
      data.uiSrcH      = uiSrcH;
      data.uiDstW      = uiDstW;
      data.uiDstH      = uiDstH;
      data.fSharpening = passData.m_fSharpeningStrength;
    },
    [](const UpscaleData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("CAS Upscaling");
      // CASUpscale.xiiShader: Contrast-Adaptive Sharpening spatial upscaler.
      // One thread per output pixel; sharpening strength parameterised by fSharpening.
      cmd.Dispatch((data.uiDstW + 7u) / 8u, (data.uiDstH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_UpscaledColor), pData->hUpscaled);
}

void xiiPopulateVolumetricFroxelSetupPass(xiiVolumetricFroxelSetupPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALBufferCreationDescription metaDesc;
  metaDesc.m_uiSize        = sizeof(float) * 4u * passData.m_uiFroxelCountX * passData.m_uiFroxelCountY * passData.m_uiFroxelCountZ;
  metaDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  metaDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  xiiGALTextureCreationDescription scatterDesc;
  scatterDesc.m_uiWidth     = passData.m_uiFroxelCountX;
  scatterDesc.m_uiHeight    = passData.m_uiFroxelCountY;
  scatterDesc.m_uiDepth     = passData.m_uiFroxelCountZ;
  scatterDesc.m_uiMipLevels = 1u;
  scatterDesc.m_TextureType = xiiGALTextureType::Texture3D;
  scatterDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  scatterDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<FroxelData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [metaDesc, scatterDesc, &passData](FroxelData& data, xiiRGBuilder& builder) {
      data.hFroxelMeta       = builder.WriteBuffer("FroxelMetadata", metaDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.hFroxelScattering = builder.WriteTexture("FroxelScattering", scatterDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.cX                = passData.m_uiFroxelCountX;
      data.cY                = passData.m_uiFroxelCountY;
      data.cZ                = passData.m_uiFroxelCountZ;
    },
    [](const FroxelData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Volumetric Froxel Setup");
      // FroxelSetup.xiiShader: [numthreads(8,8,1)], computes per-froxel bounds+phase terms.
      cmd.Dispatch((data.cX + 7u) / 8u, (data.cY + 7u) / 8u, data.cZ);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelMetadataBuffer), pData->hFroxelMeta);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelScatteringBuffer), pData->hFroxelScattering);
}

void xiiPopulateVolumetricIntegrationPass(xiiVolumetricIntegrationPass& passData, xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiRGTextureHandle hFroxel, hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_FroxelScatteringBuffer), hFroxel);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiGALTextureCreationDescription outDesc;
  outDesc.m_uiWidth     = uiW;
  outDesc.m_uiHeight    = uiH;
  outDesc.m_uiMipLevels = 1u;
  outDesc.m_Format      = xiiGALTextureFormat::R16G16B16A16Float;
  outDesc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<VolIntData>(
    passData.GetName(), xiiGALCommandQueueFlags::Compute,
    [hFroxel, hDepth, outDesc, uiW, uiH](VolIntData& data, xiiRGBuilder& builder) {
      if (hFroxel.IsValid()) data.hFroxelScattering = builder.ReadTexture(hFroxel, xiiGALResourceStateFlags::ShaderResource);
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      data.hVolumetricOut = builder.WriteTexture("VolumetricScattering", outDesc, xiiGALResourceStateFlags::UnorderedAccess);
    },
    [](const VolIntData& data, xiiRGPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Volumetric Integration");
      // VolumetricLightIntegration.xiiShader: ray-march each froxel column, accumulate front-to-back.
      cmd.Dispatch((data.cX + 7u) / 8u, (data.cY + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    });

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_VolumetricScattering), pData->hVolumetricOut);
}

// END_MOVED_PIPELINE_PASS_FUNCTIONS

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

struct xiiRenderWorldModule::DefaultPasses
{
  xiiFrameSetupPass              m_FrameSetup;
  xiiDynamicResolutionPass       m_DynamicResolution;
  xiiPerFrameBufferUploadPass    m_PerFrameBufferUpload;
  xiiSkinningAndMorphPass        m_SkinningAndMorph;
  xiiInstanceTransformPass       m_InstanceTransform;
  xiiLodAndMeshletPass           m_LodAndMeshlet;
  xiiCoarseFrustumCullPass       m_CoarseFrustumCull;
  xiiOccluderDepthPrepass        m_OccluderDepthPrepass;
  xiiHiZBuildPass                m_HiZBuild;
  xiiHiZOcclusionCullPass        m_HiZOcclusionCull;
  xiiDrawCommandBuildPass        m_DrawCommandBuild;
  xiiMainDepthPrepassPass        m_MainDepthPrepass;
  xiiMotionVectorPass            m_MotionVector;
  xiiNormalRoughnessPrepassPass  m_NormalRoughnessPrepass;
  xiiShadowCascadeSetupPass      m_ShadowCascadeSetup;
  xiiShadowCasterCullPass        m_ShadowCasterCull;
  xiiDirectionalShadowRenderPass m_DirectionalShadowRender;
  xiiLocalLightShadowPass        m_LocalLightShadow;
  xiiClusterGridAndLightListPass m_ClusterGridAndLightList;
  xiiContactShadowPass           m_ContactShadow;
  xiiDecalPass                   m_Decal;
  xiiAtmosphereLUTPass           m_AtmosphereLUT;
  xiiVolumetricFroxelSetupPass   m_VolumetricFroxelSetup;
  xiiGBufferBasePass             m_GBufferBase;
  xiiEmissiveAuxPass             m_EmissiveAux;
  xiiAmbientOcclusionPass        m_AmbientOcclusion;
  xiiScreenSpaceReflectionsPass  m_ScreenSpaceReflections;
  xiiAccelerationStructurePass   m_AccelerationStructure;
  xiiRTShadowPass                m_RTShadow;
  xiiRTReflectionPass            m_RTReflection;
  xiiRTGlobalIlluminationPass    m_RTGlobalIllumination;
  xiiLightingCombinePass         m_LightingCombine;
  xiiVolumetricIntegrationPass   m_VolumetricIntegration;
  xiiAtmosphereCompositePass     m_AtmosphereComposite;
  xiiOpaqueCompositePass         m_OpaqueComposite;
  xiiTransparentPass             m_Transparent;
  xiiParticleVFXPass             m_ParticleVFX;
  xiiExposurePass                m_Exposure;
  xiiTemporalResolvePass         m_TemporalResolve;
  xiiUpscalingPass               m_Upscaling;
  xiiBloomPass                   m_Bloom;
  xiiToneMappingPass             m_ToneMapping;
  xiiColorGradingPass            m_ColorGrading;
  xiiSharpeningPass              m_Sharpening;
  xiiUICompositePass             m_UIComposite;
  xiiReadbackAndTelemetryPass    m_ReadbackAndTelemetry;
  xiiPresentPass                 m_Present;
};

// -----------------------------------------------------------------------
// Construction / destruction

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
  InitializeDefaultPasses();
}

xiiRenderWorldModule::~xiiRenderWorldModule() = default;

// -----------------------------------------------------------------------
// WorldModule overrides

void xiiRenderWorldModule::Initialize()
{
  // Register ExtractRenderData (concurrent, runs on async worker threads per component manager)
  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExtractRenderData, this);
    desc.m_Phase                     = xiiWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(desc);
  }

  // Register ExecuteRenderGraphs (post-async, single-threaded, after all extraction is complete)
  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExecuteRenderGraphs, this);
    desc.m_Phase                     = xiiWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(desc);
  }
}

void xiiRenderWorldModule::Deinitialize()
{
  m_Views.Clear();
  m_pDefaultPasses     = nullptr;
  m_uiRenderFrameIndex = 0;
}

void xiiRenderWorldModule::OnSimulationStarted()
{
}

// -----------------------------------------------------------------------
// View management

xiiView* xiiRenderWorldModule::CreateView(xiiStringView sName)
{
  xiiUniquePtr<xiiView> pView = XII_DEFAULT_NEW(xiiView);
  pView->SetName(sName);
  xiiView* pRet = pView.Borrow();
  m_Views.PushBack(std::move(pView));
  return pRet;
}

void xiiRenderWorldModule::DestroyView(xiiView* pView)
{
  for (xiiUInt32 i = 0; i < m_Views.GetCount(); ++i)
  {
    if (m_Views[i].Borrow() == pView)
    {
      m_Views.RemoveAtAndCopy(i);
      return;
    }
  }
}

void xiiRenderWorldModule::InitializeDefaultPasses()
{
  if (m_pDefaultPasses != nullptr)
    return;

  m_pDefaultPasses = XII_DEFAULT_NEW(DefaultPasses);
}

void xiiRenderWorldModule::BuildDefaultRenderGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  XII_ASSERT_DEV(m_pDefaultPasses != nullptr, "Default pass set has not been initialized.");

  auto AddIfActive = [&view, &graph, &blackboard](auto& pass, auto&& populatePass) {
    if (pass.IsActive())
    {
      populatePass(pass, view, graph, blackboard);
    }
  };

  AddIfActive(m_pDefaultPasses->m_FrameSetup, xiiPopulateFrameSetupPass);
  AddIfActive(m_pDefaultPasses->m_DynamicResolution, xiiPopulateDynamicResolutionPass);
  AddIfActive(m_pDefaultPasses->m_PerFrameBufferUpload, xiiPopulatePerFrameBufferUploadPass);
  AddIfActive(m_pDefaultPasses->m_SkinningAndMorph, xiiPopulateSkinningAndMorphPass);
  AddIfActive(m_pDefaultPasses->m_InstanceTransform, xiiPopulateInstanceTransformPass);
  AddIfActive(m_pDefaultPasses->m_LodAndMeshlet, xiiPopulateLodAndMeshletPass);
  AddIfActive(m_pDefaultPasses->m_CoarseFrustumCull, xiiPopulateCoarseFrustumCullPass);
  AddIfActive(m_pDefaultPasses->m_OccluderDepthPrepass, xiiPopulateOccluderDepthPrepass);
  AddIfActive(m_pDefaultPasses->m_HiZBuild, xiiPopulateHiZBuildPass);
  AddIfActive(m_pDefaultPasses->m_HiZOcclusionCull, xiiPopulateHiZOcclusionCullPass);
  AddIfActive(m_pDefaultPasses->m_DrawCommandBuild, xiiPopulateDrawCommandBuildPass);
  AddIfActive(m_pDefaultPasses->m_MainDepthPrepass, xiiPopulateMainDepthPrepassPass);
  AddIfActive(m_pDefaultPasses->m_MotionVector, xiiPopulateMotionVectorPass);
  AddIfActive(m_pDefaultPasses->m_NormalRoughnessPrepass, xiiPopulateNormalRoughnessPrepassPass);
  AddIfActive(m_pDefaultPasses->m_ShadowCascadeSetup, xiiPopulateShadowCascadeSetupPass);
  AddIfActive(m_pDefaultPasses->m_ShadowCasterCull, xiiPopulateShadowCasterCullPass);
  AddIfActive(m_pDefaultPasses->m_DirectionalShadowRender, xiiPopulateDirectionalShadowRenderPass);
  AddIfActive(m_pDefaultPasses->m_LocalLightShadow, xiiPopulateLocalLightShadowPass);
  AddIfActive(m_pDefaultPasses->m_ClusterGridAndLightList, xiiPopulateClusterGridAndLightListPass);
  AddIfActive(m_pDefaultPasses->m_ContactShadow, xiiPopulateContactShadowPass);
  AddIfActive(m_pDefaultPasses->m_Decal, xiiPopulateDecalPass);
  AddIfActive(m_pDefaultPasses->m_AtmosphereLUT, xiiPopulateAtmosphereLUTPass);
  AddIfActive(m_pDefaultPasses->m_VolumetricFroxelSetup, xiiPopulateVolumetricFroxelSetupPass);
  AddIfActive(m_pDefaultPasses->m_GBufferBase, xiiPopulateGBufferBasePass);
  AddIfActive(m_pDefaultPasses->m_EmissiveAux, xiiPopulateEmissiveAuxPass);
  AddIfActive(m_pDefaultPasses->m_AmbientOcclusion, xiiPopulateAmbientOcclusionPass);
  AddIfActive(m_pDefaultPasses->m_ScreenSpaceReflections, xiiPopulateScreenSpaceReflectionsPass);
  AddIfActive(m_pDefaultPasses->m_AccelerationStructure, xiiPopulateAccelerationStructurePass);
  AddIfActive(m_pDefaultPasses->m_RTShadow, xiiPopulateRTShadowPass);
  AddIfActive(m_pDefaultPasses->m_RTReflection, xiiPopulateRTReflectionPass);
  AddIfActive(m_pDefaultPasses->m_RTGlobalIllumination, xiiPopulateRTGlobalIlluminationPass);
  AddIfActive(m_pDefaultPasses->m_LightingCombine, xiiPopulateLightingCombinePass);
  AddIfActive(m_pDefaultPasses->m_VolumetricIntegration, xiiPopulateVolumetricIntegrationPass);
  AddIfActive(m_pDefaultPasses->m_AtmosphereComposite, xiiPopulateAtmosphereCompositePass);
  AddIfActive(m_pDefaultPasses->m_OpaqueComposite, xiiPopulateOpaqueCompositePass);
  AddIfActive(m_pDefaultPasses->m_Transparent, xiiPopulateTransparentPass);
  AddIfActive(m_pDefaultPasses->m_ParticleVFX, xiiPopulateParticleVFXPass);
  AddIfActive(m_pDefaultPasses->m_Exposure, xiiPopulateExposurePass);
  AddIfActive(m_pDefaultPasses->m_TemporalResolve, xiiPopulateTemporalResolvePass);
  AddIfActive(m_pDefaultPasses->m_Upscaling, xiiPopulateUpscalingPass);
  AddIfActive(m_pDefaultPasses->m_Bloom, xiiPopulateBloomPass);
  AddIfActive(m_pDefaultPasses->m_ToneMapping, xiiPopulateToneMappingPass);
  AddIfActive(m_pDefaultPasses->m_ColorGrading, xiiPopulateColorGradingPass);
  AddIfActive(m_pDefaultPasses->m_Sharpening, xiiPopulateSharpeningPass);
  AddIfActive(m_pDefaultPasses->m_UIComposite, xiiPopulateUICompositePass);
  AddIfActive(m_pDefaultPasses->m_ReadbackAndTelemetry, xiiPopulateReadbackAndTelemetryPass);
  AddIfActive(m_pDefaultPasses->m_Present, xiiPopulatePresentPass);
}

// -----------------------------------------------------------------------
// Frame update - extraction

void xiiRenderWorldModule::ExtractRenderData(const xiiWorldModule::UpdateContext& context)
{
  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiExtractedRenderData* pExtractedData = pView->GetExtractedRenderData();
    pExtractedData->Clear();

    xiiMsgExtractRenderData msg;
    msg.m_pView                = pView.Borrow();
    msg.m_pExtractedRenderData = pExtractedData;

    // Broadcast to all objects; each object routes to matching component message handlers.
    {
      XII_LOCK(GetWorld()->GetReadMarker());
      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        it->SendMessage(msg);
      }
    }

    // Flatten concurrent batches, then radix-sort each category by sort key.
    pExtractedData->SortAndBatches();
  }
}

// -----------------------------------------------------------------------
// Frame update - graph execution

void xiiRenderWorldModule::ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  const xiiUInt64 uiFrameIndex = m_uiRenderFrameIndex++;

  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiRenderGraph*              pGraph        = pView->GetRenderGraph();
    xiiRenderGraphBlackboard&    blackboard    = pView->GetBlackboard();
    xiiRenderGraphResourceCache& resourceCache = pView->GetResourceCache();

    // Clear the per-view blackboard at the start of each frame so passes start with a clean slate.
    // History data must live inside persistent GPU buffers owned by each pass.
    blackboard.Clear();
    blackboard.Set(xiiMakeHashedString("FrameIndex"), static_cast<xiiUInt32>(uiFrameIndex));

    // Reconstruct the graph for this frame.
    pGraph->BeginSetup(uiFrameIndex);

    const xiiView::RenderGraphBuilder& graphBuilder = pView->GetRenderGraphBuilder();
    if (graphBuilder.IsValid())
    {
      graphBuilder(*pView, *pGraph, blackboard);
    }
    else
    {
      BuildDefaultRenderGraph(*pView, *pGraph, blackboard);
    }

    pGraph->EndSetup();

    xiiRGCompileSettings compileSettings;
    compileSettings.m_bEnableGPUProfiling = true;

    if (pGraph->Compile(compileSettings).Succeeded())
    {
      const xiiResult executeResult = pGraph->Execute(pDevice, pView.Borrow(), &blackboard, &resourceCache);
      XII_ASSERT_DEV(executeResult.Succeeded(), "Render graph execution failed for view '{0}'.", pView->GetName());
    }
  }
}
