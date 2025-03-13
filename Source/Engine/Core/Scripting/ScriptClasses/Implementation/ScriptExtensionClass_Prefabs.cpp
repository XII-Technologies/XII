#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Prefabs.h>

#include <Core/Prefabs/PrefabResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_Prefabs, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SpawnPrefab, In, "World", In, "Prefab", In, "GlobalTransform", In, "RelativePos", In, "RelativeRotation")->AddAttributes(new xiiFunctionArgumentAttributes(1, new xiiAssetBrowserAttribute("CompatibleAsset_Prefab"))),

    XII_SCRIPT_FUNCTION_PROPERTY(SpawnPrefabAsChild, In, "World", In, "Prefab", In, "Parent", In, "RelativePos", In, "RelativeRotation")->AddAttributes(new xiiFunctionArgumentAttributes(1, new xiiAssetBrowserAttribute("CompatibleAsset_Prefab"))),
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

void xiiScriptExtensionClass_Prefabs::SpawnPrefab(xiiWorld* pWorld, xiiStringView sPrefab, const xiiTransform& globalTransform, const xiiVec3& vRelativePosition, const xiiQuat& qRelativeRotation)
{
  if (sPrefab.IsEmpty())
    return;

  xiiPrefabResourceHandle hPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>(sPrefab);

  xiiResourceLock<xiiPrefabResource> pPrefab(hPrefab, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pPrefab.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  xiiTransform trans = xiiTransform::MakeGlobalTransform(globalTransform, xiiTransform(vRelativePosition, qRelativeRotation));

  pPrefab->InstantiatePrefab(*pWorld, trans, {});
}

void xiiScriptExtensionClass_Prefabs::SpawnPrefabAsChild(xiiWorld* pWorld, xiiStringView sPrefab, xiiGameObject* pParent, const xiiVec3& vRelativePosition, const xiiQuat& qRelativeRotation)
{
  if (sPrefab.IsEmpty())
    return;

  xiiPrefabResourceHandle hPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>(sPrefab);

  xiiResourceLock<xiiPrefabResource> pPrefab(hPrefab, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pPrefab.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  xiiPrefabInstantiationOptions opt;
  opt.m_hParent = pParent ? pParent->GetHandle() : xiiGameObjectHandle();

  pPrefab->InstantiatePrefab(*pWorld, xiiTransform(vRelativePosition, qRelativeRotation), opt);
}
