#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/RopeSimulator.h>
#include <GameplayPlugin/GameplayPluginDLL.h>

//////////////////////////////////////////////////////////////////////////

class XII_GAMEPLAYPLUGIN_DLL xiiFakeRopeComponentManager : public xiiComponentManager<class xiiFakeRopeComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiFakeRopeComponentManager(xiiWorld* pWorld);
  ~xiiFakeRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const xiiWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEPLAYPLUGIN_DLL xiiFakeRopeComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiFakeRopeComponent, xiiComponent, xiiFakeRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiFakeRopeComponent

public:
  xiiFakeRopeComponent();
  ~xiiFakeRopeComponent();

  xiiUInt16 m_uiPieces = 16; // [ property ]

  void SetAnchor1Reference(const char* szReference); // [ property ]
  void SetAnchor2Reference(const char* szReference); // [ property ]
  void SetAnchor1(xiiGameObjectHandle hActor);
  void SetAnchor2(xiiGameObjectHandle hActor);

  void  SetSlack(float fVal);
  float GetSlack() const { return m_fSlack; }

  void SetAttachToAnchor1(bool bVal);
  void SetAttachToAnchor2(bool bVal);
  bool GetAttachToAnchor1() const;
  bool GetAttachToAnchor2() const;

  float m_fSlack   = 0.0f;
  float m_fDamping = 0.5f;

private:
  xiiResult ConfigureRopeSimulator();
  void      SendCurrentPose();
  void      SendPreviewPose();
  void      RuntimeUpdate();

  xiiGameObjectHandle m_hAnchor1;
  xiiGameObjectHandle m_hAnchor2;

  xiiUInt32 m_uiPreviewHash = 0;

  // if the owner or the anchor object are flagged as 'dynamic', the rope must follow their movement
  // otherwise it can skip some update steps
  bool             m_bIsDynamic                = true;
  xiiUInt8         m_uiCheckEquilibriumCounter = 0;
  xiiUInt8         m_uiSleepCounter            = 0;
  xiiRopeSimulator m_RopeSim;
  float            m_fWindInfluence = 0.0f;

private:
  const char* DummyGetter() const { return nullptr; }
};
