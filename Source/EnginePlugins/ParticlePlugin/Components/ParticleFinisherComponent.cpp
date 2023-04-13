#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

xiiParticleFinisherComponentManager::xiiParticleFinisherComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld)
{
}

void xiiParticleFinisherComponentManager::UpdateBounds()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->UpdateBounds();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiParticleFinisherComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiHiddenAttribute,
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiParticleFinisherComponent::xiiParticleFinisherComponent()  = default;
xiiParticleFinisherComponent::~xiiParticleFinisherComponent() = default;

void xiiParticleFinisherComponent::OnDeactivated()
{
  m_EffectController.StopImmediate();

  xiiRenderComponent::OnDeactivated();
}

xiiResult xiiParticleFinisherComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  if (m_EffectController.IsAlive())
  {
    m_EffectController.GetBoundingVolume(bounds);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiParticleFinisherComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // do not extract particles during shadow map rendering
  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow)
    return;

  m_EffectController.ExtractRenderData(msg, GetOwner()->GetGlobalTransform());
}

void xiiParticleFinisherComponent::UpdateBounds()
{
  if (m_EffectController.IsAlive())
  {
    // This function is called in the post-transform phase so the global bounds and transform have already been calculated at this point.
    // Therefore we need to manually update the global bounds again to ensure correct bounds for culling and rendering.
    GetOwner()->UpdateLocalBounds();
    GetOwner()->UpdateGlobalBounds();
  }
  else
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
  }
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleFinisherComponent);

