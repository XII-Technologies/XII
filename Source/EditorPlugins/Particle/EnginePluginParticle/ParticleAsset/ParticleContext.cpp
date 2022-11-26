#include <EnginePluginParticle/EnginePluginParticlePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleContext, 1, xiiRTTIDefaultAllocator<xiiParticleContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Particle Effect"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleContext::xiiParticleContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

xiiParticleContext::~xiiParticleContext() = default;

void xiiParticleContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiSimulationSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages

    auto msg = static_cast<const xiiSimulationSettingsMsgToEngine*>(pMsg);

    m_pWorld->SetWorldSimulationEnabled(msg->m_bSimulateWorld);
    m_pWorld->GetClock().SetSpeed(msg->m_fSimulationSpeed);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiEditorEngineRestartSimulationMsg>())
  {
    RestartEffect();
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiEditorEngineLoopAnimationMsg>())
  {
    SetAutoRestartEffect(((const xiiEditorEngineLoopAnimationMsg*)pMsg)->m_bLoop);
  }

  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
}

void xiiParticleContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiParticleComponentManager* pCompMan = pWorld->GetOrCreateComponentManager<xiiParticleComponentManager>();


  // Preview Effect
  {
    xiiGameObjectDesc obj;
    xiiGameObject*    pObj;
    obj.m_sName.Assign("ParticlePreview");
    pWorld->CreateObject(obj, pObj);

    pCompMan->CreateComponent(pObj, m_pComponent);
    m_pComponent->m_OnFinishedAction = xiiOnComponentFinishedAction2::Restart;
    m_pComponent->m_MinRestartDelay  = xiiTime::Seconds(0.5);

    xiiStringBuilder sParticleGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sParticleGuid);
    m_hParticle = xiiResourceManager::LoadResource<xiiParticleEffectResource>(sParticleGuid);

    m_pComponent->SetParticleEffect(m_hParticle);
  }

  const char* szMeshName = "ParticlePreviewBackgroundMesh";
  m_hPreviewMeshResource = xiiResourceManager::GetExistingResource<xiiMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "ParticlePreviewBackgroundMeshBuffer";

    xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      xiiGeometry geom;

      geom.AddBox(xiiVec3(4, 4, 4), true);
      geom.ComputeTangents();

      xiiMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::Triangles);

      hMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }
    {
      xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::AllowLoadingFallback);

      xiiMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.xiiMaterialAsset
      md.ComputeBounds();

      m_hPreviewMeshResource = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  xiiPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<xiiPhysicsWorldModuleInterface>();

  // Background Mesh
  {
    xiiGameObjectDesc obj;
    obj.m_sName.Assign("ParticleBackground");

    const xiiColor bgColor(0.3f, 0.3f, 0.3f);

    for (int y = -1; y <= 5; ++y)
    {
      for (int x = -5; x <= 5; ++x)
      {
        xiiGameObject* pObj;
        obj.m_LocalPosition.Set(6, (float)x * 4, 1 + (float)y * 4);
        pWorld->CreateObject(obj, pObj);

        xiiMeshComponent* pMesh;
        xiiMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(m_hPreviewMeshResource);
        pMesh->SetColor(bgColor);

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, xiiVec3(4, 4, 4));
      }
    }

    for (int y = -5; y <= 5; ++y)
    {
      for (int x = -5; x <= 1; ++x)
      {
        xiiGameObject* pObj;
        obj.m_LocalPosition.Set((float)x * 4, (float)y * 4, -3);
        pWorld->CreateObject(obj, pObj);

        xiiMeshComponent* pMesh;
        xiiMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(m_hPreviewMeshResource);
        pMesh->SetColor(bgColor);

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, xiiVec3(4, 4, 4));
      }
    }

    for (int x = -5; x <= 5; ++x)
    {
      xiiGameObject* pObj;
      obj.m_LocalPosition.Set(4, (float)x * 4, -2);
      obj.m_LocalRotation.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(45));
      pWorld->CreateObject(obj, pObj);

      xiiMeshComponent* pMesh;
      xiiMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);
      pMesh->SetColor(bgColor);

      if (pPhysicsInterface)
        pPhysicsInterface->AddStaticCollisionBox(pObj, xiiVec3(4, 4, 4));
    }
  }
}

xiiEngineProcessViewContext* xiiParticleContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiParticleViewContext, this);
}

void xiiParticleContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

void xiiParticleContext::OnThumbnailViewContextRequested()
{
  m_ThumbnailBoundingVolume.SetInvalid();
}

