#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

////////////////////////////////////////////////////////////////////////
// xiiDocumentObjectManager
////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentRoot, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Children", m_RootObjects)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_ARRAY_MEMBER_PROPERTY("TempObjects", m_TempObjects)->AddFlags(xiiPropertyFlags::PointerOwner)->AddAttributes(new xiiTemporaryAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiDocumentRootObject::InsertSubObject(xiiDocumentObject* pObject, const char* szProperty, const xiiVariant& index)
{
  if (xiiStringUtils::IsNullOrEmpty(szProperty))
    szProperty = "Children";
  return xiiDocumentObject::InsertSubObject(pObject, szProperty, index);
}

void xiiDocumentRootObject::RemoveSubObject(xiiDocumentObject* pObject)
{
  return xiiDocumentObject::RemoveSubObject(pObject);
}

xiiVariant xiiDocumentObjectPropertyEvent::getInsertIndex() const
{
  if (m_EventType == Type::PropertyMoved)
  {
    const xiiIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    const xiiRTTI*                   pType    = accessor.GetType();
    auto*                            pProp    = pType->FindPropertyByName(m_sProperty);
    if (pProp->GetCategory() == xiiPropertyCategory::Array || pProp->GetCategory() == xiiPropertyCategory::Set)
    {
      xiiInt32 iCurrentIndex = m_OldIndex.ConvertTo<xiiInt32>();
      xiiInt32 iNewIndex     = m_NewIndex.ConvertTo<xiiInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        return xiiVariant(iNewIndex);
      }
    }
  }
  return m_NewIndex;
}

xiiDocumentObjectManager::Storage::Storage(const xiiRTTI* pRootType) :
  m_RootObject(pRootType)
{
}

xiiDocumentObjectManager::xiiDocumentObjectManager(const xiiRTTI* pRootType)
{
  auto pStorage                                   = XII_DEFAULT_NEW(Storage, pRootType);
  pStorage->m_RootObject.m_pDocumentObjectManager = this;
  SwapStorage(pStorage);
}

xiiDocumentObjectManager::~xiiDocumentObjectManager()
{
  if (m_pObjectStorage->GetRefCount() == 1)
  {
    XII_ASSERT_DEV(m_pObjectStorage->m_GuidToObject.IsEmpty(), "Not all objects have been destroyed!");
  }
}

////////////////////////////////////////////////////////////////////////
// xiiDocumentObjectManager Object Construction / Destruction
////////////////////////////////////////////////////////////////////////

xiiDocumentObject* xiiDocumentObjectManager::CreateObject(const xiiRTTI* pRtti, xiiUuid guid)
{
  XII_ASSERT_DEV(pRtti != nullptr, "Unknown RTTI type");

  xiiDocumentObject* pObject = InternalCreateObject(pRtti);
  // In case the storage is swapped, objects should still be created in their original document manager.
  pObject->m_pDocumentObjectManager = m_pObjectStorage->m_RootObject.GetDocumentObjectManager();

  if (guid.IsValid())
    pObject->m_Guid = guid;
  else
    pObject->m_Guid.CreateNewUuid();

  PatchEmbeddedClassObjectsInternal(pObject, pRtti, false);

  xiiDocumentObjectEvent e;
  e.m_pObject   = pObject;
  e.m_EventType = xiiDocumentObjectEvent::Type::AfterObjectCreated;
  m_pObjectStorage->m_ObjectEvents.Broadcast(e);

  return pObject;
}

void xiiDocumentObjectManager::DestroyObject(xiiDocumentObject* pObject)
{
  for (xiiDocumentObject* pChild : pObject->m_Children)
  {
    DestroyObject(pChild);
  }

  xiiDocumentObjectEvent e;
  e.m_pObject   = pObject;
  e.m_EventType = xiiDocumentObjectEvent::Type::BeforeObjectDestroyed;
  m_pObjectStorage->m_ObjectEvents.Broadcast(e);

  InternalDestroyObject(pObject);
}

void xiiDocumentObjectManager::DestroyAllObjects()
{
  for (auto child : m_pObjectStorage->m_RootObject.m_Children)
  {
    DestroyObject(child);
  }

  m_pObjectStorage->m_RootObject.m_Children.Clear();
  m_pObjectStorage->m_GuidToObject.Clear();
}

