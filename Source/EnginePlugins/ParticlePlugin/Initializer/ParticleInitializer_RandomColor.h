#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using xiiColorGradientResourceHandle = xiiTypedResourceHandle<class xiiColorGradientResource>;

class XII_PARTICLEPLUGIN_DLL xiiParticleInitializerFactory_RandomColor final : public xiiParticleInitializerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializerFactory_RandomColor, xiiParticleInitializerFactory);

public:
  virtual const xiiRTTI* GetInitializerType() const override;
  virtual void           CopyInitializerProperties(xiiParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  void                    SetColorGradient(const xiiColorGradientResourceHandle& hResource) { m_hGradient = hResource; }
  XII_ALWAYS_INLINE const xiiColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  void        SetColorGradientFile(const char* szFile);
  const char* GetColorGradientFile() const;

  xiiColor m_Color1;
  xiiColor m_Color2;

private:
  xiiColorGradientResourceHandle m_hGradient;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleInitializer_RandomColor final : public xiiParticleInitializer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializer_RandomColor, xiiParticleInitializer);

public:
  xiiColor m_Color1;
  xiiColor m_Color2;

  xiiColorGradientResourceHandle m_hGradient;


  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamColor;
};
