#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>
#include <EnginePluginScene/SceneExport/ExportModifiers.h>
#include <GameEngine/Animation/PathComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier_RemoveShapeIconComponents, 1, xiiRTTIDefaultAllocator<xiiSceneExportModifier_RemoveShapeIconComponents>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport)
{
  XII_LOCK(ref_world.GetWriteMarker());

  if (xiiShapeIconComponentManager* pSiMan = ref_world.GetComponentManager<xiiShapeIconComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      pSiMan->DeleteComponent(it->GetHandle());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier_RemovePathNodeComponents, 1, xiiRTTIDefaultAllocator<xiiSceneExportModifier_RemovePathNodeComponents>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSceneExportModifier_RemovePathNodeComponents::ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport)
{
  if (!bForExport)
    return;

  XII_LOCK(ref_world.GetWriteMarker());

  if (xiiPathComponentManager* pSiMan = ref_world.GetComponentManager<xiiPathComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      it->EnsureControlPointRepresentationIsUpToDate();
      it->SetDisableControlPointUpdates(true);
    }
  }

  if (xiiPathNodeComponentManager* pSiMan = ref_world.GetComponentManager<xiiPathNodeComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      if (it->GetOwner()->GetComponents().GetCount() == 1 && it->GetOwner()->GetChildCount() == 0)
      {
        // if this is the only component on the object, clear it's name, so that the entire object may get cleaned up
        it->GetOwner()->SetName(xiiStringView());
      }

      pSiMan->DeleteComponent(it->GetHandle());
    }
  }
}

//////////////////////////////////////////////////////////////////////////