bool xiiParticleContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  xiiParticleViewContext* pParticleViewContext = static_cast<xiiParticleViewContext*>(pThumbnailViewContext);

  if (!m_ThumbnailBoundingVolume.IsValid())
  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    const bool bWorldPaused = m_pWorld->GetClock().GetPaused();

    // make sure the component restarts as soon as possible
    {
      const xiiTime restartDelay = m_pComponent->m_MinRestartDelay;
      const auto    onFinished   = m_pComponent->m_OnFinishedAction;
      const double  fClockSpeed  = m_pWorld->GetClock().GetSpeed();

      m_pComponent->m_MinRestartDelay.SetZero();
      m_pComponent->m_OnFinishedAction = xiiOnComponentFinishedAction2::Restart;

      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->GetClock().SetPaused(false);
      m_pWorld->GetClock().SetSpeed(10);
      m_pWorld->Update();
      m_pWorld->GetClock().SetPaused(bWorldPaused);
      m_pWorld->GetClock().SetSpeed(fClockSpeed);
      m_pWorld->SetWorldSimulationEnabled(false);

      m_pComponent->m_MinRestartDelay  = restartDelay;
      m_pComponent->m_OnFinishedAction = onFinished;
    }

    if (m_pComponent && !m_pComponent->m_EffectController.IsAlive())
    {
      // if this happens, the effect has finished and we need to wait for it to restart, so that it can be reconfigured for the screenshot
      // not very clean solution

      pParticleViewContext->PositionThumbnailCamera(m_ThumbnailBoundingVolume);
      return false;
    }

    if (m_pComponent && m_pComponent->m_EffectController.IsAlive())
    {
      // set a fixed random seed
      m_pComponent->m_uiRandomSeed = 11;

      const xiiUInt32 uiMinSimSteps    = 3;
      xiiUInt32       uiSimStepsNeeded = uiMinSimSteps;

      if (m_pComponent->m_EffectController.IsContinuousEffect())
      {
        uiSimStepsNeeded = 30;
      }
      else
      {
        m_pComponent->InterruptEffect();
        m_pComponent->StartEffect();

        xiiUInt64 uiMostParticles     = 0;
        xiiUInt64 uiMostParticlesStep = 0;

        for (xiiUInt32 step = 0; step < 30; ++step)
        {
          // step once, to get the initial bbox out of the way
          m_pComponent->m_EffectController.ForceVisible();
          m_pComponent->m_EffectController.Tick(xiiTime::Seconds(0.05));

          if (!m_pComponent->m_EffectController.IsAlive())
            break;

          const xiiUInt64 numParticles = m_pComponent->m_EffectController.GetNumActiveParticles();

          if (step == uiMinSimSteps && numParticles > 0)
          {
            uiMostParticles = 0;
          }

          if (numParticles > uiMostParticles)
          {
            // this is the step with the largest number of particles
            // but usually a few steps later is the best step to capture

            uiMostParticles     = numParticles;
            uiMostParticlesStep = step;
            uiSimStepsNeeded    = step;
          }
          else if ((numParticles > uiMostParticles * 0.8f) && (step < uiMostParticlesStep + 5))
          {
            // if a few steps later we still have a decent amount of particles (so it didn't drop significantly),
            // prefer to use that step
            uiSimStepsNeeded = step;
          }
        }
      }

      m_pComponent->InterruptEffect();
      m_pComponent->StartEffect();

      for (xiiUInt32 step = 0; step < uiSimStepsNeeded; ++step)
      {
        m_pComponent->m_EffectController.ForceVisible();
        m_pComponent->m_EffectController.Tick(xiiTime::Seconds(0.05));

        if (m_pComponent->m_EffectController.IsAlive())
        {
          m_pComponent->m_EffectController.GetBoundingVolume(m_ThumbnailBoundingVolume);

          // shrink the bbox to zoom in
          m_ThumbnailBoundingVolume.m_fSphereRadius *= 0.7f;
          m_ThumbnailBoundingVolume.m_vBoxHalfExtends *= 0.7f;
        }
      }

      m_pComponent->m_uiRandomSeed = 0;

      // tick the world once more, so that the effect passes on its bounding box to the culling system
      // otherwise the effect is not rendered, when only the thumbnail is updated, but the document is not open
      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->GetClock().SetPaused(true);
      m_pWorld->Update();
      m_pWorld->GetClock().SetPaused(bWorldPaused);
      m_pWorld->SetWorldSimulationEnabled(false);
    }
  }

  pParticleViewContext->PositionThumbnailCamera(m_ThumbnailBoundingVolume);
  return true;
}

void xiiParticleContext::RestartEffect()
{
  XII_LOCK(m_pWorld->GetWriteMarker());

  if (m_pComponent)
  {
    m_pComponent->InterruptEffect();
    m_pComponent->StartEffect();
  }
}

void xiiParticleContext::SetAutoRestartEffect(bool loop)
{
  XII_LOCK(m_pWorld->GetWriteMarker());

  if (m_pComponent)
  {
    m_pComponent->m_OnFinishedAction = loop ? xiiOnComponentFinishedAction2::Restart : xiiOnComponentFinishedAction2::None;
  }
}
