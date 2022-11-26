#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiWorld;
class xiiParticleSystemDescriptor;
class xiiParticleEventReactionFactory;
class xiiParticleEventReaction;
class xiiParticleEmitter;
class xiiParticleInitializer;
class xiiParticleBehavior;
class xiiParticleType;
class xiiProcessingStreamGroup;
class xiiProcessingStream;
class xiiRandom;
struct xiiParticleEvent;
class xiiParticleEffectDescriptor;
class xiiParticleWorldModule;
class xiiParticleEffectInstance;
class xiiParticleSystemInstance;
struct xiiRenderViewContext;
class xiiRenderPipelinePass;
class xiiParticleFinalizer;
class xiiParticleFinalizerFactory;

using xiiParticleEffectResourceHandle = xiiTypedResourceHandle<class xiiParticleEffectResource>;

typedef xiiGenericId<22, 10> xiiParticleEffectId;

/// \brief A handle to a particle effect
class XII_PARTICLEPLUGIN_DLL xiiParticleEffectHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiParticleEffectHandle, xiiParticleEffectId);
};


struct XII_PARTICLEPLUGIN_DLL xiiParticleSystemState
{
  enum Enum
  {
    Active,
    EmittersFinished,
    OnlyReacting,
    Inactive,
  };
};

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamBinding
{
public:
  void UpdateBindings(const xiiProcessingStreamGroup* pGroup) const;
  void Clear() { m_Bindings.Clear(); }

private:
  friend class xiiParticleSystemInstance;

  struct Binding
  {
    xiiString             m_sName;
    xiiProcessingStream** m_ppStream;
  };

  xiiHybridArray<Binding, 4> m_Bindings;
};

//////////////////////////////////////////////////////////////////////////

struct XII_PARTICLEPLUGIN_DLL xiiParticleTypeRenderMode
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Additive,
    Blended,
    Opaque,
    Distortion,
    BlendedBackground,
    BlendedForeground,
    BlendAdd,
    Default = Additive
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PARTICLEPLUGIN_DLL, xiiParticleTypeRenderMode);

//////////////////////////////////////////////////////////////////////////

/// \brief What to do when an effect is not visible.
struct XII_PARTICLEPLUGIN_DLL xiiEffectInvisibleUpdateRate
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    FullUpdate,
    Max20fps,
    Max10fps,
    Max5fps,
    Pause,
    Discard,

    Default = Max10fps
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PARTICLEPLUGIN_DLL, xiiEffectInvisibleUpdateRate);

//////////////////////////////////////////////////////////////////////////

struct XII_PARTICLEPLUGIN_DLL xiiParticleTextureAtlasType
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    None,

    RandomVariations,
    FlipbookAnimation,
    RandomYAnimatedX,

    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PARTICLEPLUGIN_DLL, xiiParticleTextureAtlasType);

//////////////////////////////////////////////////////////////////////////

struct XII_PARTICLEPLUGIN_DLL xiiParticleColorGradientMode
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Age,
    Speed,

    Default = Age
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PARTICLEPLUGIN_DLL, xiiParticleColorGradientMode);


//////////////////////////////////////////////////////////////////////////

struct XII_PARTICLEPLUGIN_DLL xiiParticleOutOfBoundsMode
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Teleport,
    Die,

    Default = Teleport
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PARTICLEPLUGIN_DLL, xiiParticleOutOfBoundsMode);

//////////////////////////////////////////////////////////////////////////

struct xiiParticleEffectFloatParam
{
  XII_DECLARE_POD_TYPE();
  xiiHashedString m_sName;
  float           m_Value;
};

struct xiiParticleEffectColorParam
{
  XII_DECLARE_POD_TYPE();
  xiiHashedString m_sName;
  xiiColor        m_Value;
};

class xiiParticleEffectParameters final : public xiiRefCounted
{
public:
  xiiHybridArray<xiiParticleEffectFloatParam, 2> m_FloatParams;
  xiiHybridArray<xiiParticleEffectColorParam, 2> m_ColorParams;
};
