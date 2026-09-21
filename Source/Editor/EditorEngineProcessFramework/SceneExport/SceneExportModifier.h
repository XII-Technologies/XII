/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Reflection/Reflection.h>

class xiiWorld;

/// Base class for modifiers that can be applied to a world before exporting it. This allows to modify the world in a way that is specific to the document type or the export process, without having to add export specific code to the regular world update loop.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSceneExportModifier : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneExportModifier, xiiReflectedClass);

public:
  /// Creates an instance of each type derived from xiiSceneExportModifier and adds it to the given array.
  static void CreateModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& ref_modifiers);

  /// Destroys all modifiers in the given array and removes them from it.
  static void DestroyModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& ref_modifiers);

  /// Creates an instance of each type derived from xiiSceneExportModifier, applies it to the given world and destroys it again.
  static void ApplyAllModifiers(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport);

  /// Modifies the given world in-place. The document type and guid can be used to apply modifications that are specific to certain document types or even specific documents. The bForExport flag indicates whether the world is being modified for export (true) or for use within the editor (false). This allows to apply different modifications in those two cases, if necessary.
  virtual void ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport) = 0;

  /// Cleans up the world after all modifiers have been applied. This can be used to remove any objects that were marked for deletion during the modification process, but should not be removed immediately to avoid invalidating iterators or handles.
  static void CleanUpWorld(xiiWorld& ref_world);
};