void xiiDocumentObjectManager::PatchEmbeddedClassObjects(const xiiDocumentObject* pObject) const
{
  // Functional should be callable from anywhere but will of course have side effects.
  const_cast<xiiDocumentObjectManager*>(this)->PatchEmbeddedClassObjectsInternal(
    const_cast<xiiDocumentObject*>(pObject), pObject->GetTypeAccessor().GetType(), true);
}

const xiiDocumentObject* xiiDocumentObjectManager::GetObject(const xiiUuid& guid) const
{
  const xiiDocumentObject* pObject = nullptr;
  if (m_pObjectStorage->m_GuidToObject.TryGetValue(guid, pObject))
  {
    return pObject;
  }
  else if (guid == m_pObjectStorage->m_RootObject.GetGuid())
    return &m_pObjectStorage->m_RootObject;
  return nullptr;
}

xiiDocumentObject* xiiDocumentObjectManager::GetObject(const xiiUuid& guid)
{
  return const_cast<xiiDocumentObject*>(((const xiiDocumentObjectManager*)this)->GetObject(guid));
}

////////////////////////////////////////////////////////////////////////
// xiiDocumentObjectManager Property Change
////////////////////////////////////////////////////////////////////////

xiiStatus xiiDocumentObjectManager::SetValue(xiiDocumentObject* pObject, const char* szProperty, const xiiVariant& newValue, xiiVariant index)
{
  XII_ASSERT_DEBUG(pObject, "Object must not be null.");
  xiiIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  xiiVariant                 oldValue = accessor.GetValue(szProperty, index);

  if (!accessor.SetValue(szProperty, newValue, index))
  {
    return xiiStatus(xiiFmt("Set Property: The property '{0}' does not exist or value type does not match", szProperty));
  }

  xiiDocumentObjectPropertyEvent e;
  e.m_EventType = xiiDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_pObject   = pObject;
  e.m_OldValue  = oldValue;
  e.m_NewValue  = newValue;
  e.m_sProperty = szProperty;
  e.m_NewIndex  = index;

  // Allow a recursion depth of 2 for property setters. This allowed for two levels of side-effects on property setters.
  m_pObjectStorage->m_PropertyEvents.Broadcast(e, 2);
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiDocumentObjectManager::InsertValue(xiiDocumentObject* pObject, const char* szProperty, const xiiVariant& newValue, xiiVariant index)
{
  xiiIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  if (!accessor.InsertValue(szProperty, index, newValue))
  {
    if (!accessor.GetType()->FindPropertyByName(szProperty))
    {
      return xiiStatus(xiiFmt("Insert Property: The property '{0}' does not exist", szProperty));
    }
    return xiiStatus(xiiFmt("Insert Property: The property '{0}' already has the key '{1}'", szProperty, index));
  }

  xiiDocumentObjectPropertyEvent e;
  e.m_EventType = xiiDocumentObjectPropertyEvent::Type::PropertyInserted;
  e.m_pObject   = pObject;
  e.m_NewValue  = newValue;
  e.m_NewIndex  = index;
  e.m_sProperty = szProperty;

  m_pObjectStorage->m_PropertyEvents.Broadcast(e);

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiDocumentObjectManager::RemoveValue(xiiDocumentObject* pObject, const char* szProperty, xiiVariant index)
{
  xiiIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  xiiVariant                 oldValue = accessor.GetValue(szProperty, index);

  if (!accessor.RemoveValue(szProperty, index))
  {
    return xiiStatus(xiiFmt("Remove Property: The index '{0}' in property '{1}' does not exist!", index.ConvertTo<xiiString>(), szProperty));
  }

  xiiDocumentObjectPropertyEvent e;
  e.m_EventType = xiiDocumentObjectPropertyEvent::Type::PropertyRemoved;
  e.m_pObject   = pObject;
  e.m_OldValue  = oldValue;
  e.m_OldIndex  = index;
  e.m_sProperty = szProperty;

  m_pObjectStorage->m_PropertyEvents.Broadcast(e);

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiDocumentObjectManager::MoveValue(xiiDocumentObject* pObject, const char* szProperty, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  if (!oldIndex.CanConvertTo<xiiInt32>() || !newIndex.CanConvertTo<xiiInt32>())
    return xiiStatus("Move Property: Invalid indices provided.");

  xiiIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  xiiInt32                   iCount   = accessor.GetCount(szProperty);
  if (iCount < 0)
    return xiiStatus("Move Property: Invalid property.");
  if (oldIndex.ConvertTo<xiiInt32>() < 0 || oldIndex.ConvertTo<xiiInt32>() >= iCount)
    return xiiStatus(xiiFmt("Move Property: Invalid old index '{0}'.", oldIndex.ConvertTo<xiiInt32>()));
  if (newIndex.ConvertTo<xiiInt32>() < 0 || newIndex.ConvertTo<xiiInt32>() > iCount)
    return xiiStatus(xiiFmt("Move Property: Invalid new index '{0}'.", newIndex.ConvertTo<xiiInt32>()));

  if (!accessor.MoveValue(szProperty, oldIndex, newIndex))
    return xiiStatus("Move Property: Move value failed.");

  {
    xiiDocumentObjectPropertyEvent e;
    e.m_EventType = xiiDocumentObjectPropertyEvent::Type::PropertyMoved;
    e.m_pObject   = pObject;
    e.m_OldIndex  = oldIndex;
    e.m_NewIndex  = newIndex;
    e.m_sProperty = szProperty;
    e.m_NewValue  = accessor.GetValue(szProperty, e.getInsertIndex());
    // NewValue can be invalid if an invalid variant in a variant array is moved
    // XII_ASSERT_DEV(e.m_NewValue.IsValid(), "Value at new pos should be valid now, index missmatch?");
    m_pObjectStorage->m_PropertyEvents.Broadcast(e);
  }

  return xiiStatus(XII_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// xiiDocumentObjectManager Structure Change
////////////////////////////////////////////////////////////////////////

void xiiDocumentObjectManager::AddObject(xiiDocumentObject* pObject, xiiDocumentObject* pParent, const char* szParentProperty, xiiVariant index)
{
  if (pParent == nullptr)
    pParent = &m_pObjectStorage->m_RootObject;
  if (pParent == &m_pObjectStorage->m_RootObject && xiiStringUtils::IsNullOrEmpty(szParentProperty))
    szParentProperty = "Children";

  XII_ASSERT_DEV(pObject->GetGuid().IsValid(), "Object Guid invalid! Object was not created via a xiiObjectManagerBase!");
  XII_ASSERT_DEV(
    CanAdd(pObject->GetTypeAccessor().GetType(), pParent, szParentProperty, index).m_Result.Succeeded(), "Trying to execute invalid add!");

  InternalAddObject(pObject, pParent, szParentProperty, index);
}

void xiiDocumentObjectManager::RemoveObject(xiiDocumentObject* pObject)
{
  XII_ASSERT_DEV(CanRemove(pObject).m_Result.Succeeded(), "Trying to execute invalid remove!");
  InternalRemoveObject(pObject);
}

void xiiDocumentObjectManager::MoveObject(xiiDocumentObject* pObject, xiiDocumentObject* pNewParent, const char* szParentProperty, xiiVariant index)
{
  XII_ASSERT_DEV(CanMove(pObject, pNewParent, szParentProperty, index).m_Result.Succeeded(), "Trying to execute invalid move!");

  InternalMoveObject(pNewParent, pObject, szParentProperty, index);
}


////////////////////////////////////////////////////////////////////////
// xiiDocumentObjectManager Structure Change Test
////////////////////////////////////////////////////////////////////////

xiiStatus xiiDocumentObjectManager::CanAdd(
  const xiiRTTI*           pRtti,
  const xiiDocumentObject* pParent,
  const char*              szParentProperty,
  const xiiVariant&        index) const
{
  // Test whether parent exists in tree.
  if (pParent == GetRootObject())
    pParent = nullptr;

  if (pParent != nullptr)
  {
    const xiiDocumentObject* pObjectInTree = GetObject(pParent->GetGuid());
    XII_ASSERT_DEV(pObjectInTree == pParent, "Tree Corruption!!!");
    if (pObjectInTree == nullptr)
      return xiiStatus("Parent is not part of the object manager!");

    const xiiIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    const xiiRTTI*                   pType    = accessor.GetType();
    auto*                            pProp    = pType->FindPropertyByName(szParentProperty);
    if (pProp == nullptr)
      return xiiStatus(xiiFmt("Property '{0}' could not be found in type '{1}'", szParentProperty, pType->GetTypeName()));

    const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp);

    if (bIsValueType || pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
    {
      return xiiStatus("Need to use 'InsertValue' action instead.");
    }
    else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
    {
      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        if (!pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          return xiiStatus(xiiFmt("Cannot add object to the pointer property '{0}' as it does not hold ownership.", szParentProperty));

        if (!pRtti->IsDerivedFrom(pProp->GetSpecificType()))
          return xiiStatus(xiiFmt("Cannot add object to the pointer property '{0}' as its type '{1}' is not derived from the property type '{2}'!",
                                  szParentProperty, pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName()));
      }
      else
      {
        if (pRtti != pProp->GetSpecificType())
          return xiiStatus(xiiFmt("Cannot add object to the property '{0}' as its type '{1}' does not match the property type '{2}'!", szParentProperty,
                                  pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName()));
      }
    }

    if (pProp->GetCategory() == xiiPropertyCategory::Array || pProp->GetCategory() == xiiPropertyCategory::Set)
    {
      xiiInt32 iCount = accessor.GetCount(szParentProperty);
      if (!index.CanConvertTo<xiiInt32>())
      {
        return xiiStatus(xiiFmt("Cannot add object to the property '{0}', the given index is an invalid xiiVariant (Either use '-1' to append "
                                "or a valid index).",
                                szParentProperty));
      }
      xiiInt32 iNewIndex = index.ConvertTo<xiiInt32>();
      if (iNewIndex > (xiiInt32)iCount)
        return xiiStatus(xiiFmt(
          "Cannot add object to its new location '{0}' is out of the bounds of the parent's property range '{1}'!", iNewIndex, (xiiInt32)iCount));
      if (iNewIndex < 0 && iNewIndex != -1)
        return xiiStatus(xiiFmt("Cannot add object to the property '{0}', the index '{1}' is not valid (Either use '-1' to append or a valid index).",
                                szParentProperty, iNewIndex));
    }
    if (pProp->GetCategory() == xiiPropertyCategory::Map)
    {
      if (!index.IsA<xiiString>())
        return xiiStatus(xiiFmt("Cannot add object to the map property '{0}' as its index type is not a string.", szParentProperty));
      xiiVariant value = accessor.GetValue(szParentProperty, index);
      if (value.IsValid() && value.IsA<xiiUuid>())
      {
        xiiUuid guid = value.Get<xiiUuid>();
        if (guid.IsValid())
          return xiiStatus(
            xiiFmt("Cannot add object to the map property '{0}' at key '{1}'. Delete old value first.", szParentProperty, index.Get<xiiString>()));
      }
    }
    else if (pProp->GetCategory() == xiiPropertyCategory::Member)
    {
      if (!pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        return xiiStatus("Embedded classes cannot be changed manually.");

      xiiVariant value = accessor.GetValue(szParentProperty);
      if (!value.IsA<xiiUuid>())
        return xiiStatus("Property is not a pointer and thus can't be added to.");

      if (value.Get<xiiUuid>().IsValid())
        return xiiStatus("Can't set pointer if it already has a value, need to delete value first.");
    }
  }

  return InternalCanAdd(pRtti, pParent, szParentProperty, index);
}

xiiStatus xiiDocumentObjectManager::CanRemove(const xiiDocumentObject* pObject) const
{
  const xiiDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return xiiStatus("Object is not part of the object manager!");

  if (pObject->GetParent())
  {
    xiiAbstractProperty* pProp = pObject->GetParentPropertyType();
    XII_ASSERT_DEV(pProp != nullptr, "Parent property should always be valid!");
    if (pProp->GetCategory() == xiiPropertyCategory::Member && !pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      return xiiStatus("Non pointer members can't be deleted!");
  }
  XII_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  return InternalCanRemove(pObject);
}

xiiStatus xiiDocumentObjectManager::CanMove(
  const xiiDocumentObject* pObject,
  const xiiDocumentObject* pNewParent,
  const char*              szParentProperty,
  const xiiVariant&        index) const
{
  XII_SUCCEED_OR_RETURN(CanAdd(pObject->GetTypeAccessor().GetType(), pNewParent, szParentProperty, index));

  XII_SUCCEED_OR_RETURN(CanRemove(pObject));

  if (pNewParent == nullptr)
    pNewParent = GetRootObject();

  if (pObject == pNewParent)
    return xiiStatus("Can't move object onto itself!");

  const xiiDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return xiiStatus("Object is not part of the object manager!");

  XII_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  if (pNewParent != GetRootObject())
  {
    const xiiDocumentObject* pNewParentInTree = GetObject(pNewParent->GetGuid());

    if (pNewParentInTree == nullptr)
      return xiiStatus("New parent is not part of the object manager!");

    XII_ASSERT_DEV(pNewParentInTree == pNewParent, "Tree Corruption!!!");
  }

  const xiiDocumentObject* pCurParent = pNewParent->GetParent();

  while (pCurParent)
  {
    if (pCurParent == pObject)
      return xiiStatus("Can't move object to one of its children!");

    pCurParent = pCurParent->GetParent();
  }

  const xiiIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
  const xiiRTTI*                   pType    = accessor.GetType();

  auto* pProp = pType->FindPropertyByName(szParentProperty);

  if (pProp == nullptr)
    return xiiStatus(xiiFmt("Property '{0}' could not be found in type '{1}'", szParentProperty, pType->GetTypeName()));

  if (pProp->GetCategory() == xiiPropertyCategory::Array || pProp->GetCategory() == xiiPropertyCategory::Set)
  {
    xiiInt32 iChildIndex = index.ConvertTo<xiiInt32>();
    if (iChildIndex == -1)
    {
      iChildIndex = pNewParent->GetTypeAccessor().GetCount(szParentProperty);
    }

    if (pNewParent == pObject->GetParent())
    {
      // Test whether we are moving before or after ourselves, both of which are not allowed and would not change the tree.
      xiiIReflectedTypeAccessor& oldAccessor   = pObject->m_pParent->GetTypeAccessor();
      xiiInt32                   iCurrentIndex = oldAccessor.GetPropertyChildIndex(szParentProperty, pObject->GetGuid()).ConvertTo<xiiInt32>();
      if (iChildIndex == iCurrentIndex || iChildIndex == iCurrentIndex + 1)
        return xiiStatus("Can't move object onto itself!");
    }
  }
  if (pProp->GetCategory() == xiiPropertyCategory::Map)
  {
    if (!index.IsA<xiiString>())
      return xiiStatus(xiiFmt("Cannot add object to the map property '{0}' as its index type is not a string.", szParentProperty));
    xiiVariant value = accessor.GetValue(szParentProperty, index);
    if (value.IsValid() && value.IsA<xiiUuid>())
    {
      xiiUuid guid = value.Get<xiiUuid>();
      if (guid.IsValid())
        return xiiStatus(
          xiiFmt("Cannot add object to the map property '{0}' at key '{1}'. Delete old value first.", szParentProperty, index.Get<xiiString>()));
    }
  }

  if (pNewParent == GetRootObject())
    pNewParent = nullptr;

  return InternalCanMove(pObject, pNewParent, szParentProperty, index);
}

xiiStatus xiiDocumentObjectManager::CanSelect(const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEV(pObject != nullptr, "pObject must be valid");

  const xiiDocumentObject* pOwnObject = GetObject(pObject->GetGuid());
  if (pOwnObject == nullptr)
    return xiiStatus(
      xiiFmt("Object of type '{0}' is not part of the document and can't be selected", pObject->GetTypeAccessor().GetType()->GetTypeName()));

  return InternalCanSelect(pObject);
}


bool xiiDocumentObjectManager::IsUnderRootProperty(const char* szRootProperty, const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEBUG(m_pObjectStorage->m_RootObject.GetDocumentObjectManager() == pObject->GetDocumentObjectManager(), "Passed in object does not belong to this object manager.");
  while (pObject->GetParent() != GetRootObject())
  {
    pObject = pObject->GetParent();
  }
  return xiiStringUtils::IsEqual(pObject->GetParentProperty(), szRootProperty);
}


bool xiiDocumentObjectManager::IsUnderRootProperty(const char* szRootProperty, const xiiDocumentObject* pParent, const char* szParentProperty) const
{
  XII_ASSERT_DEBUG(pParent == nullptr || m_pObjectStorage->m_RootObject.GetDocumentObjectManager() == pParent->GetDocumentObjectManager(), "Passed in object does not belong to this object manager.");
  if (pParent == nullptr || pParent == GetRootObject())
  {
    return xiiStringUtils::IsEqual(szParentProperty, szRootProperty);
  }
  return IsUnderRootProperty(szRootProperty, pParent);
}

bool xiiDocumentObjectManager::IsTemporary(const xiiDocumentObject* pObject) const
{
  return IsUnderRootProperty("TempObjects", pObject);
}

bool xiiDocumentObjectManager::IsTemporary(const xiiDocumentObject* pParent, const char* szParentProperty) const
{
  return IsUnderRootProperty("TempObjects", pParent, szParentProperty);
}

xiiSharedPtr<xiiDocumentObjectManager::Storage> xiiDocumentObjectManager::SwapStorage(xiiSharedPtr<xiiDocumentObjectManager::Storage> pNewStorage)
{
  XII_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pObjectStorage;

  m_StructureEventsUnsubscriber.Unsubscribe();
  m_PropertyEventsUnsubscriber.Unsubscribe();
  m_ObjectEventsUnsubscriber.Unsubscribe();

  m_pObjectStorage = pNewStorage;

  m_pObjectStorage->m_StructureEvents.AddEventHandler([this](const xiiDocumentObjectStructureEvent& e) { m_StructureEvents.Broadcast(e); }, m_StructureEventsUnsubscriber);
  m_pObjectStorage->m_PropertyEvents.AddEventHandler([this](const xiiDocumentObjectPropertyEvent& e) { m_PropertyEvents.Broadcast(e, 2); }, m_PropertyEventsUnsubscriber);
  m_pObjectStorage->m_ObjectEvents.AddEventHandler([this](const xiiDocumentObjectEvent& e) { m_ObjectEvents.Broadcast(e); }, m_ObjectEventsUnsubscriber);

  return retVal;
}

////////////////////////////////////////////////////////////////////////
// xiiDocumentObjectManager Private Functions
////////////////////////////////////////////////////////////////////////

void xiiDocumentObjectManager::InternalAddObject(xiiDocumentObject* pObject, xiiDocumentObject* pParent, const char* szParentProperty, xiiVariant index)
{
  xiiDocumentObjectStructureEvent e;
  e.m_pDocument        = m_pObjectStorage->m_pDocument;
  e.m_EventType        = xiiDocumentObjectStructureEvent::Type::BeforeObjectAdded;
  e.m_pObject          = pObject;
  e.m_pPreviousParent  = nullptr;
  e.m_pNewParent       = pParent;
  e.m_sParentProperty  = szParentProperty;
  e.m_NewPropertyIndex = index;

  if (e.m_NewPropertyIndex.CanConvertTo<xiiInt32>() && e.m_NewPropertyIndex.ConvertTo<xiiInt32>() == -1)
  {
    xiiIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    e.m_NewPropertyIndex                = accessor.GetCount(szParentProperty);
  }
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  pParent->InsertSubObject(pObject, szParentProperty, e.m_NewPropertyIndex);
  RecursiveAddGuids(pObject);

  e.m_EventType = xiiDocumentObjectStructureEvent::Type::AfterObjectAdded;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void xiiDocumentObjectManager::InternalRemoveObject(xiiDocumentObject* pObject)
{
  xiiDocumentObjectStructureEvent e;
  e.m_pDocument        = m_pObjectStorage->m_pDocument;
  e.m_EventType        = xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved;
  e.m_pObject          = pObject;
  e.m_pPreviousParent  = pObject->m_pParent;
  e.m_pNewParent       = nullptr;
  e.m_sParentProperty  = pObject->m_sParentProperty;
  e.m_OldPropertyIndex = pObject->GetPropertyIndex();
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  pObject->m_pParent->RemoveSubObject(pObject);
  RecursiveRemoveGuids(pObject);

  e.m_EventType = xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void xiiDocumentObjectManager::InternalMoveObject(
  xiiDocumentObject* pNewParent,
  xiiDocumentObject* pObject,
  const char*        szParentProperty,
  xiiVariant         index)
{
  if (pNewParent == nullptr)
    pNewParent = &m_pObjectStorage->m_RootObject;

  xiiDocumentObjectStructureEvent e;
  e.m_pDocument        = m_pObjectStorage->m_pDocument;
  e.m_EventType        = xiiDocumentObjectStructureEvent::Type::BeforeObjectMoved;
  e.m_pObject          = pObject;
  e.m_pPreviousParent  = pObject->m_pParent;
  e.m_pNewParent       = pNewParent;
  e.m_sParentProperty  = szParentProperty;
  e.m_OldPropertyIndex = pObject->GetPropertyIndex();
  e.m_NewPropertyIndex = index;
  if (e.m_NewPropertyIndex.CanConvertTo<xiiInt32>() && e.m_NewPropertyIndex.ConvertTo<xiiInt32>() == -1)
  {
    xiiIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
    e.m_NewPropertyIndex                = accessor.GetCount(szParentProperty);
  }

  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  xiiVariant newIndex = e.getInsertIndex();

  pObject->m_pParent->RemoveSubObject(pObject);
  pNewParent->InsertSubObject(pObject, szParentProperty, newIndex);

  e.m_EventType = xiiDocumentObjectStructureEvent::Type::AfterObjectMoved;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  e.m_EventType = xiiDocumentObjectStructureEvent::Type::AfterObjectMoved2;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void xiiDocumentObjectManager::RecursiveAddGuids(xiiDocumentObject* pObject)
{
  m_pObjectStorage->m_GuidToObject[pObject->m_Guid] = pObject;

  for (xiiUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveAddGuids(pObject->GetChildren()[c]);
}

void xiiDocumentObjectManager::RecursiveRemoveGuids(xiiDocumentObject* pObject)
{
  m_pObjectStorage->m_GuidToObject.Remove(pObject->m_Guid);

  for (xiiUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveRemoveGuids(pObject->GetChildren()[c]);
}

void xiiDocumentObjectManager::PatchEmbeddedClassObjectsInternal(xiiDocumentObject* pObject, const xiiRTTI* pType, bool addToDoc)
{
  const xiiRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    PatchEmbeddedClassObjectsInternal(pObject, pParent, addToDoc);

  xiiIReflectedTypeAccessor& accessor        = pObject->GetTypeAccessor();
  const xiiUInt32            uiPropertyCount = pType->GetProperties().GetCount();
  for (xiiUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const xiiAbstractProperty* pProperty = pType->GetProperties()[i];
    const xiiVariantTypeInfo*  pInfo     = xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pProperty->GetSpecificType());

    if (pProperty->GetCategory() == xiiPropertyCategory::Member && pProperty->GetFlags().IsSet(xiiPropertyFlags::Class) && !pInfo &&
        !pProperty->GetFlags().IsSet(xiiPropertyFlags::Pointer))
    {
      xiiUuid value = accessor.GetValue(pProperty->GetPropertyName()).Get<xiiUuid>();
      XII_ASSERT_DEV(addToDoc || !value.IsValid(), "If addToDoc is false, the current value must be invalid!");
      if (value.IsValid())
      {
        xiiDocumentObject* pEmbeddedObject = GetObject(value);
        if (pEmbeddedObject)
        {
          if (pEmbeddedObject->GetTypeAccessor().GetType() == pProperty->GetSpecificType())
            continue;
          else
          {
            // Type mismatch, delete old.
            InternalRemoveObject(pEmbeddedObject);
          }
        }
      }

      // Create new
      xiiStringBuilder sTemp;
      xiiConversionUtils::ToString(pObject->GetGuid(), sTemp);
      sTemp.Append("/", pProperty->GetPropertyName());
      const xiiUuid      subObjectGuid   = xiiUuid::StableUuidForString(sTemp);
      xiiDocumentObject* pEmbeddedObject = CreateObject(pProperty->GetSpecificType(), subObjectGuid);
      if (addToDoc)
      {
        InternalAddObject(pEmbeddedObject, pObject, pProperty->GetPropertyName(), xiiVariant());
      }
      else
      {
        pObject->InsertSubObject(pEmbeddedObject, pProperty->GetPropertyName(), xiiVariant());
      }
    }
  }
}


const xiiAbstractProperty* xiiDocumentObjectStructureEvent::GetProperty() const
{
  return m_pObject->GetParentPropertyType();
}

xiiVariant xiiDocumentObjectStructureEvent::getInsertIndex() const
{
  if ((m_EventType == Type::BeforeObjectMoved || m_EventType == Type::AfterObjectMoved || m_EventType == Type::AfterObjectMoved2) &&
      m_pNewParent == m_pPreviousParent)
  {
    const xiiIReflectedTypeAccessor& accessor = m_pPreviousParent->GetTypeAccessor();
    const xiiRTTI*                   pType    = accessor.GetType();
    auto*                            pProp    = pType->FindPropertyByName(m_sParentProperty);
    if (pProp->GetCategory() == xiiPropertyCategory::Array || pProp->GetCategory() == xiiPropertyCategory::Set)
    {
      xiiInt32 iCurrentIndex = m_OldPropertyIndex.ConvertTo<xiiInt32>();
      xiiInt32 iNewIndex     = m_NewPropertyIndex.ConvertTo<xiiInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        return xiiVariant(iNewIndex);
      }
    }
  }
  return m_NewPropertyIndex;
}
