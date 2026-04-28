/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSceneExportModifier::CreateModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& ref_modifiers)
{
  xiiRTTI::ForEachDerivedType<xiiSceneExportModifier>(
    [&](const xiiRTTI* pRtti) {
      xiiSceneExportModifier* pMod = pRtti->GetAllocator()->Allocate<xiiSceneExportModifier>();
      ref_modifiers.PushBack(pMod);
    },
    xiiRTTI::ForEachOptions::ExcludeNonAllocatable);
}

void xiiSceneExportModifier::DestroyModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& ref_modifiers)
{
  for (auto pMod : ref_modifiers)
  {
    pMod->GetDynamicRTTI()->GetAllocator()->Deallocate(pMod);
  }

  ref_modifiers.Clear();
}

void xiiSceneExportModifier::ApplyAllModifiers(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport)
{
  xiiHybridArray<xiiSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(ref_world, sDocumentType, documentGuid, bForExport);
  }

  DestroyModifiers(modifiers);

  CleanUpWorld(ref_world);
}

void VisitObject(xiiWorld& ref_world, xiiGameObject* pObject)
{
  for (auto it = pObject->GetChildren(); it.IsValid(); it.Next())
  {
    VisitObject(ref_world, it);
  }

  if (pObject->GetChildCount() > 0)
    return;

  if (!pObject->GetComponents().IsEmpty())
    return;

  if (!pObject->GetName().IsEmpty())
    return;

  if (!pObject->GetGlobalKey().IsEmpty())
    return;

  ref_world.DeleteObjectDelayed(pObject->GetHandle(), false);
}

void xiiSceneExportModifier::CleanUpWorld(xiiWorld& ref_world)
{
  XII_LOCK(ref_world.GetWriteMarker());

  // Don't do this (for now), as we would also delete objects that are referenced by other components,
  // and currently we can't know which ones are important to keep.

  // for (auto it = world.GetObjects(); it.IsValid(); it.Next())
  //{
  //   // only visit objects without parents, those are the root objects
  //   if (it->GetParent() != nullptr)
  //     continue;

  //  VisitObject(world, it);
  //}

  const bool bSim = ref_world.GetWorldSimulationEnabled();
  ref_world.SetWorldSimulationEnabled(false);
  ref_world.Update();
  ref_world.SetWorldSimulationEnabled(bSim);
}
