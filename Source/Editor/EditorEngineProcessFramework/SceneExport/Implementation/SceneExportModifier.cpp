/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiSceneExportModifier::CreateModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& ref_modifiers)
{
  xiiRTTI::ForEachDerivedType<xiiSceneExportModifier>(
    [&](const xiiRTTI* pRtti) -> void {
      xiiSceneExportModifier* pMod = pRtti->GetAllocator()->Allocate<xiiSceneExportModifier>();
      ref_modifiers.PushBack(pMod);
    },
    xiiRTTI::ForEachOptions::ExcludeNonAllocatable);
}

void xiiSceneExportModifier::DestroyModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& ref_modifiers)
{
  for (auto pModifier : ref_modifiers)
  {
    pModifier->GetDynamicRTTI()->GetAllocator()->Deallocate(pModifier);
  }

  ref_modifiers.Clear();
}

void xiiSceneExportModifier::ApplyAllModifiers(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport)
{
  xiiTemporaryHybridArray<xiiSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pModifier : modifiers)
  {
    pModifier->ModifyWorld(ref_world, sDocumentType, documentGuid, bForExport);
  }

  DestroyModifiers(modifiers);

  CleanUpWorld(ref_world);
}

void xiiSceneExportModifier::CleanUpWorld(xiiWorld& ref_world)
{
  XII_LOCK(ref_world.GetWriteMarker());

  const bool bWasSimulationEnabled = ref_world.GetWorldSimulationEnabled();

  ref_world.SetWorldSimulationEnabled(false);
  ref_world.Update();
  ref_world.SetWorldSimulationEnabled(bWasSimulationEnabled);
}
