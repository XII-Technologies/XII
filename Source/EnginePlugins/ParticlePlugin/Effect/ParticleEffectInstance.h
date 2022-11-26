#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

class xiiParticleEffectInstance;

class xiiParticleEffectUpdateTask final : public xiiTask
{
public:
  xiiParticleEffectUpdateTask(xiiParticleEffectInstance* pEffect);

  xiiTime m_UpdateDiff;

private:
  virtual void Execute() override;

  xiiParticleEffectInstance* m_pEffect;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleEffectInstance
{
  friend class xiiParticleWorldModule;
  friend class xiiParticleEffectUpdateTask;

public:
  xiiParticleEffectInstance();
  ~xiiParticleEffectInstance();

  void Construct(xiiParticleEffectHandle hEffectHandle, const xiiParticleEffectResourceHandle& hResource, xiiWorld* pWorld, xiiParticleWorldModule* pOwnerModule, xiiUInt64 uiRandomSeed, bool bIsShared, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams);
  void Destruct();

  void Interrupt();

  const xiiParticleEffectHandle& GetHandle() const { return m_hEffectHandle; }

  void SetEmitterEnabled(bool enable);
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ClearParticleSystems();
  void ClearEventReactions();

  bool IsContinuous() const;

  xiiWorld*               GetWorld() const { return m_pWorld; }
  xiiParticleWorldModule* GetOwnerWorldModule() const { return m_pOwnerModule; }

  const xiiParticleEffectResourceHandle& GetResource() const { return m_hResource; }

  const xiiHybridArray<xiiParticleSystemInstance*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void AddParticleEvent(const xiiParticleEvent& pe);

  xiiRandom& GetRNG() { return m_Random; }

  xiiUInt64 GetRandomSeed() const { return m_uiRandomSeed; }

  void UpdateWindSamples();

  /// \brief Returns the number of currently active particles across all systems.
  xiiUInt64 GetNumActiveParticles() const;

  /// @name Transform Related
  /// @{
public:
  /// \brief Whether the effect is simulated around the origin and thus not affected by instance position and rotation
  bool IsSimulatedInLocalSpace() const { return m_bSimulateInLocalSpace; }

  /// \brief Sets the transformation of this instance
  void SetTransform(const xiiTransform& transform, const xiiVec3& vParticleStartVelocity);

  /// \brief Sets the transformation of this instance that should be used next frame.
  /// This function is typically used to set the transformation while the particle simulation is running to prevent race conditions.
  void SetTransformForNextFrame(const xiiTransform& transform, const xiiVec3& vParticleStartVelocity);

  /// \brief Returns the transform of the main or shared instance.
  const xiiTransform& GetTransform() const { return m_Transform; }

  /// \brief For the renderer to know whether the instance transform has to be applied to each particle position.
  bool NeedsToApplyTransform() const { return m_bSimulateInLocalSpace || m_bIsSharedEffect; }

  /// \brief Adds a location where the wind system should be sampled.
  ///
  /// Particle behaviors can't sample the wind system directly, because they are updated in parallel and this can
  /// cause crashes. Therefore the desired sample locations have to be cached.
  /// In the next frame, the result can be retrieved via GetWindSampleResult() with the returned index.
  ///
  /// Only a very limited amount of locations can be sampled (4) across all behaviors.
  xiiInt32 AddWindSampleLocation(const xiiVec3& pos);

  /// \brief Returns the wind result sampled at the previously specified location (see AddWindSampleLocation()).
  ///
  /// Returns a zero vector, if no wind value is available (invalid index).
  xiiVec3 GetWindSampleResult(xiiInt32 idx) const;

private:
  void PassTransformToSystems();

  xiiTransform m_Transform;
  xiiTransform m_TransformForNextFrame;

  xiiVec3 m_vVelocity;
  xiiVec3 m_vVelocityForNextFrame;

  xiiStaticArray<xiiVec3, 4> m_vSampleWindLocations[2];
  xiiStaticArray<xiiVec3, 4> m_vSampleWindResults[2];

  /// @}
  /// @name Updates
  /// @{

public:
  /// \brief Returns false when the effect is finished.
  bool Update(const xiiTime& tDiff);

  /// \brief Returns the total (game) time that the effect is alive and has been updated.
  ///
  /// Use this time, instead of a world clock, for time-dependent calculations. It is mostly tied to the world clock (game update),
  /// but additionally includes pre-simulation timings, which would otherwise be left out which can break some calculations.
  xiiTime GetTotalEffectLifeTime() const { return m_TotalEffectLifeTime; }

private: // friend xiiParticleWorldModule
  /// \brief Whether this instance is in a state where its update task should be run
  bool ShouldBeUpdated() const;

  /// \brief Returns the task that is used to update the effect
  const xiiSharedPtr<xiiTask>& GetUpdateTask() { return m_pTask; }

private: // friend xiiParticleEffectUpdateTask
  friend class xiiParticleEffectController;
  /// \brief If the effect wants to skip all the initial behavior, this simulates it multiple times before it is shown the first time.
  void PreSimulate();

  /// \brief Applies a given time step, without any restrictions.
  bool StepSimulation(const xiiTime& tDiff);

private:
  xiiTime m_TotalEffectLifeTime    = xiiTime::Zero();
  xiiTime m_ElapsedTimeSinceUpdate = xiiTime::Zero();


  /// @}
  /// @name Shared Instances
  /// @{
public:
  /// \brief Returns true, if this effect is configured to be simulated once per frame, but rendered by multiple instances.
  bool IsSharedEffect() const { return m_bIsSharedEffect; }

private: // friend xiiParticleWorldModule
  void AddSharedInstance(const void* pSharedInstanceOwner);
  void RemoveSharedInstance(const void* pSharedInstanceOwner);

private:
  bool m_bIsSharedEffect = false;

  /// @}
  /// \name Visibility and Culling
  /// @{
public:
  /// \brief Marks this effect as visible from at least one view.
  /// This affects simulation update rates.
  void SetIsVisible() const;

  void SetVisibleIf(xiiParticleEffectInstance* pOtherVisible);

  /// \brief Whether the effect has been marked as visible recently.
  bool IsVisible() const;

  /// \brief Returns the bounding volume of the effect.
  /// The volume is in the local space of the effect.
  void GetBoundingVolume(xiiBoundingBoxSphere& volume) const;

private:
  void CombineSystemBoundingVolumes();

  xiiBoundingBoxSphere                  m_BoundingVolume;
  mutable xiiTime                       m_EffectIsVisible;
  xiiParticleEffectInstance*            m_pVisibleIf = nullptr;
  xiiEnum<xiiEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  xiiUInt64                             m_uiRandomSeed = 0;

  /// @}
  /// \name Effect Parameters
  /// @{
public:
  void SetParameter(const xiiTempHashedString& name, float value);
  void SetParameter(const xiiTempHashedString& name, const xiiColor& value);

  xiiInt32 FindFloatParameter(const xiiTempHashedString& name) const;
  float    GetFloatParameter(const xiiTempHashedString& name, float defaultValue) const;
  float    GetFloatParameter(xiiUInt32 idx) const { return m_FloatParameters[idx].m_fValue; }

  xiiInt32        FindColorParameter(const xiiTempHashedString& name) const;
  const xiiColor& GetColorParameter(const xiiTempHashedString& name, const xiiColor& defaultValue) const;
  const xiiColor& GetColorParameter(xiiUInt32 idx) const { return m_ColorParameters[idx].m_Value; }


private:
  struct FloatParameter
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt64 m_uiNameHash;
    float     m_fValue;
  };

