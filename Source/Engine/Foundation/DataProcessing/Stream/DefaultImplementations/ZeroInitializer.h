/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

class xiiProcessingStream;

/// This element spawner initializes new elements with 0 (by writing 0 bytes into the whole element).
class XII_FOUNDATION_DLL xiiProcessingStreamSpawnerZeroInitialized : public xiiProcessingStreamProcessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcessingStreamSpawnerZeroInitialized, xiiProcessingStreamProcessor);

public:
  xiiProcessingStreamSpawnerZeroInitialized();

  /// Which stream to zero initialize
  void SetStreamName(xiiStringView sStreamName);

protected:
  virtual xiiResult UpdateStreamBindings() override;

  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
  virtual void Process(xiiUInt64 uiNumElements) override { XII_IGNORE_UNUSED(uiNumElements); }

  xiiHashedString m_sStreamName;

  xiiProcessingStream* m_pStream = nullptr;
};
