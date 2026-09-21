/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

class xiiProcessingStreamProcessor;
class xiiProcessingStreamGroup;

/// Event data that contains the stream group and the index of the element which is about to be removed.
struct xiiStreamGroupElementRemovedEvent
{
  xiiProcessingStreamGroup* m_pStreamGroup;
  xiiUInt64                 m_uiElementIndex;
};

/// Event data that contains the stream group for which all elements have been cleared.
struct xiiStreamGroupElementsClearedEvent
{
  xiiProcessingStreamGroup* m_pStreamGroup;
};

/// A stream group encapsulates the streams and the corresponding data processors.
class XII_FOUNDATION_DLL xiiProcessingStreamGroup
{
public:
  /// Constructor
  xiiProcessingStreamGroup();

  /// Destructor
  ~xiiProcessingStreamGroup();

  void Clear();

  /// Adds a stream processor to the stream group.
  /// Ownership is transferred to the stream group and the processor will be deallocated using the RTTI deallocator on destruction.
  /// Processors are executed in the order they are added to the stream group.
  void AddProcessor(xiiProcessingStreamProcessor* pProcessor);

  /// Removes the given stream processor from the group.
  void RemoveProcessor(xiiProcessingStreamProcessor* pProcessor);

  /// Removes all stream processors from the group.
  void ClearProcessors();

  /// Adds a stream with the given name to the stream group. Adding a stream two times with the same name will return nullptr for the second
  /// attempt to signal an error.
  xiiProcessingStream* AddStream(xiiStringView sName, xiiProcessingStream::DataType type);

  /// Removes the stream with the given name, if it exists.
  void RemoveStreamByName(xiiTempHashedString sName);

  /// Returns the stream by it's name, returns nullptr if not existent. More efficient since direct use of xiiHashedString.
  xiiProcessingStream* GetStreamByName(xiiTempHashedString sName) const;

  /// Resizes all streams to contain storage for uiNumElements. Any pending remove and spawn operations will be reset!
  void SetSize(xiiUInt64 uiNumElements);

  /// Removes an element (e.g. due to the death of a particle etc.), this will be enqueued (and thus is safe to be called from within data
  /// processors).
  void RemoveElement(xiiUInt64 uiElementIndex);

  /// Spawns a number of new elements, they will be added as newly initialized stream elements. Safe to call from data processors since the
  /// spawning will be queued.
  void InitializeElements(xiiUInt64 uiNumElements);

  /// Runs the stream processors which have been added to the stream group.
  void Process();

  /// Returns the number of elements the streams store.
  inline xiiUInt64 GetNumElements() const { return m_uiNumElements; }

  /// Returns the number of currently active elements.
  inline xiiUInt64 GetNumActiveElements() const { return m_uiNumActiveElements; }

  /// Returns the highest number of active elements since the last SetSize() call.
  inline xiiUInt64 GetHighestNumActiveElements() const { return m_uiHighestNumActiveElements; }

  /// Subscribe to this event to be informed when (shortly before) items are deleted.
  xiiEvent<const xiiStreamGroupElementRemovedEvent&> m_ElementRemovedEvent;

private:
  /// Internal helper function which removes any pending elements and spawns new elements as needed
  void RunPendingDeletions();

  void EnsureStreamAssignmentValid();

  void RunPendingSpawns();

  void SortProcessorsByPriority();

  xiiHybridArray<xiiProcessingStreamProcessor*, 8> m_Processors;

  xiiHybridArray<xiiProcessingStream*, 8> m_DataStreams;

  xiiHybridArray<xiiUInt64, 64> m_PendingRemoveIndices;

  xiiUInt64 m_uiPendingNumberOfElementsToSpawn;

  xiiUInt64 m_uiNumElements;

  xiiUInt64 m_uiNumActiveElements;

  xiiUInt64 m_uiHighestNumActiveElements;

  bool m_bStreamAssignmentDirty;
};
