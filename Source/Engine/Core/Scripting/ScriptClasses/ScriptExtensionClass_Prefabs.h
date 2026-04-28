/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class xiiWorld;
class xiiGameObject;

/// Script extension class providing prefab instantiation functionality for scripts.
class XII_CORE_DLL xiiScriptExtensionClass_Prefabs
{
public:
  /// Spawns a prefab instance at the specified global transform.
  ///
  /// \param sPrefab Path or name of the prefab to spawn.
  /// \param globalTransform World position, rotation and scale for the prefab.
  /// \param uiUniqueID Unique identifier for deterministic spawning, use 0 for random.
  /// \param bSetCreatedByPrefab Whether to mark spawned objects as created by prefab.
  /// \param bSetHideShapeIcon Whether to hide shape icons in the editor for spawned objects.
  /// \return Array of game object handles for the spawned prefab's top-level objects.
  static xiiVariantArray SpawnPrefab(xiiWorld* pWorld, xiiStringView sPrefab, const xiiTransform& globalTransform, xiiUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);

  /// Spawns a prefab instance as a child of the specified parent object.
  ///
  /// \param sPrefab Path or name of the prefab to spawn.
  /// \param pParent Parent game object for the spawned prefab.
  /// \param localTransform Local transform relative to the parent.
  /// \param uiUniqueID Unique identifier for deterministic spawning, use 0 for random.
  /// \param bSetCreatedByPrefab Whether to mark spawned objects as created by prefab.
  /// \param bSetHideShapeIcon Whether to hide shape icons in the editor for spawned objects.
  /// \return Array of game object handles for the spawned prefab's top-level objects.
  static xiiVariantArray SpawnPrefabAsChild(xiiWorld* pWorld, xiiStringView sPrefab, xiiGameObject* pParent, const xiiTransform& localTransform, xiiUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptExtensionClass_Prefabs);
