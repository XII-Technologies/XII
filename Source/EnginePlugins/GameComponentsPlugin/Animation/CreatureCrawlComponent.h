/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/World/ComponentManager.h>

class xiiPhysicsWorldModuleInterface;

using xiiCreatureCrawlComponentManager = xiiComponentManagerSimple<class xiiCreatureCrawlComponent, xiiComponentUpdateType::WhenSimulating>;

struct xiiCreatureLeg
{
  xiiHashedString m_sLegObject;
  xiiUInt8        m_uiStepGroup = 0;

  xiiVec3             m_vRestPositionRelative;
  xiiGameObjectHandle m_hLegObject;
  xiiVec3             m_vCurTargetPosAbs;
  float               m_fMoveLegFactor;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMECOMPONENTS_DLL, xiiCreatureLeg);

class XII_GAMECOMPONENTS_DLL xiiCreatureCrawlComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCreatureCrawlComponent, xiiComponent, xiiCreatureCrawlComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCreatureCrawlComponent

protected:
  void Update();

  void OnSimulationStarted() override;

public:
  xiiCreatureCrawlComponent();
  ~xiiCreatureCrawlComponent();

  void SetBodyReference(const char* szReference); // [ property ]

  float m_fCastUp         = 0.3f;
  float m_fCastDown       = 1.0f;
  float m_fStepDistance   = 0.4f;
  float m_fMinLegDistance = 0.5f;

protected:
  xiiGameObjectHandle               m_hBody; // [ property ]
  xiiHybridArray<xiiCreatureLeg, 4> m_Legs;  // [ property ]

  const xiiPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
  xiiTime                               m_LastMove;
  xiiQuat                               m_qBodyTilt = xiiQuat::MakeIdentity();

private:
  const char* DummyGetter() const { return nullptr; }
};
