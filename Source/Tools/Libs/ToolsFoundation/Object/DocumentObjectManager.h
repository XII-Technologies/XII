/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiDocumentObjectManager;
class xiiDocument;

// Prevent conflicts with windows.h
#ifdef GetObject
#  undef GetObject
#endif

/// Standard root object for most documents.
/// m_RootObjects stores what is in the document and m_TempObjects stores transient data used during editing which is not part of the document.
class XII_TOOLSFOUNDATION_DLL xiiDocumentRoot : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentRoot, xiiReflectedClass);

  xiiHybridArray<xiiReflectedClass*, 1> m_RootObjects;
  xiiHybridArray<xiiReflectedClass*, 1> m_TempObjects;
};

/// Implementation detail of xiiDocumentObjectManager.
class xiiDocumentRootObject : public xiiDocumentStorageObject
{
public:
  xiiDocumentRootObject(const xiiRTTI* pRootType) :
    xiiDocumentStorageObject(pRootType)
  {
    m_Guid = xiiUuid::MakeStableUuidFromString("DocumentRoot");
  }

public:
  virtual void InsertSubObject(xiiDocumentObject* pObject, xiiStringView sProperty, const xiiVariant& index) override;
  virtual void RemoveSubObject(xiiDocumentObject* pObject) override;
};

/// Used by xiiDocumentObjectManager::m_StructureEvents.
struct xiiDocumentObjectStructureEvent
{
  xiiDocumentObjectStructureEvent() = default;

  const xiiAbstractProperty* GetProperty() const;
  xiiVariant                 getInsertIndex() const;
  enum class Type
  {
    BeforeReset,
    AfterReset,
    BeforeObjectAdded,
    AfterObjectAdded,
    BeforeObjectRemoved,
    AfterObjectRemoved,
    BeforeObjectMoved,
    AfterObjectMoved,
    AfterObjectMoved2,
  };

  Type                     m_EventType;
  const xiiDocument*       m_pDocument       = nullptr;
  const xiiDocumentObject* m_pObject         = nullptr;
  const xiiDocumentObject* m_pPreviousParent = nullptr;
  const xiiDocumentObject* m_pNewParent      = nullptr;
  xiiString                m_sParentProperty;
  xiiVariant               m_OldPropertyIndex;
  xiiVariant               m_NewPropertyIndex;
};

/// Used by xiiDocumentObjectManager::m_PropertyEvents.
struct xiiDocumentObjectPropertyEvent
{
  xiiDocumentObjectPropertyEvent() { m_pObject = nullptr; }
  xiiVariant getInsertIndex() const;

  enum class Type
  {
    PropertySet,
    PropertyInserted,
    PropertyRemoved,
    PropertyMoved,
  };

  Type                     m_EventType;
  const xiiDocumentObject* m_pObject;
  xiiVariant               m_OldValue;
  xiiVariant               m_NewValue;
  xiiString                m_sProperty;
  xiiVariant               m_OldIndex;
  xiiVariant               m_NewIndex;
};

/// Used by xiiDocumentObjectManager::m_ObjectEvents.
struct xiiDocumentObjectEvent
{
  xiiDocumentObjectEvent() { m_pObject = nullptr; }

  enum class Type
  {
    BeforeObjectDestroyed,
    AfterObjectCreated,
    Invalid
  };

  Type                     m_EventType = Type::Invalid;
  const xiiDocumentObject* m_pObject;
};

/// Represents to content of a document. Every document has exactly one root object under which all objects need to be parented. The default root object is xiiDocumentRoot.
class XII_TOOLSFOUNDATION_DLL xiiDocumentObjectManager
{
public:
  // Storage for the object manager so it can be swapped when using multiple sub documents.
  class Storage : public xiiRefCounted
  {
  public:
    Storage(const xiiRTTI* pRootType);

    xiiDocument*          m_pDocument = nullptr;
    xiiDocumentRootObject m_RootObject;

    xiiHashTable<xiiUuid, const xiiDocumentObject*> m_GuidToObject;

    mutable xiiCopyOnBroadcastEvent<const xiiDocumentObjectStructureEvent&> m_StructureEvents;
    mutable xiiCopyOnBroadcastEvent<const xiiDocumentObjectPropertyEvent&>  m_PropertyEvents;
    xiiEvent<const xiiDocumentObjectEvent&>                                 m_ObjectEvents;
  };

public:
  mutable xiiCopyOnBroadcastEvent<const xiiDocumentObjectStructureEvent&> m_StructureEvents;
  mutable xiiCopyOnBroadcastEvent<const xiiDocumentObjectPropertyEvent&>  m_PropertyEvents;
  xiiEvent<const xiiDocumentObjectEvent&>                                 m_ObjectEvents;

  xiiDocumentObjectManager(const xiiRTTI* pRootType = xiiDocumentRoot::GetStaticRTTI());
  virtual ~xiiDocumentObjectManager();
  void SetDocument(xiiDocument* pDocument) { m_pObjectStorage->m_pDocument = pDocument; }

  // Object Construction / Destruction
  // holds object data
  xiiDocumentObject* CreateObject(const xiiRTTI* pRtti, xiiUuid guid = xiiUuid());

