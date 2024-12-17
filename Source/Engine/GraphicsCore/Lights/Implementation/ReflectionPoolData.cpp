#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lights/Implementation/ReflectionPoolData.h>

#include <Core/Graphics/Geometry.h>
#include <GraphicsCore/Lights/BoxReflectionProbeComponent.h>
#include <GraphicsCore/Lights/SkyLightComponent.h>
#include <GraphicsCore/Lights/SphereReflectionProbeComponent.h>
#include <GraphicsCore/Meshes/MeshComponentBase.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>

//////////////////////////////////////////////////////////////////////////
/// xiiReflectionPool::Data

xiiReflectionPool::Data* xiiReflectionPool::s_pData;

xiiReflectionPool::Data::Data()
{
  m_SkyIrradianceStorage.SetCount(64);
}

xiiReflectionPool::Data::~Data()
{
  if (!m_hFallbackReflectionSpecularTexture.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroyTexture(m_hFallbackReflectionSpecularTexture);
    m_hFallbackReflectionSpecularTexture.Invalidate();
  }

  xiiUInt32 uiWorldReflectionCount = m_WorldReflectionData.GetCount();
  for (xiiUInt32 i = 0; i < uiWorldReflectionCount; ++i)
  {
    WorldReflectionData* pData = m_WorldReflectionData[i].Borrow();
    XII_ASSERT_DEV(!pData || pData->m_Probes.IsEmpty(), "Not all probes were deregistered.");
  }
  m_WorldReflectionData.Clear();

  if (!m_hSkyIrradianceTexture.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroyTexture(m_hSkyIrradianceTexture);
    m_hSkyIrradianceTexture.Invalidate();
  }
}

xiiReflectionProbeId xiiReflectionPool::Data::AddProbe(const xiiWorld* pWorld, ProbeData&& probeData)
{
  const xiiUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex >= s_pData->m_WorldReflectionData.GetCount())
    s_pData->m_WorldReflectionData.SetCount(uiWorldIndex + 1);

  if (s_pData->m_WorldReflectionData[uiWorldIndex] == nullptr)
  {
    s_pData->m_WorldReflectionData[uiWorldIndex]                          = XII_DEFAULT_NEW(WorldReflectionData);
    s_pData->m_WorldReflectionData[uiWorldIndex]->m_mappingSubscriptionId = s_pData->m_WorldReflectionData[uiWorldIndex]->m_mapping.m_Events.AddEventHandler([uiWorldIndex, this](const xiiReflectionProbeMappingEvent& e) {
      OnReflectionProbeMappingEvent(uiWorldIndex, e);
    });
  }

  xiiReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];

  const xiiBitflags<xiiProbeFlags> flags = probeData.m_Flags;
  xiiReflectionProbeId             id    = data.m_Probes.Insert(std::move(probeData));

  if (probeData.m_Flags.IsSet(xiiProbeFlags::SkyLight))
  {
    data.m_SkyLight = id;
  }
  data.m_mapping.AddProbe(id, flags);

  return id;
}

xiiReflectionPool::Data::WorldReflectionData& xiiReflectionPool::Data::GetWorldData(const xiiWorld* pWorld)
{
  const xiiUInt32 uiWorldIndex = pWorld->GetIndex();
  return *s_pData->m_WorldReflectionData[uiWorldIndex];
}

void xiiReflectionPool::Data::RemoveProbe(const xiiWorld* pWorld, xiiReflectionProbeId id)
{
  const xiiUInt32                               uiWorldIndex = pWorld->GetIndex();
  xiiReflectionPool::Data::WorldReflectionData& data         = *s_pData->m_WorldReflectionData[uiWorldIndex];

  data.m_mapping.RemoveProbe(id);

  if (data.m_SkyLight == id)
  {
    data.m_SkyLight.Invalidate();
  }

  data.m_Probes.Remove(id);

  if (data.m_Probes.IsEmpty())
  {
    s_pData->m_WorldReflectionData[uiWorldIndex]->m_mapping.m_Events.RemoveEventHandler(s_pData->m_WorldReflectionData[uiWorldIndex]->m_mappingSubscriptionId);
    s_pData->m_WorldReflectionData[uiWorldIndex].Clear();
  }
}

