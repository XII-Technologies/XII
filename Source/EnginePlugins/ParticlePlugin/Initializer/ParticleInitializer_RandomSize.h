#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using xiiCurve1DResourceHandle = xiiTypedResourceHandle<class xiiCurve1DResource>;

class XII_PARTICLEPLUGIN_DLL xiiParticleInitializerFactory_RandomSize final : public xiiParticleInitializerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializerFactory_RandomSize, xiiParticleInitializerFactory);

public:
  virtual const xiiRTTI* GetInitializerType() const override;
  virtual void           CopyInitializerProperties(xiiParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  void        SetSizeCurveFile(const char* szFile);
  const char* GetSizeCurveFile() const;

  xiiVarianceTypeFloat     m_Size;
  xiiCurve1DResourceHandle m_hCurve;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleInitializer_RandomSize final : public xiiParticleInitializer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializer_RandomSize, xiiParticleInitializer);

public:
  xiiVarianceTypeFloat     m_Size;
  xiiCurve1DResourceHandle m_hCurve;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamSize;
};
