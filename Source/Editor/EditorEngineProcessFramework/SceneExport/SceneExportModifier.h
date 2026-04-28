/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class xiiWorld;

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSceneExportModifier : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneExportModifier, xiiReflectedClass);

public:
  static void CreateModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& ref_modifiers);
  static void DestroyModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& ref_modifiers);

  static void ApplyAllModifiers(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport);

  virtual void ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport) = 0;

  static void CleanUpWorld(xiiWorld& ref_world);
};
