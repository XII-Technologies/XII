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

void xiiSceneExportModifier::ApplyAllModifiers(xiiWorld& world, const xiiUuid& documentGuid)
{
  xiiHybridArray<xiiSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(world, documentGuid);
  }

  DestroyModifiers(modifiers);
}