  void         DestroyObject(xiiDocumentObject* pObject);
  virtual void DestroyAllObjects();
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const {};

  void PatchEmbeddedClassObjects(const xiiDocumentObject* pObject) const;

  const xiiDocumentObject* GetRootObject() const { return &m_pObjectStorage->m_RootObject; }
  xiiDocumentObject*       GetRootObject() { return &m_pObjectStorage->m_RootObject; }
  const xiiDocumentObject* GetObject(const xiiUuid& guid) const;
  xiiDocumentObject*       GetObject(const xiiUuid& guid);
  const xiiDocument*       GetDocument() const { return m_pObjectStorage->m_pDocument; }
  xiiDocument*             GetDocument() { return m_pObjectStorage->m_pDocument; }

  // Property Change
  xiiStatus SetValue(xiiDocumentObject* pObject, xiiStringView sProperty, const xiiVariant& newValue, xiiVariant index = xiiVariant());
  xiiStatus InsertValue(xiiDocumentObject* pObject, xiiStringView sProperty, const xiiVariant& newValue, xiiVariant index = xiiVariant());
  xiiStatus RemoveValue(xiiDocumentObject* pObject, xiiStringView sProperty, xiiVariant index = xiiVariant());
  xiiStatus MoveValue(xiiDocumentObject* pObject, xiiStringView sProperty, const xiiVariant& oldIndex, const xiiVariant& newIndex);

  // Structure Change
  void AddObject(xiiDocumentObject* pObject, xiiDocumentObject* pParent, xiiStringView sParentProperty, xiiVariant index);
  void RemoveObject(xiiDocumentObject* pObject);
  void MoveObject(xiiDocumentObject* pObject, xiiDocumentObject* pNewParent, xiiStringView sParentProperty, xiiVariant index);

  // Structure Change Test
  xiiStatus CanAdd(const xiiRTTI* pRtti, const xiiDocumentObject* pParent, xiiStringView sParentProperty, const xiiVariant& index) const;
  xiiStatus CanRemove(const xiiDocumentObject* pObject) const;
  xiiStatus CanMove(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, xiiStringView sParentProperty, const xiiVariant& index) const;
  xiiStatus CanSelect(const xiiDocumentObject* pObject) const;

  bool IsUnderRootProperty(xiiStringView sRootProperty, const xiiDocumentObject* pObject) const;
  bool IsUnderRootProperty(xiiStringView sRootProperty, const xiiDocumentObject* pParent, xiiStringView sParentProperty) const;
  bool IsTemporary(const xiiDocumentObject* pObject) const;
  bool IsTemporary(const xiiDocumentObject* pParent, xiiStringView sParentProperty) const;

  xiiSharedPtr<xiiDocumentObjectManager::Storage> SwapStorage(xiiSharedPtr<xiiDocumentObjectManager::Storage> pNewStorage);
  xiiSharedPtr<xiiDocumentObjectManager::Storage> GetStorage() { return m_pObjectStorage; }

private:
  virtual xiiDocumentObject* InternalCreateObject(const xiiRTTI* pRtti) { return XII_DEFAULT_NEW(xiiDocumentStorageObject, pRtti); }
  virtual void               InternalDestroyObject(xiiDocumentObject* pObject) { XII_DEFAULT_DELETE(pObject); }

  void InternalAddObject(xiiDocumentObject* pObject, xiiDocumentObject* pParent, xiiStringView sParentProperty, xiiVariant index);
  void InternalRemoveObject(xiiDocumentObject* pObject);
  void InternalMoveObject(xiiDocumentObject* pNewParent, xiiDocumentObject* pObject, xiiStringView sParentProperty, xiiVariant index);

  virtual xiiStatus InternalCanAdd(const xiiRTTI* pRtti, const xiiDocumentObject* pParent, xiiStringView sParentProperty, const xiiVariant& index) const
  {
    return XII_SUCCESS;
  };
  virtual xiiStatus InternalCanRemove(const xiiDocumentObject* pObject) const { return XII_SUCCESS; };
  virtual xiiStatus InternalCanMove(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, xiiStringView sParentProperty, const xiiVariant& index) const
  {
    return XII_SUCCESS;
  };
  virtual xiiStatus InternalCanSelect(const xiiDocumentObject* pObject) const { return XII_SUCCESS; };

  void RecursiveAddGuids(xiiDocumentObject* pObject);
  void RecursiveRemoveGuids(xiiDocumentObject* pObject);
  void PatchEmbeddedClassObjectsInternal(xiiDocumentObject* pObject, const xiiRTTI* pType, bool addToDoc);

private:
  friend class xiiObjectAccessorBase;

  xiiSharedPtr<xiiDocumentObjectManager::Storage> m_pObjectStorage;

  xiiCopyOnBroadcastEvent<const xiiDocumentObjectStructureEvent&>::Unsubscriber m_StructureEventsUnsubscriber;
  xiiCopyOnBroadcastEvent<const xiiDocumentObjectPropertyEvent&>::Unsubscriber  m_PropertyEventsUnsubscriber;
  xiiEvent<const xiiDocumentObjectEvent&>::Unsubscriber                         m_ObjectEventsUnsubscriber;
};
