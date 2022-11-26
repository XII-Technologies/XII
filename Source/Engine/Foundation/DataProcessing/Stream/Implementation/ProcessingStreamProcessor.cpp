#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcessingStreamProcessor, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiProcessingStreamProcessor::xiiProcessingStreamProcessor() :
  m_pStreamGroup(nullptr)
{
}

xiiProcessingStreamProcessor::~xiiProcessingStreamProcessor()
{
  m_pStreamGroup = nullptr;
}



XII_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_Implementation_ProcessingStreamProcessor);
