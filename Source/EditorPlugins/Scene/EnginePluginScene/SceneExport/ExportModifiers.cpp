/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>
#include <EnginePluginScene/SceneExport/ExportModifiers.h>
#include <GameEngine/Messages/ExportMessage.h>

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier_RemoveShapeIconComponents, 1, xiiRTTIDefaultAllocator<xiiSceneExportModifier_RemoveShapeIconComponents>)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport)
{
  XII_LOCK(ref_world.GetWriteMarker());

  if (xiiShapeIconComponentManager* pComponentManager = ref_world.GetComponentManager<xiiShapeIconComponentManager>())
  {
    for (auto it = pComponentManager->GetComponents(); it.IsValid(); it.Next())
    {
      pComponentManager->DeleteComponent(it);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier_GenericExport, 1, xiiRTTIDefaultAllocator<xiiSceneExportModifier_GenericExport>)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiSceneExportModifier_GenericExport::ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport)
{
  if (!bForExport)
    return;

  xiiStringBuilder sb;
  xiiConversionUtils::ToString(documentGuid, sb);

  XII_LOCK(ref_world.GetWriteMarker());

  xiiMsgExport msg;
  msg.m_sDocumentType = sDocumentType;
  msg.m_sDocumentGuid = sb;

  for (auto it = ref_world.GetObjects(); it.IsValid(); ++it)
  {
    if (!it->IsStatic())
      continue;

    it->SendMessage(msg);
  }
}
