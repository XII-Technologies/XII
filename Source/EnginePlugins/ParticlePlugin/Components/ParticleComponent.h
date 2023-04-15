#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

class xiiParticleRenderData;
struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractRenderData;
class xiiParticleSystemInstance;
class xiiParticleComponent;
struct xiiMsgSetPlaying;

using xiiParticleEffectResourceHandle = xiiTypedResourceHandle<class xiiParticleEffectResource>;

class XII_PARTICLEPLUGIN_DLL xiiParticleComponentManager final : public xiiComponentManager<class xiiParticleComponent, xiiBlockStorageType::Compact>
{
  using SUPER = xiiComponentManager<class xiiParticleComponent, xiiBlockStorageType::Compact>;

public:
  xiiParticleComponentManager(xiiWorld* pWorld);

  virtual void Initialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);

  void UpdatePfxTransformsAndBounds();
};

class XII_PARTICLEPLUGIN_DLL xiiParticleComponent final : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleComponent, xiiRenderComponent, xiiParticleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiParticleComponent

public:
  xiiParticleComponent();
  ~xiiParticleComponent();

  /// \brief Starts a new particle effect. If one is already running, it will be stopped (but not interrupted) and a new one is started as
  /// well.
  ///
  /// Returns false, if no valid particle resource is specified.
  bool StartEffect(); // [ scriptable ]

  /// \brief Stops emitting further particles, making any existing particle system stop in a finite amount of time.
  void StopEffect(); // [ scriptable ]

  /// \brief Cancels the entire effect immediately, it will pop out of existence.
  void InterruptEffect(); // [ scriptable ]

  /// \brief Returns true, if an effect is currently in a state where it might emit new particles
  bool IsEffectActive() const; // [ scriptable ]

  void OnMsgSetPlaying(xiiMsgSetPlaying& ref_msg); // [ msg handler ]

  void                    SetParticleEffect(const xiiParticleEffectResourceHandle& hEffect);
  XII_ALWAYS_INLINE const xiiParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffectResource; }

  void        SetParticleEffectFile(const char* szFile); // [ property ]
  const char* GetParticleEffectFile() const;             // [ property ]

  // Exposed Parameters
  const xiiRangeView<const char*, xiiUInt32> GetParameters() const;                                        // [ property ]
  void                                       SetParameter(const char* szKey, const xiiVariant& value);     // [ property ]
  void                                       RemoveParameter(const char* szKey);                           // [ property ]
  bool                                       GetParameter(const char* szKey, xiiVariant& out_value) const; // [ property ]

  xiiUInt64 m_uiRandomSeed = 0;    // [ property ]
  xiiString m_sSharedInstanceName; // [ property ]

  bool                                   m_bSpawnAtStart              = true;        // [ property ]
  bool                                   m_bIfContinuousStopRightAway = false;       // [ property ]
  bool                                   m_bIgnoreOwnerRotation       = false;       // [ property ]
  xiiEnum<xiiOnComponentFinishedAction2> m_OnFinishedAction;                         // [ property ]
  xiiTime                                m_MinRestartDelay;                          // [ property ]
  xiiTime                                m_RestartDelayRange;                        // [ property ]
  xiiEnum<xiiBasisAxis>                  m_SpawnDirection = xiiBasisAxis::PositiveZ; // [ property ]

  xiiParticleEffectController m_EffectController;

protected:
  void         Update();
  xiiTransform GetPfxTransform() const;
  void         UpdatePfxTransform();

  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;
  void OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg);

  virtual void OnDeactivated() override;

  xiiParticleEffectResourceHandle m_hEffectResource;
  xiiTime                         m_RestartTime;

  // Exposed Parameters
  friend class xiiParticleEventReaction_Effect;
  bool                                           m_bFloatParamsChanged = false;
  bool                                           m_bColorParamsChanged = false;
  xiiHybridArray<xiiParticleEffectFloatParam, 2> m_FloatParams;
  xiiHybridArray<xiiParticleEffectColorParam, 2> m_ColorParams;
};
