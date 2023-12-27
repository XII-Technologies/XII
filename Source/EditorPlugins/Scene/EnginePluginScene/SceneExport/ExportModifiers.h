#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

class XII_ENGINEPLUGINSCENE_DLL xiiSceneExportModifier_RemoveShapeIconComponents : public xiiSceneExportModifier
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneExportModifier_RemoveShapeIconComponents, xiiSceneExportModifier);

public:
  virtual void ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport) override;
};

//////////////////////////////////////////////////////////////////////////

class XII_ENGINEPLUGINSCENE_DLL xiiSceneExportModifier_RemovePathNodeComponents : public xiiSceneExportModifier
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneExportModifier_RemovePathNodeComponents, xiiSceneExportModifier);

public:
  virtual void ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport) override;
};
