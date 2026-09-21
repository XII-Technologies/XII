/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

/// Stores meta data for document objects that is not part of the object itself. E.g. editor-only states like hidden or prefab information.
/// \tparam KEY The key under which data is stored. Usually xiiUuid to reference document objects.
/// \tparam VALUE Meta value type to be stored.
template <typename KEY, typename VALUE>
class xiiObjectMetaData
{
public:
  struct EventData
  {
    KEY          m_ObjectKey;
    const VALUE* m_pValue;
    xiiUInt32    m_uiModifiedFlags;
  };

  xiiEvent<const EventData&> m_DataModifiedEvent;

  // Storage for the meta data so it can be swapped when using multiple sub documents.
  class Storage : public xiiRefCounted
  {
  public:
    mutable enum class AccessMode { Nothing,
                                    Read,
                                    Write } m_AccessMode;
    mutable KEY              m_AcessingKey;
    mutable xiiMutex         m_Mutex;
    xiiHashTable<KEY, VALUE> m_MetaData;

    xiiEvent<const EventData&> m_DataModifiedEvent;
  };

  xiiObjectMetaData();

  bool HasMetaData(const KEY objectKey) const;

  void ClearMetaData(const KEY objectKey);

  /// Will always return a non-null result. May be a default object.
  const VALUE* BeginReadMetaData(const KEY objectKey) const;
  void         EndReadMetaData() const;

  VALUE* BeginModifyMetaData(const KEY objectKey);
  void   EndModifyMetaData(xiiUInt32 uiModifiedFlags = 0xFFFFFFFF);


  xiiMutex& GetMutex() const { return m_pMetaStorage->m_Mutex; }

  const VALUE& GetDefaultValue() const { return m_DefaultValue; }

  /// Uses reflection information from VALUE to store all properties that differ from the default value as additional properties for the graph
  /// objects.
  void AttachMetaDataToAbstractGraph(xiiAbstractObjectGraph& inout_graph) const;

  /// Uses reflection information from VALUE to restore all meta data properties from the graph.
  void RestoreMetaDataFromAbstractGraph(const xiiAbstractObjectGraph& graph);

  xiiSharedPtr<xiiObjectMetaData<KEY, VALUE>::Storage> SwapStorage(xiiSharedPtr<xiiObjectMetaData<KEY, VALUE>::Storage> pNewStorage);
  xiiSharedPtr<xiiObjectMetaData<KEY, VALUE>::Storage> GetStorage() { return m_pMetaStorage; }

private:
  VALUE                                                m_DefaultValue;
  xiiSharedPtr<xiiObjectMetaData<KEY, VALUE>::Storage> m_pMetaStorage;
  typename xiiEvent<const EventData&>::Unsubscriber    m_EventsUnsubscriber;
};

#include <ToolsFoundation/Object/Implementation/ObjectMetaData_inl.h>
