#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Prefabs.h>

#include <Core/Prefabs/PrefabResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_Prefabs, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SpawnPrefab, In, "World", In, "Prefab", In, "GlobalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(new xiiFunctionArgumentAttributes(1, new xiiAssetBrowserAttribute("CompatibleAsset_Prefab")), new xiiFunctionArgumentAttributes(3, new xiiDefaultValueAttribute(xiiVariant(xiiInvalidIndex))), new xiiFunctionArgumentAttributes(4, new xiiDefaultValueAttribute(true)), new xiiFunctionArgumentAttributes(5, new xiiDefaultValueAttribute(true))),
    XII_SCRIPT_FUNCTION_PROPERTY(SpawnPrefabAsChild, In, "World", In, "Prefab", In, "Parent", In, "LocalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(new xiiFunctionArgumentAttributes(1, new xiiAssetBrowserAttribute("CompatibleAsset_Prefab")), new xiiFunctionArgumentAttributes(4, new xiiDefaultValueAttribute(xiiVariant(xiiInvalidIndex))), new xiiFunctionArgumentAttributes(5, new xiiDefaultValueAttribute(true)), new xiiFunctionArgumentAttributes(6, new xiiDefaultValueAttribute(true))),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiScriptExtensionAttribute("Prefabs"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void SpawnPrefabHelper(xiiWorld& ref_world, xiiStringView sPrefab, xiiGameObjectHandle hParent, const xiiTransform& transform, xiiUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon, xiiVariantArray& out_rootObjects)
{
  xiiPrefabResourceHandle hPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>(sPrefab);

  xiiResourceLock<xiiPrefabResource> pPrefab(hPrefab, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pPrefab.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  xiiHybridArray<xiiGameObject*, 8> createdRootObjects;
  xiiHybridArray<xiiGameObject*, 8> createdChildObjects;

  xiiPrefabInstantiationOptions opt;
  opt.m_hParent                 = hParent;
  opt.m_pCreatedRootObjectsOut  = &createdRootObjects;
  opt.m_pCreatedChildObjectsOut = &createdChildObjects;

  pPrefab->InstantiatePrefab(ref_world, transform, opt);

  auto FixupObject = [&](xiiGameObject* pObject) {
    if (uiUniqueID != xiiInvalidIndex)
    {
      for (auto pComponent : pObject->GetComponents())
      {
        pComponent->SetUniqueID(uiUniqueID);
      }
    }

    if (bSetCreatedByPrefab)
      pObject->SetCreatedByPrefab();

    if (bSetHideShapeIcon)
      pObject->SetHideShapeIcon();
  };

  for (auto pObject : createdRootObjects)
  {
    FixupObject(pObject);
    out_rootObjects.PushBack(pObject->GetHandle());
  }

  for (auto pObject : createdChildObjects)
  {
    FixupObject(pObject);
  }
}

xiiVariantArray xiiScriptExtensionClass_Prefabs::SpawnPrefab(xiiWorld* pWorld, xiiStringView sPrefab, const xiiTransform& globalTransform, xiiUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return {};

  xiiVariantArray rootObjects;
  SpawnPrefabHelper(*pWorld, sPrefab, xiiGameObjectHandle(), globalTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, rootObjects);
  return rootObjects;
}

xiiVariantArray xiiScriptExtensionClass_Prefabs::SpawnPrefabAsChild(xiiWorld* pWorld, xiiStringView sPrefab, xiiGameObject* pParent, const xiiTransform& localTransform, xiiUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return {};

  xiiVariantArray rootObjects;
  SpawnPrefabHelper(*pWorld, sPrefab, pParent != nullptr ? pParent->GetHandle() : xiiGameObjectHandle(), localTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, rootObjects);
  return rootObjects;
}

XII_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Prefabs);
