#pragma once

#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

struct xiiMsgExtractRenderData;

class XII_PARTICLEPLUGIN_DLL xiiParticleFinisherComponentManager final : public xiiComponentManager<class xiiParticleFinisherComponent, xiiBlockStorageType::Compact>
{
  using SUPER = xiiComponentManager<class xiiParticleFinisherComponent, xiiBlockStorageType::Compact>;

public:
  xiiParticleFinisherComponentManager(xiiWorld* pWorld);

  void UpdateBounds();
};

class XII_PARTICLEPLUGIN_DLL xiiParticleFinisherComponent final : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleFinisherComponent, xiiRenderComponent, xiiParticleFinisherComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

protected:
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiParticleFinisherComponent

public:
  xiiParticleFinisherComponent();
  ~xiiParticleFinisherComponent();

  xiiParticleEffectController m_EffectController;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  void UpdateBounds();
};
