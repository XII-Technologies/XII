#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ReflectionPool)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiReflectionPool::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiReflectionPool::OnEngineShutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////
/// xiiReflectionPool

xiiReflectionProbeId xiiReflectionPool::RegisterReflectionProbe(const xiiWorld* pWorld, const xiiReflectionProbeDesc& desc, const xiiReflectionProbeComponentBase* pComponent)
{
  XII_LOCK(s_pData->m_Mutex);

  Data::ProbeData probe;
  s_pData->UpdateProbeData(probe, desc, pComponent);
  return s_pData->AddProbe(pWorld, std::move(probe));
}

void xiiReflectionPool::DeregisterReflectionProbe(const xiiWorld* pWorld, xiiReflectionProbeId id)
{
  XII_LOCK(s_pData->m_Mutex);
  s_pData->RemoveProbe(pWorld, id);
}

void xiiReflectionPool::UpdateReflectionProbe(const xiiWorld* pWorld, xiiReflectionProbeId id, const xiiReflectionProbeDesc& desc, const xiiReflectionProbeComponentBase* pComponent)
{
  XII_LOCK(s_pData->m_Mutex);
  xiiReflectionPool::Data::WorldReflectionData& data      = s_pData->GetWorldData(pWorld);
  Data::ProbeData&                              probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  s_pData->UpdateProbeData(probeData, desc, pComponent);
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

void xiiReflectionPool::ExtractReflectionProbe(const xiiComponent* pComponent, xiiMsgExtractRenderData& ref_msg, xiiReflectionProbeRenderData* pRenderData0, const xiiWorld* pWorld, xiiReflectionProbeId id, float fPriority)
{
  XII_LOCK(s_pData->m_Mutex);
  s_pData->m_ReflectionProbeUpdater.ScheduleUpdateSteps();

  const xiiUInt32                               uiWorldIndex = pWorld->GetIndex();
  xiiReflectionPool::Data::WorldReflectionData& data         = *s_pData->m_WorldReflectionData[uiWorldIndex];
  data.m_mapping.AddWeight(id, fPriority);
  const xiiInt32 iMappedIndex = data.m_mapping.GetReflectionIndex(id, true);

  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);

  if (pComponent->GetOwner()->IsDynamic())
  {
    xiiTransform globalTransform = pComponent->GetOwner()->GetGlobalTransform();
    if (!probeData.m_Flags.IsSet(xiiProbeFlags::Dynamic) && probeData.m_GlobalTransform != globalTransform)
    {
      data.m_mapping.UpdateProbe(id, probeData.m_Flags);
    }
    probeData.m_GlobalTransform = globalTransform;
  }

  // The sky light is always active and not added to the render data (always passes in nullptr as pRenderData).
  if (pRenderData0 && iMappedIndex > 0)
  {
    // Index and flags are stored in m_uiIndex so we can't just overwrite it.
    pRenderData0->m_uiIndex |= (xiiUInt32)iMappedIndex;
    ref_msg.AddRenderData(pRenderData0, xiiDefaultRenderDataCategories::ReflectionProbe, xiiRenderData::Caching::Never);
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiUInt32 uiMipLevels = GetMipLevels();
  if (probeData.m_desc.m_bShowDebugInfo && s_pData->m_hDebugMaterial.GetCount() == uiMipLevels * s_uiNumReflectionProbeCubeMaps)
  {
    if (ref_msg.m_OverrideCategory == xiiInvalidRenderDataCategory)
    {
      xiiInt32 activeIndex = 0;
      if (s_pData->m_ActiveDynamicUpdate.Contains(xiiReflectionProbeRef{uiWorldIndex, id}))
      {
        activeIndex = 1;
      }

      xiiStringBuilder sEnum;
      xiiReflectionUtils::BitflagsToString(probeData.m_Flags, sEnum, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);
      xiiStringBuilder s;
      s.Format("\n RefIdx: {}\nUpdating: {}\nFlags: {}\n", iMappedIndex, activeIndex, sEnum);
      xiiDebugRenderer::Draw3DText(pWorld, s, pComponent->GetOwner()->GetGlobalPosition(), xiiColorScheme::LightUI(xiiColorScheme::Violet));
    }

    // Not mapped in the atlas - cannot render it.
    if (iMappedIndex < 0)
      return;

    const xiiGameObject* pOwner         = pComponent->GetOwner();
    const xiiTransform   ownerTransform = pOwner->GetGlobalTransform();

    xiiUInt32 uiMipLevelsToRender = probeData.m_desc.m_bShowMipMaps ? uiMipLevels : 1;
    for (xiiUInt32 i = 0; i < uiMipLevelsToRender; i++)
    {
      xiiMeshRenderData* pRenderData             = xiiCreateRenderDataForThisFrame<xiiMeshRenderData>(pOwner);
      pRenderData->m_GlobalTransform.m_vPosition = ownerTransform * probeData.m_desc.m_vCaptureOffset;
      pRenderData->m_GlobalTransform.m_vScale    = xiiVec3(1.0f);
      if (!probeData.m_Flags.IsSet(xiiProbeFlags::SkyLight))
      {
        pRenderData->m_GlobalTransform.m_qRotation = ownerTransform.m_qRotation;
      }
      pRenderData->m_GlobalTransform.m_vPosition.z += s_fDebugSphereRadius * i * 2;
      pRenderData->m_GlobalBounds   = pOwner->GetGlobalBounds();
      pRenderData->m_hMesh          = s_pData->m_hDebugSphere;
      pRenderData->m_hMaterial      = s_pData->m_hDebugMaterial[iMappedIndex * uiMipLevels + i];
      pRenderData->m_Color          = xiiColor::White;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID     = xiiRenderComponent::GetUniqueIdForRendering(pComponent, 0);

      pRenderData->FillBatchIdAndSortingKey();
      ref_msg.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::LitOpaque, xiiRenderData::Caching::Never);
    }
  }
#endif
}