void xiiReflectionPool::Data::UpdateProbeData(ProbeData& ref_probeData, const xiiReflectionProbeDesc& desc, const xiiReflectionProbeComponentBase* pComponent)
{
  ref_probeData.m_desc            = desc;
  ref_probeData.m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();

  if (const xiiSphereReflectionProbeComponent* pSphere = xiiDynamicCast<const xiiSphereReflectionProbeComponent*>(pComponent))
  {
    ref_probeData.m_Flags = xiiProbeFlags::Sphere;
  }
  else if (const xiiBoxReflectionProbeComponent* pBox = xiiDynamicCast<const xiiBoxReflectionProbeComponent*>(pComponent))
  {
    ref_probeData.m_Flags = xiiProbeFlags::Box;
  }

  if (ref_probeData.m_desc.m_Mode == xiiReflectionProbeMode::Dynamic)
  {
    ref_probeData.m_Flags |= xiiProbeFlags::Dynamic;
  }
  else
  {
    xiiStringBuilder sComponentGuid, sCubeMapFile;
    xiiConversionUtils::ToString(ref_probeData.m_desc.m_uniqueID, sComponentGuid);

    // this is where the editor will put the file for this probe
    sCubeMapFile.SetFormat(":project/AssetCache/Generated/{0}.xiiTexture", sComponentGuid);

    ref_probeData.m_hCubeMap = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sCubeMapFile);
  }
}

bool xiiReflectionPool::Data::UpdateSkyLightData(ProbeData& ref_probeData, const xiiReflectionProbeDesc& desc, const xiiSkyLightComponent* pComponent)
{
  bool bProbeTypeChanged = false;
  if (ref_probeData.m_desc.m_Mode != desc.m_Mode)
  {
    //#TODO any other reason to unmap a probe.
    bProbeTypeChanged = true;
  }

  ref_probeData.m_desc            = desc;
  ref_probeData.m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();

  if (auto pSkyLight = xiiDynamicCast<const xiiSkyLightComponent*>(pComponent))
  {
    ref_probeData.m_Flags    = xiiProbeFlags::SkyLight;
    ref_probeData.m_hCubeMap = pSkyLight->GetCubeMap();
    if (ref_probeData.m_desc.m_Mode == xiiReflectionProbeMode::Dynamic)
    {
      ref_probeData.m_Flags |= xiiProbeFlags::Dynamic;
    }
    else
    {
      if (ref_probeData.m_hCubeMap.IsValid())
      {
        ref_probeData.m_Flags |= xiiProbeFlags::HasCustomCubeMap;
      }
      else
      {
        xiiStringBuilder sComponentGuid, sCubeMapFile;
        xiiConversionUtils::ToString(ref_probeData.m_desc.m_uniqueID, sComponentGuid);

        // this is where the editor will put the file for this probe
        sCubeMapFile.SetFormat(":project/AssetCache/Generated/{0}.xiiTexture", sComponentGuid);

        ref_probeData.m_hCubeMap = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sCubeMapFile);
      }
    }
  }
  return bProbeTypeChanged;
}

void xiiReflectionPool::Data::OnReflectionProbeMappingEvent(const xiiUInt32 uiWorldIndex, const xiiReflectionProbeMappingEvent& e)
{
  switch (e.m_Type)
  {
    case xiiReflectionProbeMappingEvent::Type::ProbeMapped:
      break;
    case xiiReflectionProbeMappingEvent::Type::ProbeUnmapped:
    {
      xiiReflectionProbeRef probeUpdate = {uiWorldIndex, e.m_Id};
      if (m_PendingDynamicUpdate.Contains(probeUpdate))
      {
        m_PendingDynamicUpdate.Remove(probeUpdate);
        m_DynamicUpdateQueue.RemoveAndCopy(probeUpdate);
      }

      if (m_ActiveDynamicUpdate.Contains(probeUpdate))
      {
        m_ActiveDynamicUpdate.Remove(probeUpdate);
        m_ReflectionProbeUpdater.CancelUpdate(probeUpdate);
      }
    }
    break;
    case xiiReflectionProbeMappingEvent::Type::ProbeUpdateRequested:
    {
      // For now, we just manage a FIFO queue of all dynamic probes that have a high enough priority.
      const xiiReflectionProbeRef                   du   = {uiWorldIndex, e.m_Id};
      xiiReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];
      if (!m_PendingDynamicUpdate.Contains(du))
      {
        m_PendingDynamicUpdate.Insert(du);
        m_DynamicUpdateQueue.PushBack(du);
      }
    }
    break;
  }
}

//////////////////////////////////////////////////////////////////////////
/// Dynamic Update

