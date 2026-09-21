/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/TagSet.h>

/// Stores an entire xiiWorld in a stream.
///
/// Used for exporting a world in binary form either as a level or as a prefab (though there is no
/// difference).
/// Can be used for saving a game, if the exact state of the world shall be stored (e.g. like in an FPS).
class XII_CORE_DLL xiiWorldWriter
{
public:
  /// Writes all content in \a world to \a stream.
  ///
  /// All game objects with tags that overlap with \a pExclude will be ignored.
  void WriteWorld(xiiStreamWriter& ref_stream, xiiWorld& ref_world, const xiiTagSet* pExclude = nullptr);

  /// Only writes the given root objects and all their children to the stream.
  void WriteObjects(xiiStreamWriter& ref_stream, const xiiDeque<const xiiGameObject*>& rootObjects);

  /// Only writes the given root objects and all their children to the stream.
  void WriteObjects(xiiStreamWriter& ref_stream, xiiArrayPtr<const xiiGameObject*> rootObjects);

  /// Writes the given game object handle to the stream.
  ///
  /// \note If the handle belongs to an object that is not part of the serialized scene, e.g. an object
  /// that was excluded by a tag, this function will assert.
  void WriteGameObjectHandle(const xiiGameObjectHandle& hObject);

  /// Writes the given component handle to the stream.
  ///
  /// \note If the handle belongs to a component that is not part of the serialized scene, e.g. an object
  /// that was excluded by a tag, this function will assert.
  void WriteComponentHandle(const xiiComponentHandle& hComponent);

  /// Accesses the stream to which data is written. Use this in component serialization functions
  /// to write data to the stream.
  xiiStreamWriter& GetStream() const { return *m_pStream; }

  /// Returns an array containing all game object pointers that were written to the stream as root objects
  const xiiDeque<const xiiGameObject*>& GetAllWrittenRootObjects() const { return m_AllRootObjects; }

  /// Returns an array containing all game object pointers that were written to the stream as child objects
  const xiiDeque<const xiiGameObject*>& GetAllWrittenChildObjects() const { return m_AllChildObjects; }

private:
  void      Clear();
  xiiResult WriteToStream();
  void      AssignGameObjectIndices();
  void      AssignComponentHandleIndices(const xiiMap<xiiString, const xiiRTTI*>& sortedTypes);
  void      IncludeAllComponentBaseTypes();
  void      IncludeAllComponentBaseTypes(const xiiRTTI* pRtti);
  void      Traverse(xiiGameObject* pObject);

  xiiVisitorExecution::Enum ObjectTraverser(xiiGameObject* pObject);
  void                      WriteGameObject(const xiiGameObject* pObject);
  void                      WriteComponentTypeInfo(const xiiRTTI* pRtti);
  void                      WriteComponentCreationData(const xiiDeque<const xiiComponent*>& components);
  void                      WriteComponentSerializationData(const xiiDeque<const xiiComponent*>& components);

  xiiStreamWriter* m_pStream  = nullptr;
  const xiiTagSet* m_pExclude = nullptr;

  xiiDeque<const xiiGameObject*>         m_AllRootObjects;
  xiiDeque<const xiiGameObject*>         m_AllChildObjects;
  xiiMap<xiiGameObjectHandle, xiiUInt32> m_WrittenGameObjectHandles;

  struct Components
  {
    xiiUInt16                             m_uiSerializedTypeIndex = 0;
    xiiDeque<const xiiComponent*>         m_Components;
    xiiMap<xiiComponentHandle, xiiUInt32> m_HandleToIndex;
  };

  xiiHashTable<const xiiRTTI*, Components> m_AllComponents;
};
