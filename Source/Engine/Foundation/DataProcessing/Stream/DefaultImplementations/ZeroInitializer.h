
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

class xiiProcessingStream;

/// \brief This element spawner initializes new elements with 0 (by writing 0 bytes into the whole element)
class XII_FOUNDATION_DLL xiiProcessingStreamSpawnerZeroInitialized : public xiiProcessingStreamProcessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcessingStreamSpawnerZeroInitialized, xiiProcessingStreamProcessor);

public:
  xiiProcessingStreamSpawnerZeroInitialized();

  /// \brief Which stream to zero initialize
  void SetStreamName(xiiStringView szStreamName);

protected:
  virtual xiiResult UpdateStreamBindings() override;

  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
  virtual void Process(xiiUInt64 uiNumElements) override {}

  xiiHashedString m_sStreamName;

  xiiProcessingStream* m_pStream;
};