void xiiReflectionPool::Data::PreExtraction()
{
  XII_LOCK(s_pData->m_Mutex);
  const xiiUInt32 uiWorldCount = s_pData->m_WorldReflectionData.GetCount();

  for (xiiUInt32 uiWorld = 0; uiWorld < uiWorldCount; uiWorld++)
  {
    if (s_pData->m_WorldReflectionData[uiWorld] == nullptr)
      continue;

    xiiReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorld];
    data.m_mapping.PreExtraction();
  }

  // Schedule new dynamic updates
  {
    xiiHybridArray<xiiReflectionProbeRef, 4> updatesFinished;
    const xiiUInt32                          uiCount = xiiMath::Min(m_ReflectionProbeUpdater.GetFreeUpdateSlots(updatesFinished), m_DynamicUpdateQueue.GetCount());
    for (const xiiReflectionProbeRef& probe : updatesFinished)
    {
      m_ActiveDynamicUpdate.Remove(probe);

      if (s_pData->m_WorldReflectionData[probe.m_uiWorldIndex] == nullptr)
        continue;

      xiiReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[probe.m_uiWorldIndex];
      data.m_mapping.ProbeUpdateFinished(probe.m_Id);
    }

    for (xiiUInt32 i = 0; i < uiCount; i++)
    {
      xiiReflectionProbeRef nextUpdate = m_DynamicUpdateQueue.PeekFront();
      m_DynamicUpdateQueue.PopFront();
      m_PendingDynamicUpdate.Remove(nextUpdate);

      if (s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex] == nullptr)
        continue;

      xiiReflectionPool::Data::WorldReflectionData& data      = *s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex];
      ProbeData&                                    probeData = data.m_Probes.GetValueUnchecked(nextUpdate.m_Id.m_InstanceIndex);

      xiiReflectionProbeUpdater::TargetSlot target;
      target.m_hSpecularOutputTexture = data.m_mapping.GetTexture();
      target.m_iSpecularOutputIndex   = data.m_mapping.GetReflectionIndex(nextUpdate.m_Id);

      if (probeData.m_Flags.IsSet(xiiProbeFlags::SkyLight))
      {
        target.m_hIrradianceOutputTexture = m_hSkyIrradianceTexture;
        target.m_iIrradianceOutputIndex   = nextUpdate.m_uiWorldIndex;
      }

      if (probeData.m_Flags.IsSet(xiiProbeFlags::HasCustomCubeMap))
      {
        XII_ASSERT_DEBUG(probeData.m_hCubeMap.IsValid(), "");
        XII_VERIFY(m_ReflectionProbeUpdater.StartFilterUpdate(nextUpdate, probeData.m_desc, probeData.m_hCubeMap, target).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      else
      {
        XII_VERIFY(m_ReflectionProbeUpdater.StartDynamicUpdate(nextUpdate, probeData.m_desc, probeData.m_GlobalTransform, target).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      m_ActiveDynamicUpdate.Insert(nextUpdate);
    }
    m_ReflectionProbeUpdater.GenerateUpdateSteps();
  }
}

void xiiReflectionPool::Data::PostExtraction()
{
  XII_LOCK(s_pData->m_Mutex);
  const xiiUInt32 uiWorldCount = s_pData->m_WorldReflectionData.GetCount();
  for (xiiUInt32 uiWorld = 0; uiWorld < uiWorldCount; uiWorld++)
  {
    if (s_pData->m_WorldReflectionData[uiWorld] == nullptr)
      continue;
    xiiReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorld];
    data.m_mapping.PostExtraction();
  }
}

//////////////////////////////////////////////////////////////////////////
/// Resource Creation

