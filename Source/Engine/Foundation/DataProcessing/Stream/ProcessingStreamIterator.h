/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Memory/MemoryUtils.h>

/// Helper template class to iterate over stream elements.
template <typename Type>
class xiiProcessingStreamIterator
{
public:
  /// Constructor.
  xiiProcessingStreamIterator(const xiiProcessingStream* pStream, xiiUInt64 uiNumElements, xiiUInt64 uiStartIndex);

  /// Returns a reference to the current element. Note that the behavior is undefined if HasReachedEnd() is true!
  Type& Current() const;

  /// Returns true of the iterator has reached the end of the stream or the number of elements it should iterate over.
  bool HasReachedEnd() const;

  /// Advances the current pointer to the next element in the stream.
  void Advance();

  /// Advances the current pointer by the given number of elements.
  void Advance(xiiUInt32 uiNumElements);

  // TODO: Add iterator interface? Only makes really sense for element spawners and processors which work on a single stream

protected:
  void* m_pCurrentPtr = nullptr;
  void* m_pEndPtr     = nullptr;

  xiiUInt64 m_uiElementStride = 0;
};

#include <Foundation/DataProcessing/Stream/Implementation/ProcessingStreamIterator_inl.h>
