#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using xiiCurve1DResourceHandle = xiiTypedResourceHandle<class xiiCurve1DResource>;

class XII_PARTICLEPLUGIN_DLL xiiParticleInitializerFactory_RandomRotationSpeed final : public xiiParticleInitializerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializerFactory_RandomRotationSpeed, xiiParticleInitializerFactory);

public:
  virtual const xiiRTTI* GetInitializerType() const override;
  virtual void           CopyInitializerProperties(xiiParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  bool                 m_bRandomStartAngle = false;
  xiiVarianceTypeAngle m_RotationSpeed;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleInitializer_RandomRotationSpeed final : public xiiParticleInitializer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializer_RandomRotationSpeed, xiiParticleInitializer);

public:
  bool                 m_bRandomStartAngle = false;
  xiiVarianceTypeAngle m_RotationSpeed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  bool                 m_bPositiveSign         = false;
  xiiProcessingStream* m_pStreamRotationSpeed  = nullptr;
  xiiProcessingStream* m_pStreamRotationOffset = nullptr;
};