  struct ColorParameter
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt64 m_uiNameHash;
    xiiColor  m_Value;
  };

  xiiHybridArray<FloatParameter, 2> m_FloatParameters;
  xiiHybridArray<ColorParameter, 2> m_ColorParameters;

  /// @}


private:
  void Reconfigure(bool bFirstTime, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams);
  void ClearParticleSystem(xiiUInt32 index);
  void ProcessEventQueues();

  // for deterministic randomness
  xiiRandom m_Random;

  xiiHashSet<const void*>         m_SharedInstances;
  xiiParticleEffectHandle         m_hEffectHandle;
  bool                            m_bEmitterEnabled        = true;
  bool                            m_bSimulateInLocalSpace  = false;
  bool                            m_bIsFinishing           = false;
  xiiUInt8                        m_uiReviveTimeout        = 3;
  xiiInt8                         m_iMinSimStepsToDo       = 0;
  float                           m_fApplyInstanceVelocity = 0;
  xiiTime                         m_PreSimulateDuration;
  xiiParticleEffectResourceHandle m_hResource;

  xiiParticleWorldModule*                       m_pOwnerModule = nullptr;
  xiiWorld*                                     m_pWorld       = nullptr;
  xiiHybridArray<xiiParticleSystemInstance*, 4> m_ParticleSystems;
  xiiHybridArray<xiiParticleEventReaction*, 4>  m_EventReactions;

  xiiSharedPtr<xiiTask> m_pTask;

  xiiStaticArray<xiiParticleEvent, 16> m_EventQueue;
};
