#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEffectAssetDocument, 6, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleEffectAssetDocument::xiiParticleEffectAssetDocument(const char* szDocumentPath) :
  xiiSimpleAssetDocument<xiiParticleEffectDescriptor>(szDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
  xiiVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_bRenderVisualizers);
}

void xiiParticleEffectAssetDocument::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiParticleEffectDescriptor>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bShared = e.m_pObject->GetTypeAccessor().GetValue("AlwaysShared").ConvertTo<bool>();

    props["SimulateInLocalSpace"].m_Visibility = bShared ? xiiPropertyUiState::Disabled : xiiPropertyUiState::Default;
    props["ApplyOwnerVelocity"].m_Visibility   = bShared ? xiiPropertyUiState::Disabled : xiiPropertyUiState::Default;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiParticleTypeQuadFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    xiiInt64 orientation  = e.m_pObject->GetTypeAccessor().GetValue("Orientation").ConvertTo<xiiInt64>();
    xiiInt64 renderMode   = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<xiiInt64>();
    xiiInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<xiiInt64>();

    props["Deviation"].m_Visibility          = xiiPropertyUiState::Invisible;
    props["DistortionTexture"].m_Visibility  = xiiPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = xiiPropertyUiState::Invisible;
    props["ParticleStretch"].m_Visibility =
      (orientation == xiiQuadParticleOrientation::FixedAxis_EmitterDir || orientation == xiiQuadParticleOrientation::FixedAxis_ParticleDir) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility =
      (textureAtlas == (int)xiiParticleTextureAtlasType::None) ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility =
      (textureAtlas == (int)xiiParticleTextureAtlasType::None) ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;


    if (orientation == xiiQuadParticleOrientation::Fixed_EmitterDir || orientation == xiiQuadParticleOrientation::Fixed_WorldUp)
    {
      props["Deviation"].m_Visibility = xiiPropertyUiState::Default;
    }

    if (renderMode == xiiParticleTypeRenderMode::Distortion)
    {
      props["DistortionTexture"].m_Visibility  = xiiPropertyUiState::Default;
      props["DistortionStrength"].m_Visibility = xiiPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiParticleTypeTrailFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    xiiInt64 renderMode   = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<xiiInt64>();
    xiiInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<xiiInt64>();

    props["DistortionTexture"].m_Visibility  = xiiPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = xiiPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility =
      (textureAtlas == (int)xiiParticleTextureAtlasType::None) ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility =
      (textureAtlas == (int)xiiParticleTextureAtlasType::None) ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;

    if (renderMode == xiiParticleTypeRenderMode::Distortion)
    {
      props["DistortionTexture"].m_Visibility  = xiiPropertyUiState::Default;
      props["DistortionStrength"].m_Visibility = xiiPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiParticleBehaviorFactory_ColorGradient>())
  {
    auto& props = *e.m_pPropertyStates;

    xiiInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("ColorGradientMode").ConvertTo<xiiInt64>();

    props["GradientMaxSpeed"].m_Visibility = (mode == xiiParticleColorGradientMode::Speed) ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiParticleInitializerFactory_CylinderPosition>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bSetVelocity = e.m_pObject->GetTypeAccessor().GetValue("SetVelocity").ConvertTo<bool>();

    props["Speed"].m_Visibility = bSetVelocity ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiParticleInitializerFactory_SpherePosition>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bSetVelocity = e.m_pObject->GetTypeAccessor().GetValue("SetVelocity").ConvertTo<bool>();

    props["Speed"].m_Visibility = bSetVelocity ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  }
}

void xiiParticleEffectAssetDocument::WriteResource(xiiStreamWriter& stream) const
{
  const xiiParticleEffectDescriptor* pProp = GetProperties();

  pProp->Save(stream);
}


void xiiParticleEffectAssetDocument::TriggerRestartEffect()
{
  xiiParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiParticleEffectAssetEvent::RestartEffect;

  m_Events.Broadcast(e);
}


void xiiParticleEffectAssetDocument::SetAutoRestart(bool enable)
{
  if (m_bAutoRestart == enable)
    return;

  m_bAutoRestart = enable;

  xiiParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiParticleEffectAssetEvent::AutoRestartChanged;

  m_Events.Broadcast(e);
}


void xiiParticleEffectAssetDocument::SetSimulationPaused(bool bPaused)
{
  if (m_bSimulationPaused == bPaused)
    return;

  m_bSimulationPaused = bPaused;

  xiiParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiParticleEffectAssetEvent::SimulationSpeedChanged;

  m_Events.Broadcast(e);
}

void xiiParticleEffectAssetDocument::SetSimulationSpeed(float speed)
{
  if (m_fSimulationSpeed == speed)
    return;

  m_fSimulationSpeed = speed;

  xiiParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiParticleEffectAssetEvent::SimulationSpeedChanged;

  m_Events.Broadcast(e);
}


void xiiParticleEffectAssetDocument::SetRenderVisualizers(bool b)
{
  if (m_bRenderVisualizers == b)
    return;

  m_bRenderVisualizers = b;

  xiiVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_bRenderVisualizers);

  xiiParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiParticleEffectAssetEvent::RenderVisualizersChanged;

  m_Events.Broadcast(e);
}

xiiResult xiiParticleEffectAssetDocument::ComputeObjectTransformation(const xiiDocumentObject* pObject, xiiTransform& out_Result) const
{
  // currently the preview particle effect is always at the origin
  out_Result.SetIdentity();
  return XII_SUCCESS;
}

void xiiParticleEffectAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  auto* desc = GetProperties();

  for (const auto& system : desc->GetParticleSystems())
  {
    for (const auto& type : system->GetTypeFactories())
    {
      if (auto* pType = xiiDynamicCast<xiiParticleTypeQuadFactory*>(type))
      {
        if (pType->m_RenderMode != xiiParticleTypeRenderMode::Distortion)
        {
          // remove unused dependencies
          pInfo->m_AssetTransformDependencies.Remove(pType->m_sDistortionTexture);
        }
      }

      if (auto* pType = xiiDynamicCast<xiiParticleTypeTrailFactory*>(type))
      {
        if (pType->m_RenderMode != xiiParticleTypeRenderMode::Distortion)
        {
          // remove unused dependencies
          pInfo->m_AssetTransformDependencies.Remove(pType->m_sDistortionTexture);
        }
      }
    }
  }

  // shared effects do not support parameters
  if (!desc->m_bAlwaysShared)
  {
    xiiExposedParameters* pExposedParams = XII_DEFAULT_NEW(xiiExposedParameters);
    for (auto it = desc->m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName        = it.Key();
      param->m_DefaultValue = it.Value();
    }
    for (auto it = desc->m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName        = it.Key();
      param->m_DefaultValue = it.Value();
    }

    // Info takes ownership of meta data.
    pInfo->m_MetaInfo.PushBack(pExposedParams);
  }
}

xiiTransformStatus xiiParticleEffectAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  WriteResource(stream);
  return xiiStatus(XII_SUCCESS);
}

xiiTransformStatus xiiParticleEffectAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
