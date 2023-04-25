#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>

class XII_ENGINEPLUGINJOLT_DLL xiiSceneExportModifier_JoltStaticMeshConversion : public xiiSceneExportModifier
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneExportModifier_JoltStaticMeshConversion, xiiSceneExportModifier);

public:
  virtual void ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport) override;
};