//////////////////////////////////////////////////////////////////////////
/// SkyLight

xiiReflectionProbeId xiiReflectionPool::RegisterSkyLight(const xiiWorld* pWorld, xiiReflectionProbeDesc& ref_desc, const xiiSkyLightComponent* pComponent)
{
  XII_LOCK(s_pData->m_Mutex);
  const xiiUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight |= XII_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= XII_BIT(uiWorldIndex);

  Data::ProbeData probe;
  s_pData->UpdateSkyLightData(probe, ref_desc, pComponent);

  xiiReflectionProbeId id = s_pData->AddProbe(pWorld, std::move(probe));
  return id;
}

void xiiReflectionPool::DeregisterSkyLight(const xiiWorld* pWorld, xiiReflectionProbeId id)
{
  XII_LOCK(s_pData->m_Mutex);

  s_pData->RemoveProbe(pWorld, id);

  const xiiUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight &= ~XII_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= XII_BIT(uiWorldIndex);
}

void xiiReflectionPool::UpdateSkyLight(const xiiWorld* pWorld, xiiReflectionProbeId id, const xiiReflectionProbeDesc& desc, const xiiSkyLightComponent* pComponent)
{
  XII_LOCK(s_pData->m_Mutex);
  xiiReflectionPool::Data::WorldReflectionData& data      = s_pData->GetWorldData(pWorld);
  Data::ProbeData&                              probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  if (s_pData->UpdateSkyLightData(probeData, desc, pComponent))
  {
    // s_pData->UnmapProbe(pWorld->GetIndex(), data, id);
  }
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

//////////////////////////////////////////////////////////////////////////
/// Misc

// static
void xiiReflectionPool::SetConstantSkyIrradiance(const xiiWorld* pWorld, const xiiAmbientCube<xiiColor>& skyIrradiance)
{
  xiiUInt32                         uiWorldIndex     = pWorld->GetIndex();
  xiiAmbientCube<xiiColorLinear16f> skyIrradiance16f = skyIrradiance;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != skyIrradiance16f)
  {
    skyIrradianceStorage[uiWorldIndex] = skyIrradiance16f;

    s_pData->m_uiSkyIrradianceChanged |= XII_BIT(uiWorldIndex);
  }
}

void xiiReflectionPool::ResetConstantSkyIrradiance(const xiiWorld* pWorld)
{
  xiiUInt32 uiWorldIndex = pWorld->GetIndex();

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != xiiAmbientCube<xiiColorLinear16f>())
  {
    skyIrradianceStorage[uiWorldIndex] = xiiAmbientCube<xiiColorLinear16f>();

    s_pData->m_uiSkyIrradianceChanged |= XII_BIT(uiWorldIndex);
  }
}

// static
xiiUInt32 xiiReflectionPool::GetReflectionCubeMapSize()
{
  return s_uiReflectionCubeMapSize;
}

// static
xiiGALTextureHandle xiiReflectionPool::GetReflectionSpecularTexture(xiiUInt32 uiWorldIndex, xiiEnum<xiiCameraUsageHint> cameraUsageHint)
{
  if (uiWorldIndex < s_pData->m_WorldReflectionData.GetCount() && cameraUsageHint != xiiCameraUsageHint::Reflection)
  {
    Data::WorldReflectionData* pData = s_pData->m_WorldReflectionData[uiWorldIndex].Borrow();
    if (pData)
      return pData->m_mapping.GetTexture();
  }
  return s_pData->m_hFallbackReflectionSpecularTexture;
}

