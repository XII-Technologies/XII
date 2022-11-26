#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiParticleStream;
class xiiParticleSystemInstance;

/// \brief Base class for all particle stream factories
class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory, xiiReflectedClass);

public:
  xiiParticleStreamFactory(const char* szStreamName, xiiProcessingStream::DataType dataType, const xiiRTTI* pStreamTypeToCreate);

  const xiiRTTI*                GetParticleStreamType() const;
  xiiProcessingStream::DataType GetStreamDataType() const;
  const char*                   GetStreamName() const;

  static void GetFullStreamName(const char* szName, xiiProcessingStream::DataType type, xiiStringBuilder& out_Result);

  xiiParticleStream* CreateParticleStream(xiiParticleSystemInstance* pOwner) const;

private:
  const char*                   m_szStreamName        = nullptr;
  xiiProcessingStream::DataType m_DataType            = xiiProcessingStream::DataType::Float;
  const xiiRTTI*                m_pStreamTypeToCreate = nullptr;
};

/// \brief Base class for all particle streams
class XII_PARTICLEPLUGIN_DLL xiiParticleStream : public xiiProcessingStreamProcessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStream, xiiProcessingStreamProcessor);

  friend class xiiParticleSystemInstance;
  friend class xiiParticleStreamFactory;

protected:
  xiiParticleStream();
  virtual void      Initialize(xiiParticleSystemInstance* pOwner) {}
  virtual xiiResult UpdateStreamBindings() final override;
  virtual void      Process(xiiUInt64 uiNumElements) final override {}

  /// \brief The default implementation initializes all data with zero.
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStream;

private:
  xiiParticleStreamBinding m_StreamBinding;
};
