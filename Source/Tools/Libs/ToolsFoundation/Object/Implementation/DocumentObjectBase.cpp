#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

xiiIReflectedTypeAccessor& xiiDocumentObject::GetTypeAccessor()
{
  const xiiDocumentObject* pMe = this;
  return const_cast<xiiIReflectedTypeAccessor&>(pMe->GetTypeAccessor());
}

xiiUInt32 xiiDocumentObject::GetChildIndex(const xiiDocumentObject* pChild) const
{
  return m_Children.IndexOf(const_cast<xiiDocumentObject*>(pChild));
}

void xiiDocumentObject::InsertSubObject(xiiDocumentObject* pObject, const char* szProperty, const xiiVariant& index)
{
  XII_ASSERT_DEV(pObject != nullptr, "");
  XII_ASSERT_DEV(!xiiStringUtils::IsNullOrEmpty(szProperty), "Child objects must have a parent property to insert into");
  xiiIReflectedTypeAccessor& accessor = GetTypeAccessor();

  const xiiRTTI* pType = accessor.GetType();
  auto*          pProp = pType->FindPropertyByName(szProperty);
  XII_ASSERT_DEV(pProp && pProp->GetFlags().IsSet(xiiPropertyFlags::Class) &&
                   (!pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) || pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)),
                 "Only class type or pointer to class type that own the object can be inserted, everything else is handled by value.");

  if (pProp->GetCategory() == xiiPropertyCategory::Array || pProp->GetCategory() == xiiPropertyCategory::Set)
  {
    if (!index.IsValid() || (index.CanConvertTo<xiiInt32>() && index.ConvertTo<xiiInt32>() == -1))
    {
      xiiVariant newIndex = accessor.GetCount(szProperty);
      bool       bRes     = accessor.InsertValue(szProperty, newIndex, pObject->GetGuid());
      XII_ASSERT_DEV(bRes, "");
    }
    else
    {
      bool bRes = accessor.InsertValue(szProperty, index, pObject->GetGuid());
      XII_ASSERT_DEV(bRes, "");
    }
  }
  else if (pProp->GetCategory() == xiiPropertyCategory::Map)
  {
    XII_ASSERT_DEV(index.IsA<xiiString>(), "Map key must be a string.");
    bool bRes = accessor.InsertValue(szProperty, index, pObject->GetGuid());
    XII_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == xiiPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(szProperty, pObject->GetGuid());
    XII_ASSERT_DEV(bRes, "");
  }

  // Object patching
  pObject->m_sParentProperty = szProperty;
  pObject->m_pParent         = this;
  m_Children.PushBack(pObject);
}

void xiiDocumentObject::RemoveSubObject(xiiDocumentObject* pObject)
{
  XII_ASSERT_DEV(pObject != nullptr, "");
  XII_ASSERT_DEV(!pObject->m_sParentProperty.IsEmpty(), "");
  XII_ASSERT_DEV(this == pObject->m_pParent, "");
  xiiIReflectedTypeAccessor& accessor = GetTypeAccessor();

  // Property patching
  const xiiRTTI* pType = accessor.GetType();
  auto*          pProp = pType->FindPropertyByName(pObject->m_sParentProperty);
  if (pProp->GetCategory() == xiiPropertyCategory::Array || pProp->GetCategory() == xiiPropertyCategory::Set ||
      pProp->GetCategory() == xiiPropertyCategory::Map)
  {
    xiiVariant index = accessor.GetPropertyChildIndex(pObject->m_sParentProperty, pObject->GetGuid());
    bool       bRes  = accessor.RemoveValue(pObject->m_sParentProperty, index);
    XII_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == xiiPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(pObject->m_sParentProperty, xiiUuid());
    XII_ASSERT_DEV(bRes, "");
  }

  m_Children.RemoveAndCopy(pObject);
  pObject->m_pParent = nullptr;
}

void xiiDocumentObject::ComputeObjectHash(xiiUInt64& uiHash) const
{
  const xiiIReflectedTypeAccessor& acc   = GetTypeAccessor();
  auto                             pType = acc.GetType();

  uiHash = xiiHashingUtils::xxHash64(&m_Guid, sizeof(xiiUuid), uiHash);
  HashPropertiesRecursive(acc, uiHash, pType);
}


xiiDocumentObject* xiiDocumentObject::GetChild(const xiiUuid& guid)
{
  for (auto* pChild : m_Children)
  {
    if (pChild->GetGuid() == guid)
      return pChild;
  }
  return nullptr;
}


const xiiDocumentObject* xiiDocumentObject::GetChild(const xiiUuid& guid) const
{
  for (auto* pChild : m_Children)
  {
    if (pChild->GetGuid() == guid)
      return pChild;
  }
  return nullptr;
}

xiiAbstractProperty* xiiDocumentObject::GetParentPropertyType() const
{
  if (!m_pParent)
    return nullptr;
  const xiiIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  const xiiRTTI*                   pType    = accessor.GetType();
  return pType->FindPropertyByName(m_sParentProperty);
}

xiiVariant xiiDocumentObject::GetPropertyIndex() const
{
  if (m_pParent == nullptr)
    return xiiVariant();
  const xiiIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  return accessor.GetPropertyChildIndex(m_sParentProperty.GetData(), GetGuid());
}

bool xiiDocumentObject::IsOnHeap() const
{
  /// \todo Christopher: This crashes when the pointer is nullptr, which appears to be possible
  /// It happened for me when duplicating (CTRL+D) 2 objects 2 times then moving them and finally undoing everything
  XII_ASSERT_DEV(m_pParent != nullptr,
                 "Object being modified is not part of the document, e.g. may be in the undo stack instead. "
                 "This could happen if within an undo / redo op some callback tries to create a new undo scope / update prefabs etc.");

  if (GetParent() == GetDocumentObjectManager()->GetRootObject())
    return true;

  auto* pProp = GetParentPropertyType();
  return pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner);
}


void xiiDocumentObject::HashPropertiesRecursive(const xiiIReflectedTypeAccessor& acc, xiiUInt64& uiHash, const xiiRTTI* pType) const
{
  // Parse parent class
  const xiiRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    HashPropertiesRecursive(acc, uiHash, pParentType);

  // Parse properties
  xiiUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (xiiUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const xiiAbstractProperty* pProperty = pType->GetProperties()[i];

    if (pProperty->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
      continue;
    if (pProperty->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
      continue;

    if (pProperty->GetCategory() == xiiPropertyCategory::Member)
    {
      const xiiVariant var = acc.GetValue(pProperty->GetPropertyName());
      uiHash               = var.ComputeHash(uiHash);
    }
    else if (pProperty->GetCategory() == xiiPropertyCategory::Array || pProperty->GetCategory() == xiiPropertyCategory::Set)
    {
      xiiHybridArray<xiiVariant, 16> keys;
      acc.GetValues(pProperty->GetPropertyName(), keys);
      for (const xiiVariant& var : keys)
      {
        uiHash = var.ComputeHash(uiHash);
      }
    }
    else if (pProperty->GetCategory() == xiiPropertyCategory::Map)
    {
      xiiHybridArray<xiiVariant, 16> keys;
      acc.GetKeys(pProperty->GetPropertyName(), keys);
      keys.Sort([](const xiiVariant& a, const xiiVariant& b) { return a.Get<xiiString>().Compare(b.Get<xiiString>()) < 0; });
      for (const xiiVariant& key : keys)
      {
        uiHash           = key.ComputeHash(uiHash);
        xiiVariant value = acc.GetValue(pProperty->GetPropertyName(), key);
        uiHash           = value.ComputeHash(uiHash);
      }
    }
  }
}
