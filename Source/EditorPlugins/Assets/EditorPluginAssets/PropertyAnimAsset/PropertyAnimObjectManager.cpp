#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

xiiPropertyAnimObjectManager::xiiPropertyAnimObjectManager() = default;

xiiPropertyAnimObjectManager::~xiiPropertyAnimObjectManager() = default;

xiiStatus xiiPropertyAnimObjectManager::InternalCanAdd(const xiiRTTI* pRtti, const xiiDocumentObject* pParent, xiiStringView sParentProperty, const xiiVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return XII_SUCCESS;

  if (IsTemporary(pParent, sParentProperty))
    return xiiStatus("The structure of the context cannot be animated.");
  return XII_SUCCESS;
}

xiiStatus xiiPropertyAnimObjectManager::InternalCanRemove(const xiiDocumentObject* pObject) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return XII_SUCCESS;

  if (IsTemporary(pObject))
    return xiiStatus("The structure of the context cannot be animated.");
  return XII_SUCCESS;
}

xiiStatus xiiPropertyAnimObjectManager::InternalCanMove(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, xiiStringView sParentProperty, const xiiVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return XII_SUCCESS;

  if (IsTemporary(pObject))
    return xiiStatus("The structure of the context cannot be animated.");
  return XII_SUCCESS;
}
