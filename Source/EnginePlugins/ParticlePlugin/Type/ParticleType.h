#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct xiiMsgExtractRenderData;

enum xiiParticleTypeSortingKey
{
  Distortion, // samples the back-buffer, so doing this later would overwrite their result
  Opaque,
  BlendedBackground,
  Additive,
  BlendAdd,
  Blended,
  BlendedForeground,
};

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeFactory : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeFactory, xiiReflectedClass);

public:
  virtual const xiiRTTI* GetTypeType() const                                                 = 0;
  virtual void           CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const = 0;

  xiiParticleType* CreateType(xiiParticleSystemInstance* pOwner) const;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const {}

  virtual void Save(xiiStreamWriter& inout_stream) const = 0;
  virtual void Load(xiiStreamReader& inout_stream)       = 0;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleType : public xiiParticleModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleType, xiiParticleModule);

  friend class xiiParticleSystemInstance;

public:
  virtual float GetMaxParticleRadius(float fParticleSize) const { return fParticleSize * 0.5f; }

  virtual void ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const = 0;

protected:
  xiiParticleType();

  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const xiiTime& tDiff, xiiUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  static xiiUInt32 ComputeSortingKey(xiiParticleTypeRenderMode::Enum mode, xiiUInt32 uiTextureHash);

  xiiTime           m_TimeDiff;
  mutable xiiUInt64 m_uiLastExtractedFrame;
};
