#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStream, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleStreamFactory::xiiParticleStreamFactory(const char* szStreamName, xiiProcessingStream::DataType dataType, const xiiRTTI* pStreamTypeToCreate)
{
  m_szStreamName        = szStreamName;
  m_DataType            = dataType;
  m_pStreamTypeToCreate = pStreamTypeToCreate;
}

const xiiRTTI* xiiParticleStreamFactory::GetParticleStreamType() const
{
  return m_pStreamTypeToCreate;
}

xiiProcessingStream::DataType xiiParticleStreamFactory::GetStreamDataType() const
{
  return m_DataType;
}

const char* xiiParticleStreamFactory::GetStreamName() const
{
  return m_szStreamName;
}

void xiiParticleStreamFactory::GetFullStreamName(const char* szName, xiiProcessingStream::DataType type, xiiStringBuilder& out_sResult)
{
  out_sResult = szName;
  out_sResult.AppendFormat("({0})", (int)type);
}

xiiParticleStream* xiiParticleStreamFactory::CreateParticleStream(xiiParticleSystemInstance* pOwner) const
{
  const xiiRTTI* pRtti = GetParticleStreamType();
  XII_ASSERT_DEBUG(pRtti->IsDerivedFrom<xiiParticleStream>(), "Particle stream factory does not create a valid stream type");

  xiiParticleStream* pStream = pRtti->GetAllocator()->Allocate<xiiParticleStream>();

  pOwner->CreateStream(GetStreamName(), GetStreamDataType(), &pStream->m_pStream, pStream->m_StreamBinding, true);
  pStream->Initialize(pOwner);

  return pStream;
}

//////////////////////////////////////////////////////////////////////////

xiiParticleStream::xiiParticleStream()
{
  // make sure default stream initializers are run very first
  m_fPriority = -1000.0f;
}

xiiResult xiiParticleStream::UpdateStreamBindings()
{
  m_StreamBinding.UpdateBindings(m_pStreamGroup);
  return XII_SUCCESS;
}

void xiiParticleStream::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  const xiiUInt64 uiElementSize   = m_pStream->GetElementSize();
  const xiiUInt64 uiElementStride = m_pStream->GetElementStride();

  for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    xiiMemoryUtils::ZeroFill<xiiUInt8>(
      static_cast<xiiUInt8*>(xiiMemoryUtils::AddByteOffset(m_pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride))),
      static_cast<size_t>(uiElementSize));
  }
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_ParticleStream);
