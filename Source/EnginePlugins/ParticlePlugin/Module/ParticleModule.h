#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiProcessingStream;

class XII_PARTICLEPLUGIN_DLL xiiParticleModule : public xiiProcessingStreamProcessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleModule, xiiProcessingStreamProcessor);

  friend class xiiParticleSystemInstance;

public:
  virtual void CreateRequiredStreams() = 0;
  virtual void QueryOptionalStreams() {}

  void Reset(xiiParticleSystemInstance* pOwner)
  {
    m_pOwnerSystem = pOwner;
    m_StreamBinding.Clear();

    OnReset();
  }

  /// \brief Called after everything is set up.
  virtual void OnFinalize() {}

  xiiParticleSystemInstance* GetOwnerSystem() { return m_pOwnerSystem; }

  const xiiParticleSystemInstance* GetOwnerSystem() const { return m_pOwnerSystem; }

  xiiParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerSystem->GetOwnerEffect(); }

  /// \brief Override this to cache world module pointers for later (through xiiParticleWorldModule::GetCachedWorldModule()).
  virtual void RequestRequiredWorldModulesForCache(xiiParticleWorldModule* pParticleModule) {}

protected:
  /// \brief Called by Reset()
  virtual void OnReset() {}

  void CreateStream(const char* szName, xiiProcessingStream::DataType Type, xiiProcessingStream** ppStream, bool bWillInitializeStream)
  {
    m_pOwnerSystem->CreateStream(szName, Type, ppStream, m_StreamBinding, bWillInitializeStream);
  }

  virtual xiiResult UpdateStreamBindings() final override
  {
    m_StreamBinding.UpdateBindings(m_pStreamGroup);
    return XII_SUCCESS;
  }

  xiiRandom& GetRNG() const { return GetOwnerEffect()->GetRNG(); }

private:
  xiiParticleSystemInstance* m_pOwnerSystem;
  xiiParticleStreamBinding   m_StreamBinding;
};
