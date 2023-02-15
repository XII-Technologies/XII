#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSceneExportModifier::CreateModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& modifiers)
{
  for (const xiiRTTI* pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<xiiSceneExportModifier>())
      continue;

    if (pRtti->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract))
      continue;

    if (pRtti->GetAllocator() == nullptr || !pRtti->GetAllocator()->CanAllocate())
      continue;

    xiiSceneExportModifier* pMod = pRtti->GetAllocator()->Allocate<xiiSceneExportModifier>();

    modifiers.PushBack(pMod);
  }
}

void xiiSceneExportModifier::DestroyModifiers(xiiHybridArray<xiiSceneExportModifier*, 8>& modifiers)
{
  for (auto pMod : modifiers)
  {
    pMod->GetDynamicRTTI()->GetAllocator()->Deallocate(pMod);
  }

  modifiers.Clear();
}

void xiiSceneExportModifier::ApplyAllModifiers(xiiWorld& world, const xiiUuid& documentGuid, bool bForExport)
{
  xiiHybridArray<xiiSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(world, documentGuid, bForExport);
  }

  DestroyModifiers(modifiers);

  CleanUpWorld(world);
}

void VisitObject(xiiWorld& world, xiiGameObject* pObject)
{
  for (auto it = pObject->GetChildren(); it.IsValid(); it.Next())
  {
    VisitObject(world, it);
  }

  if (pObject->GetChildCount() > 0)
    return;

  if (!pObject->GetComponents().IsEmpty())
    return;

  if (!xiiStringUtils::IsNullOrEmpty(pObject->GetName()))
    return;

  if (!xiiStringUtils::IsNullOrEmpty(pObject->GetGlobalKey()))
    return;

  world.DeleteObjectDelayed(pObject->GetHandle(), false);
}

void xiiSceneExportModifier::CleanUpWorld(xiiWorld& world)
{
  XII_LOCK(world.GetWriteMarker());

  for (auto it = world.GetObjects(); it.IsValid(); it.Next())
  {
    // Only visit objects without parents, those are the root objects
    if (it->GetParent() != nullptr)
      continue;

    VisitObject(world, it);
  }

  const bool bSim = world.GetWorldSimulationEnabled();
  world.SetWorldSimulationEnabled(false);
  world.Update();
  world.SetWorldSimulationEnabled(bSim);
}
