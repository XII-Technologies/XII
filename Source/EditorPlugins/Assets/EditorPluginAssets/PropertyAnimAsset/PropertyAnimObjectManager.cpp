#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

xiiPropertyAnimObjectManager::xiiPropertyAnimObjectManager() {}

xiiPropertyAnimObjectManager::~xiiPropertyAnimObjectManager() {}

xiiStatus xiiPropertyAnimObjectManager::InternalCanAdd(
  const xiiRTTI*           pRtti,
  const xiiDocumentObject* pParent,
  const char*              szParentProperty,
  const xiiVariant&        index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return xiiStatus(XII_SUCCESS);

  if (IsTemporary(pParent, szParentProperty))
    return xiiStatus("The structure of the context cannot be animated.");
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiPropertyAnimObjectManager::InternalCanRemove(const xiiDocumentObject* pObject) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return xiiStatus(XII_SUCCESS);

  if (IsTemporary(pObject))
    return xiiStatus("The structure of the context cannot be animated.");
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiPropertyAnimObjectManager::InternalCanMove(
  const xiiDocumentObject* pObject,
  const xiiDocumentObject* pNewParent,
  const char*              szParentProperty,
  const xiiVariant&        index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return xiiStatus(XII_SUCCESS);

  if (IsTemporary(pObject))
    return xiiStatus("The structure of the context cannot be animated.");
  return xiiStatus(XII_SUCCESS);
}

bool xiiPropertyAnimObjectManager::IsTemporary(const xiiDocumentObject* pObject) const
{
  while (pObject->GetParent() != GetRootObject())
  {
    pObject = pObject->GetParent();
  }
  return xiiStringUtils::IsEqual(pObject->GetParentProperty(), "TempObjects");
}

bool xiiPropertyAnimObjectManager::IsTemporary(const xiiDocumentObject* pParent, const char* szParentProperty) const
{
  if (pParent == nullptr || pParent == GetRootObject())
  {
    return xiiStringUtils::IsEqual(szParentProperty, "TempObjects");
  }
  return IsTemporary(pParent);
}