// static
xiiGALTextureHandle xiiReflectionPool::GetSkyIrradianceTexture()
{
  return s_pData->m_hSkyIrradianceTexture;
}

//////////////////////////////////////////////////////////////////////////
/// Private Functions

// static
void xiiReflectionPool::OnEngineStartup()
{
  s_pData = XII_DEFAULT_NEW(xiiReflectionPool::Data);

  xiiRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  xiiRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void xiiReflectionPool::OnEngineShutdown()
{
  xiiRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  xiiRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  XII_DEFAULT_DELETE(s_pData);
}

// static
void xiiReflectionPool::OnExtractionEvent(const xiiRenderWorldExtractionEvent& e)
{
  if (e.m_Type == xiiRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    XII_PROFILE_SCOPE("Reflection Pool BeginExtraction");
    s_pData->CreateSkyIrradianceTexture();
    s_pData->CreateReflectionViewsAndResources();
    s_pData->PreExtraction();
  }

  if (e.m_Type == xiiRenderWorldExtractionEvent::Type::EndExtraction)
  {
    XII_PROFILE_SCOPE("Reflection Pool EndExtraction");
    s_pData->PostExtraction();
  }
}

// static
void xiiReflectionPool::OnRenderEvent(const xiiRenderWorldRenderEvent& e)
{
  if (e.m_Type != xiiRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hSkyIrradianceTexture.IsInvalidated())
    return;

  XII_LOCK(s_pData->m_Mutex);

  xiiUInt64 uiWorldHasSkyLight     = s_pData->m_uiWorldHasSkyLight;
  xiiUInt64 uiSkyIrradianceChanged = s_pData->m_uiSkyIrradianceChanged;
  if ((~uiWorldHasSkyLight & uiSkyIrradianceChanged) == 0)
    return;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto                                   pGALPass = pDevice->BeginPass("Sky Irradiance Texture Update");
  xiiHybridArray<xiiGALTextureHandle, 4> atlasToClear;

  {
    auto pGALCommandEncoder = pGALPass->BeginCompute();
    for (xiiUInt32 i = 0; i < skyIrradianceStorage.GetCount(); ++i)
    {
      if ((uiWorldHasSkyLight & XII_BIT(i)) == 0 && (uiSkyIrradianceChanged & XII_BIT(i)) != 0)
      {
        xiiBoundingBoxu32 destBox;
        destBox.m_vMin.Set(0, i, 0);
        destBox.m_vMax.Set(6, i + 1, 1);
        xiiGALSystemMemoryDescription memDesc;
        memDesc.m_pData      = &skyIrradianceStorage[i].m_Values[0];
        memDesc.m_uiRowPitch = sizeof(xiiAmbientCube<xiiColorLinear16f>);
        pGALCommandEncoder->UpdateTexture(s_pData->m_hSkyIrradianceTexture, xiiGALTextureSubresource(), destBox, memDesc);

        uiSkyIrradianceChanged &= ~XII_BIT(i);

        if (i < s_pData->m_WorldReflectionData.GetCount() && s_pData->m_WorldReflectionData[i] != nullptr)
        {
          xiiReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[i];
          atlasToClear.PushBack(data.m_mapping.GetTexture());
        }
      }
    }
    pGALPass->EndCompute(pGALCommandEncoder);
  }

  {
    // Clear specular sky reflection to black.
    const xiiUInt32 uiNumMipMaps = GetMipLevels();
    for (xiiGALTextureHandle atlas : atlasToClear)
    {
      for (xiiUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        for (xiiUInt32 uiFaceIndex = 0; uiFaceIndex < 6; ++uiFaceIndex)
        {
          xiiGALRenderingSetup                      renderingSetup;
          xiiGALRenderTargetViewCreationDescription desc;
          desc.m_hTexture     = atlas;
          desc.m_uiMipLevel   = uiMipMapIndex;
          desc.m_uiFirstSlice = uiFaceIndex;
          desc.m_uiSliceCount = 1;
          renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->CreateRenderTargetView(desc));
          renderingSetup.m_ClearColor              = xiiColor(0, 0, 0, 1);
          renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

          auto pGALCommandEncoder = pGALPass->BeginRendering(renderingSetup, "ClearSkySpecular");
          pGALCommandEncoder->Clear(xiiColor::Black);
          pGALPass->EndRendering(pGALCommandEncoder);
        }
      }
    }
  }

  pDevice->EndPass(pGALPass);
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionPool);
