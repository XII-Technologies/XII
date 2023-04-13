#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

// clang-format off

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleTypeRenderMode, 1)
  XII_ENUM_CONSTANT(xiiParticleTypeRenderMode::Opaque),
  XII_ENUM_CONSTANT(xiiParticleTypeRenderMode::Additive),
  XII_ENUM_CONSTANT(xiiParticleTypeRenderMode::Blended),
  XII_ENUM_CONSTANT(xiiParticleTypeRenderMode::BlendedForeground),
  XII_ENUM_CONSTANT(xiiParticleTypeRenderMode::BlendedBackground),
  XII_ENUM_CONSTANT(xiiParticleTypeRenderMode::Distortion),
  XII_ENUM_CONSTANT(xiiParticleTypeRenderMode::BlendAdd),
XII_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiEffectInvisibleUpdateRate, 1)
  XII_ENUM_CONSTANT(xiiEffectInvisibleUpdateRate::FullUpdate),
  XII_ENUM_CONSTANT(xiiEffectInvisibleUpdateRate::Max20fps),
  XII_ENUM_CONSTANT(xiiEffectInvisibleUpdateRate::Max10fps),
  XII_ENUM_CONSTANT(xiiEffectInvisibleUpdateRate::Max5fps),
  XII_ENUM_CONSTANT(xiiEffectInvisibleUpdateRate::Pause),
  XII_ENUM_CONSTANT(xiiEffectInvisibleUpdateRate::Discard),
XII_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleTextureAtlasType, 1)
  XII_ENUM_CONSTANT(xiiParticleTextureAtlasType::None),
  XII_ENUM_CONSTANT(xiiParticleTextureAtlasType::RandomVariations),
  XII_ENUM_CONSTANT(xiiParticleTextureAtlasType::FlipbookAnimation),
  XII_ENUM_CONSTANT(xiiParticleTextureAtlasType::RandomYAnimatedX),
XII_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleColorGradientMode, 1)
  XII_ENUM_CONSTANT(xiiParticleColorGradientMode::Age),
  XII_ENUM_CONSTANT(xiiParticleColorGradientMode::Speed),
XII_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleOutOfBoundsMode, 1)
  XII_ENUM_CONSTANT(xiiParticleOutOfBoundsMode::Teleport),
  XII_ENUM_CONSTANT(xiiParticleOutOfBoundsMode::Die),
XII_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

// clang-format on

XII_STATICLINK_LIBRARY(ParticlePlugin)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Bounds);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Flies);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Gravity);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_PullAlong);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Raycast);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Velocity);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleComponent);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleFinisherComponent);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectController);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectDescriptor);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectInstance);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Burst);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Continuous);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Distance);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction_Effect);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction_Prefab);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_Age);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_ApplyVelocity);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_LastPosition);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_Volume);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Module_ParticleModule);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Renderer_ParticleRenderer);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Resources_ParticleEffectResource);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Startup);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Streams_DefaultParticleStreams);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Streams_ParticleStream);
  XII_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemDescriptor);
  XII_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemInstance);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Effect_ParticleTypeEffect);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Light_ParticleTypeLight);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Mesh_ParticleTypeMesh);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_ParticleType);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_ParticleTypePoint);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_PointRenderer);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Quad_ParticleTypeQuad);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Quad_QuadParticleRenderer);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_ParticleTypeTrail);
  XII_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_TrailRenderer);
  XII_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleEffects);
  XII_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleSystems);
  XII_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleWorldModule);
}
