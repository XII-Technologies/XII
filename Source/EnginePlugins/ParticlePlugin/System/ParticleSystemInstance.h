#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct xiiMsgExtractRenderData;

/// \brief A particle system stores all data for one 'layer' of a running particle effect
class XII_PARTICLEPLUGIN_DLL xiiParticleSystemInstance
{
public:
  xiiParticleSystemInstance();

  void Construct(xiiUInt32 uiMaxParticles, xiiWorld* pWorld, xiiParticleEffectInstance* pOwnerEffect, float fSpawnCountMultiplier);
  void Destruct();

  bool IsVisible() const { return m_bVisible; }

  void SetEmitterEnabled(bool bEnable) { m_bEmitterEnabled = bEnable; }
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ConfigureFromTemplate(const xiiParticleSystemDescriptor* pTemplate);
  void Finalize();

  void ReinitializeStreamProcessors(const xiiParticleSystemDescriptor* pTemplate);

  void CreateStreamProcessors(const xiiParticleSystemDescriptor* pTemplate);

  void SetupOptionalStreams();

  void                SetTransform(const xiiTransform& transform, const xiiVec3& vParticleStartVelocity);
  const xiiTransform& GetTransform() const { return m_Transform; }
  const xiiVec3&      GetParticleStartVelocity() const { return m_vParticleStartVelocity; }

  xiiParticleSystemState::Enum Update(const xiiTime& diff);

  xiiWorld* GetWorld() const { return m_pWorld; }

  xiiUInt64 GetMaxParticles() const { return m_StreamGroup.GetNumElements(); }
  xiiUInt64 GetNumActiveParticles() const { return m_StreamGroup.GetNumActiveElements(); }



  /// \brief Returns the desired stream, if it already exists, nullptr otherwise.
  xiiProcessingStream* QueryStream(const char* szName, xiiProcessingStream::DataType type) const;

  /// \brief Returns the desired stream, if it already exists, creates it otherwise.
  void CreateStream(const char* szName, xiiProcessingStream::DataType type, xiiProcessingStream** pStream, xiiParticleStreamBinding& ref_binding, bool bExpectInitializedValue);

  void ProcessEventQueue(xiiParticleEventQueue queue);

  xiiParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerEffect; }
  xiiParticleWorldModule*    GetOwnerWorldModule() const;

  void ExtractSystemRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const;

  typedef xiiEvent<const xiiStreamGroupElementRemovedEvent&>::Handler ParticleDeathHandler;

  void AddParticleDeathEventHandler(ParticleDeathHandler handler);
  void RemoveParticleDeathEventHandler(ParticleDeathHandler handler);

  void                        SetBoundingVolume(const xiiBoundingBoxSphere& volume, float fMaxParticleSize);
  const xiiBoundingBoxSphere& GetBoundingVolume() const { return m_BoundingVolume; }

  bool IsContinuous() const;

  float GetSpawnCountMultiplier() const { return m_fSpawnCountMultiplier; }

private:
  bool IsEmitterConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const;
  bool IsInitializerConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const;
  bool IsBehaviorConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const;
  bool IsTypeConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const;
  bool IsFinalizerConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const;

  void CreateStreamZeroInitializers();

  xiiHybridArray<xiiParticleEmitter*, 2>     m_Emitters;
  xiiHybridArray<xiiParticleInitializer*, 6> m_Initializers;
  xiiHybridArray<xiiParticleBehavior*, 6>    m_Behaviors;
  xiiHybridArray<xiiParticleFinalizer*, 2>   m_Finalizers;
  xiiHybridArray<xiiParticleType*, 2>        m_Types;

  bool                       m_bVisible; // typically used in editor to hide a system
  bool                       m_bEmitterEnabled;
  xiiParticleEffectInstance* m_pOwnerEffect;
  xiiWorld*                  m_pWorld;
  xiiTransform               m_Transform;
  xiiVec3                    m_vParticleStartVelocity;
  float                      m_fSpawnCountMultiplier = 1.0f;

  xiiProcessingStreamGroup m_StreamGroup;

  struct StreamInfo
  {
    xiiString                     m_sName;
    bool                          m_bGetsInitialized    = false;
    bool                          m_bInUse              = false;
    xiiProcessingStreamProcessor* m_pDefaultInitializer = nullptr;
  };

  xiiHybridArray<StreamInfo, 16> m_StreamInfo;

  // culling data
  xiiBoundingBoxSphere m_BoundingVolume;
};
