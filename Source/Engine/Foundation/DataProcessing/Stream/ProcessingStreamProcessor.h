/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>

class xiiProcessingStreamGroup;

/// Base class for all stream processor implementations.
class XII_FOUNDATION_DLL xiiProcessingStreamProcessor : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcessingStreamProcessor, xiiReflectedClass);

public:
  /// Base constructor
  xiiProcessingStreamProcessor();

  /// Base destructor.
  virtual ~xiiProcessingStreamProcessor();

  /// Used for sorting processors, to ensure a certain order. Lower priority == executed first.
  float m_fPriority = 0.0f;

protected:
  friend class xiiProcessingStreamGroup;

  /// Internal method which needs to be implemented, gets the concrete stream bindings.
  /// This is called every time the streams are resized. Implementations should check that their required streams exist and are of the correct data
  /// types.
  virtual xiiResult UpdateStreamBindings() = 0;

  /// This method needs to be implemented in order to initialize new elements to specific values.
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) = 0;

  /// The actual method which processes the data, will be called with the number of elements to process.
  virtual void Process(xiiUInt64 uiNumElements) = 0;

  /// Back pointer to the stream group - will be set to the owner stream group when adding the stream processor to the group.
  /// Can be used to get stream pointers in UpdateStreamBindings();
  xiiProcessingStreamGroup* m_pStreamGroup = nullptr;
};