void xiiReflectionPool::Data::CreateReflectionViewsAndResources()
{
  if (m_hFallbackReflectionSpecularTexture.IsInvalidated())
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

    xiiGALTextureCreationDescription desc;
    desc.m_Type               = xiiGALResourceDimension::TextureCubeArray;
    desc.m_Format             = xiiGALResourceFormat::RGBA16Float;
    desc.m_uiArraySizeOrDepth = 6;
    desc.m_uiMipLevels        = GetMipLevels();
    desc.m_Size.width         = s_uiReflectionCubeMapSize;
    desc.m_Size.height        = s_uiReflectionCubeMapSize;
    desc.m_BindFlags          = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    desc.m_CPUAccessFlags     = xiiGALCPUAccessFlag::Read;

    m_hFallbackReflectionSpecularTexture = pDevice->CreateTexture(desc);
    if (!m_hFallbackReflectionSpecularTexture.IsInvalidated())
    {
      pDevice->GetTexture(m_hFallbackReflectionSpecularTexture)->SetDebugName("Reflection Fallback Specular Texture");
    }
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (!m_hDebugSphere.IsValid())
  {
    xiiGeometry geom;
    geom.AddStackedSphere(s_fDebugSphereRadius, 32, 16);

    const char*                 szBufferResourceName = "ReflectionProbeDebugSphereBuffer";
    xiiMeshBufferResourceHandle hMeshBuffer          = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      xiiMeshBufferResourceDescriptor desc;
      desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);
      desc.AddStream(xiiGALInputLayoutSemantic::Normal, xiiGALResourceFormat::RGB32Float);
      desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

      hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "ReflectionProbeDebugSphere";
    m_hDebugSphere                 = xiiResourceManager::GetExistingResource<xiiMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      xiiMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (m_hDebugMaterial.IsEmpty())
  {
    const xiiUInt32 uiMipLevelCount = GetMipLevels();

    xiiMaterialResourceHandle            hDebugMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("{ 6f8067d0-ece8-44e1-af46-79b49266de41 }"); // ReflectionProbeVisualization.xiiMaterialAsset
    xiiResourceLock<xiiMaterialResource> pMaterial(hDebugMaterial, xiiResourceAcquireMode::BlockTillLoaded);
    if (pMaterial->GetLoadingState() != xiiResourceState::Loaded)
      return;

    xiiMaterialResourceDescriptor desc                       = pMaterial->GetCurrentDesc();
    xiiUInt32                     uiMipLevel                 = desc.m_Parameters.GetCount();
    xiiUInt32                     uiReflectionProbeIndex     = desc.m_Parameters.GetCount();
    xiiTempHashedString           sMipLevelParam             = "MipLevel";
    xiiTempHashedString           sReflectionProbeIndexParam = "ReflectionProbeIndex";
    for (xiiUInt32 i = 0; i < desc.m_Parameters.GetCount(); ++i)
    {
      if (desc.m_Parameters[i].m_Name == sMipLevelParam)
      {
        uiMipLevel = i;
      }
      if (desc.m_Parameters[i].m_Name == sReflectionProbeIndexParam)
      {
        uiReflectionProbeIndex = i;
      }
    }

    if (uiMipLevel >= desc.m_Parameters.GetCount() || uiReflectionProbeIndex >= desc.m_Parameters.GetCount())
      return;

    m_hDebugMaterial.SetCount(uiMipLevelCount * s_uiNumReflectionProbeCubeMaps);
    for (xiiUInt32 iReflectionProbeIndex = 0; iReflectionProbeIndex < s_uiNumReflectionProbeCubeMaps; iReflectionProbeIndex++)
    {
      for (xiiUInt32 iMipLevel = 0; iMipLevel < uiMipLevelCount; iMipLevel++)
      {
        desc.m_Parameters[uiMipLevel].m_Value             = iMipLevel;
        desc.m_Parameters[uiReflectionProbeIndex].m_Value = iReflectionProbeIndex;
        xiiStringBuilder sMaterialName;
        sMaterialName.SetFormat("ReflectionProbeVisualization - MipLevel {}, Index {}", iMipLevel, iReflectionProbeIndex);

        xiiMaterialResourceDescriptor desc2                                   = desc;
        m_hDebugMaterial[iReflectionProbeIndex * uiMipLevelCount + iMipLevel] = xiiResourceManager::GetOrCreateResource<xiiMaterialResource>(sMaterialName, std::move(desc2));
      }
    }
  }
#endif
}

void xiiReflectionPool::Data::CreateSkyIrradianceTexture()
{
  if (m_hSkyIrradianceTexture.IsInvalidated())
  {
    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

    xiiGALTextureCreationDescription desc;
    desc.m_Type        = xiiGALResourceDimension::Texture2D;
    desc.m_Format      = xiiGALResourceFormat::RGBA16Float;
    desc.m_Size.width  = 6;
    desc.m_Size.height = 64;
    desc.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

    m_hSkyIrradianceTexture = pDevice->CreateTexture(desc);

    if (!m_hSkyIrradianceTexture.IsInvalidated())
    {
      pDevice->GetTexture(m_hSkyIrradianceTexture)->SetDebugName("Sky Irradiance Texture");
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_ReflectionPoolData);
