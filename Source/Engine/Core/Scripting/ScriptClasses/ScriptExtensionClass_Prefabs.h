#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class xiiWorld;
class xiiGameObject;

class XII_CORE_DLL xiiScriptExtensionClass_Prefabs
{
public:
  static void SpawnPrefab(xiiWorld* pWorld, xiiStringView sPrefab, const xiiTransform& globalTransform, const xiiVec3& vRelativePosition, const xiiQuat& qRelativeRotation);
  static void SpawnPrefabAsChild(xiiWorld* pWorld, xiiStringView sPrefab, xiiGameObject* pParent, const xiiVec3& vRelativePosition, const xiiQuat& qRelativeRotation);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptExtensionClass_Prefabs);
