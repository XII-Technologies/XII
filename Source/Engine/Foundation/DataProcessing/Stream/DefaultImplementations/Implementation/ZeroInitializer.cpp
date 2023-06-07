#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/Memory/MemoryUtils.h>

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>

#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcessingStreamSpawnerZeroInitialized, 1, xiiRTTIDefaultAllocator<xiiProcessingStreamSpawnerZeroInitialized>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiProcessingStreamSpawnerZeroInitialized::xiiProcessingStreamSpawnerZeroInitialized()

{
}

void xiiProcessingStreamSpawnerZeroInitialized::SetStreamName(xiiStringView sStreamName)
{
  m_sStreamName.Assign(sStreamName);
}

xiiResult xiiProcessingStreamSpawnerZeroInitialized::UpdateStreamBindings()
{
  XII_ASSERT_DEBUG(!m_sStreamName.IsEmpty(), "xiiProcessingStreamSpawnerZeroInitialized: Stream name has not been configured");

  m_pStream = m_pStreamGroup->GetStreamByName(m_sStreamName);
  return m_pStream ? XII_SUCCESS : XII_FAILURE;
}


void xiiProcessingStreamSpawnerZeroInitialized::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
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



XII_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_DefaultImplementations_Implementation_ZeroInitializer);
